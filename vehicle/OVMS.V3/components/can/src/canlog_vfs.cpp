/*
;    Project:       Open Vehicle Monitor System
;    Module:        CAN logging framework
;    Date:          18th January 2018
;
;    (C) 2018       Michael Balzer
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "ovms_log.h"
static const char *TAG = "canlog-vfs";

#include <sys/stat.h>
#include "can.h"
#include "canformat.h"
#include "canlog_vfs.h"
#include "ovms_utils.h"
#include "ovms_config.h"
#include "ovms_nvs.h"
#include "ovms_peripherals.h"

static const char *CAN_PARAM = "can";

void can_log_vfs_start(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  std::string format(cmd->GetName());
  canlog_vfs* logger = new canlog_vfs(argv[0],format);
  logger->Open();

  if (logger->IsOpen())
    {
    if (argc>1)
      { MyCan.AddLogger(logger, argc-1, &argv[1]); }
    else
      { MyCan.AddLogger(logger); }
    writer->printf("CAN logging to VFS active: %s\n", logger->GetInfo().c_str());
    MyCan.LogInfo(NULL, CAN_LogInfo_Config, logger->GetInfo().c_str());
    }
  else
    {
    writer->printf("Error: Could not start CAN logging to: %s\n", logger->GetInfo().c_str());
    delete logger;
    }
  }

void can_log_vfs_autostart(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  std::string format(cmd->GetName());
  canlog_vfs* logger = new canlog_vfs_autonaming(argv[0],format);
  logger->Open();

  if (logger->IsOpen())
    {
    if (argc>1)
      { MyCan.AddLogger(logger, argc-1, &argv[1]); }
    else
      { MyCan.AddLogger(logger); }
    writer->printf("CAN logging to VFS with autonaming active: %s\n", logger->GetInfo().c_str());
    MyCan.LogInfo(NULL, CAN_LogInfo_Config, logger->GetInfo().c_str());
    }
  else
    {
    writer->printf("Error: Could not start CAN logging with autonaming to: %s\n", logger->GetInfo().c_str());
    delete logger;
    }
  }

class OvmsCanLogVFSInit
  {
  public: OvmsCanLogVFSInit();
} MyOvmsCanLogVFSInit  __attribute__ ((init_priority (4560)));

OvmsCanLogVFSInit::OvmsCanLogVFSInit()
  {
  ESP_LOGI(TAG, "Initialising CAN logging to VFS (4560)");

  OvmsCommand* cmd_can = MyCommandApp.FindCommand("can");
  if (cmd_can)
    {
    OvmsCommand* cmd_can_log = cmd_can->FindCommand("log");
    if (cmd_can_log)
      {
      OvmsCommand* cmd_can_log_start = cmd_can_log->FindCommand("start");
      if (cmd_can_log_start)
        {
        // We have a place to put our command tree..
        OvmsCommand* start = cmd_can_log_start->RegisterCommand("vfs", "CAN logging to VFS");
        MyCanFormatFactory.RegisterCommandSet(start, "Start CAN logging to VFS",
          can_log_vfs_start,
          "<path> [filter1] ... [filterN]\n"
          "Filter: <bus> | <id>[-<id>] | <bus>:<id>[-<id>]\n"
          "Example: 2:2a0-37f",
          1, 9);

        OvmsCommand* autostart = cmd_can_log_start->RegisterCommand("vfs-auto", "CAN logging to VFS with autonaming (automatic file name)");
        MyCanFormatFactory.RegisterCommandSet(autostart, "Start CAN logging to VFS with autonaming",
          can_log_vfs_autostart,
          "<naming prefix> [filter1] ... [filterN]\n"
          "Filter: <bus> | <id>[-<id>] | <bus>:<id>[-<id>]\n"
          "Example: 2:2a0-37f",
          1, 9);

        }
      }
    }
  }


canlog_vfs_conn::canlog_vfs_conn(canlog* logger, std::string format, canformat::canformat_serve_mode_t mode)
  : canlogconnection(logger, format, mode), m_file_size(0)
  {
  m_file = NULL;
  }

canlog_vfs_conn::~canlog_vfs_conn()
  {
  if (m_file)
    {
    fclose(m_file);
    m_file = NULL;
    }
  }

void canlog_vfs_conn::OutputMsg(CAN_log_message_t& msg, std::string &result)
  {
  m_msgcount++;

  if ((m_filters != NULL) && (! m_filters->IsFiltered(&msg.frame)))
    {
    m_filtercount++;
    return;
    }

  if (result.length()>0)
    {
    fwrite(result.c_str(),result.length(),1,m_file);
    m_file_size += result.length();
    }
  }


canlog_vfs::canlog_vfs(std::string path, std::string format)
  : canlog("vfs", format)
  {
  m_path = path;
  using std::placeholders::_1;
  using std::placeholders::_2;
  MyEvents.RegisterEvent(IDTAG, "sd.mounted", std::bind(&canlog_vfs::MountListener, this, _1, _2));
  MyEvents.RegisterEvent(IDTAG, "sd.unmounting", std::bind(&canlog_vfs::MountListener, this, _1, _2));
  }

canlog_vfs::~canlog_vfs()
  {
  MyEvents.DeregisterEvent(IDTAG);

  if (m_isopen)
    {
    Close();
    }
  }

bool canlog_vfs::Open()
  {
  OvmsRecMutexLock lock(&m_cmmutex);

  if (m_isopen)
    {
    for (conn_map_t::iterator it=m_connmap.begin(); it!=m_connmap.end(); ++it)
      {
      delete it->second;
      }
    m_connmap.clear();
    m_isopen = false;
    }

  if (MyConfig.ProtectedPath(m_path))
    {
    ESP_LOGE(TAG, "Error: Path '%s' is protected and cannot be opened", m_path.c_str());
    return false;
    }

#ifdef CONFIG_OVMS_COMP_SDCARD
  if (startsWith(m_path, "/sd") && (!MyPeripherals || !MyPeripherals->m_sdcard || !MyPeripherals->m_sdcard->isavailable()))
    {
    ESP_LOGE(TAG, "Error: Cannot open '%s' as SD filesystem not available", m_path.c_str());
    return false;
    }
#endif // #ifdef CONFIG_OVMS_COMP_SDCARD

  canlog_vfs_conn* clc = new canlog_vfs_conn(this, m_format, m_mode);
  clc->m_peer = m_path;

  clc->m_file = fopen(m_path.c_str(), "w");
  if (!clc->m_file)
    {
    ESP_LOGE(TAG, "Error: Can't write to '%s'", m_path.c_str());
    delete clc;
    return false;
    }

  ESP_LOGI(TAG, "Now logging CAN messages to '%s'", m_path.c_str());

  std::string header = m_formatter->getheader();
  if (header.length()>0)
    {
    fwrite(header.c_str(),header.length(),1,clc->m_file);
    clc->m_file_size += header.length();
    }

  m_connmap[NULL] = clc;
  m_isopen = true;

  return true;
  }

void canlog_vfs::Close()
  {
  if (m_isopen)
    {
    ESP_LOGI(TAG, "Closed vfs log '%s': %s",
      m_path.c_str(), GetStats().c_str());

    OvmsRecMutexLock lock(&m_cmmutex);
    for (conn_map_t::iterator it=m_connmap.begin(); it!=m_connmap.end(); ++it)
      {
      delete it->second;
      }
    m_connmap.clear();

    m_isopen = false;
    }
  }

size_t canlog_vfs::GetFileSize()
  {
  size_t result = 0;
  if (m_isopen)
    {
    for (conn_map_t::iterator it=m_connmap.begin(); it!=m_connmap.end(); ++it)
      {
      canlog_vfs_conn * clc = static_cast<canlog_vfs_conn *>(it->second);
      result += clc->m_file_size;
      }
    }
  return result;
  }

std::string canlog_vfs_conn::GetStats()
  {
  char bufsize[15];
  format_file_size(bufsize, sizeof(bufsize), m_file_size);

  std::string result = "Size:";
  result.append(bufsize);
  result.append(" ");
  result.append(canlogconnection::GetStats());

  return result;
  }

std::string canlog_vfs::GetStats()
  {
  char bufsize[15];
  size_t size = GetFileSize();
  format_file_size(bufsize, sizeof(bufsize), size);

  std::string result = "Size:";
  result.append(bufsize);
  result.append(" ");
  result.append(canlog::GetStats());

  return result;
  }

std::string canlog_vfs::GetInfo()
  {
  std::string result = canlog::GetInfo();
  result.append(" Path:");
  result.append(m_path);
  return result;
  }

void canlog_vfs::MountListener(std::string event, void* data)
  {
  if (event == "sd.unmounting" && startsWith(m_path, "/sd"))
    Close();
  else if (event == "sd.mounted" && startsWith(m_path, "/sd"))
    Open();
  }

static void replaceAll(std::string& str, const std::string& from, const std::string& to)
  {
  if(from.empty())
    {
    return;
    }
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
  }

std::string canlog_vfs_autonaming::compute_log_file_name()
  {
  std::string path;

  std::string vehicleid = MyConfig.GetParamValue("vehicle", "id", "OVMS");
  char restarts[9];
  uint64_t restart_count = MyNonVolatileStorage.GetRestartCount();
  snprintf(restarts, sizeof(restarts), "%08lld", restart_count);

  char splits[9];
  snprintf(splits, sizeof(splits), "%08lld", m_file_nb_splits);

  // Handle patterns replacements
  path = m_file_name_pattern;
  std::map<std::string, std::string>New_Map = {
    {"{vehicleid}", vehicleid},
    {"{session}", restarts},
    {"{prefix}", m_prefix},
    {"{splits}", splits},
    {"{extension}", m_formatter->preferred_file_extension()},
  };
  for(auto x: New_Map)
    {
    replaceAll(path, x.first, x.second);
    }

  // Handle time-based replacements
  time_t t = time(nullptr);
  char replaced_path[PATH_MAX];
  std::strftime(replaced_path, sizeof(replaced_path), path.c_str(), std::localtime(&t));
  path = replaced_path;

  // Remove consecutive slashes / dots;
  for (auto i = 1; i < path.length(); /* erase shifts the pointer */ )
    {
    if ((path[i - 1] == path[i]) && ((path[i] == '/') || (path[i] == '.')))
      {
      path.erase(i, 1);
      }
    else
      {
      ++i;
      }
    }

  // Creates directory hierarchy if necessary
  // (Note: we always call mkdir, which will fail if the directory already exists)
  if (path.length() > 1)
    {
    // We skip the first component which is the pseudo-mount point of the FS
    size_t slashpos = path.find('/', 2);
    if (path.length() > 1 + slashpos)
      {
      slashpos = path.find('/', 1 + slashpos);
      for (size_t i = slashpos; i < path.length(); i = path.find('/', i+1))
        {
        // printf("%s\n", path.substr(0, i).c_str());
        mkdir(path.substr(0, i).c_str(), 0);
        }
      }
    }
  return path;
  }

void canlog_vfs_autonaming::CycleLogfile()
  {
  if (!m_keep_empty_files && ((GetFileSize() == 0) || (m_msgcount == 0)))
    {
    ESP_LOGD(TAG, "File was empty, not changing the name");
    }
  else
    {
    m_file_nb_splits++;
    std::string new_path = compute_log_file_name();
    if (new_path != m_path)
      {
      bool was_open = m_isopen;
      if (was_open)
        {
        Close();
        m_msgcount = 0;
        m_dropcount = 0;
        m_filtercount = 0;
        }
      ESP_LOGD(TAG, "Changing file name from '%s' to '%s'", m_path.c_str(), new_path.c_str());
      m_path = new_path;
      if (was_open)
        {
        Open();
        }
      }
    else
      {
      ESP_LOGD(TAG, "File name is unchanged");
      }
    }
  }

bool canlog_vfs_autonaming::Open()
  {
  m_logfile_start_time = esp_timer_get_time();
  return canlog_vfs::Open();
  }


canlog_vfs_autonaming::canlog_vfs_autonaming(std::string prefix, std::string format)
  : canlog_vfs("", format), m_prefix(prefix)
  {
  ReadConfig();
  m_path = compute_log_file_name();
  using std::placeholders::_1;
  using std::placeholders::_2;
  MyEvents.RegisterEvent(IDTAG, "config.mounted", std::bind(&canlog_vfs_autonaming::EventHandler, this, _1, _2));
  MyEvents.RegisterEvent(IDTAG, "config.changed", std::bind(&canlog_vfs_autonaming::EventHandler, this, _1, _2));
  MyEvents.RegisterEvent(IDTAG, "can.log.rotatefiles", std::bind(&canlog_vfs_autonaming::EventHandler, this, _1, _2));
  }

/**
 * Load, or reload, the configuration if a config event occurred.
 */
void canlog_vfs_autonaming::EventHandler(std::string event, void* data)
  {
  if (event == "config.changed")
    {
    // Only reload if our parameter has changed
    OvmsConfigParam* param = (OvmsConfigParam*)data;
    if (param && param->GetName() == CAN_PARAM)
      {
      ReadConfig();
      }
    }
  else if (event == "config.mounted")
    {
    ReadConfig();
    }
  else if (event == "can.log.rotatefiles")
    {
    CycleLogfile();
    }
  }

/**
 * Load, or reload, the configuration of log file (name templating, rotation, ...).
 */
void canlog_vfs_autonaming::ReadConfig()
  {
  m_keep_empty_files = MyConfig.GetParamValueBool(CAN_PARAM, "log.file.keep_empty", true);
  m_logfile_max_size_kb = MyConfig.GetParamValueInt(CAN_PARAM, "log.file.maxsize_kb", 1024);
  m_logfile_max_duration_s = MyConfig.GetParamValueInt(CAN_PARAM, "log.file.maxduration_s", 1800);
  m_file_name_pattern = MyConfig.GetParamValue(CAN_PARAM, "log.file.pattern", "/sd/log/{vehicleid}/{session}/{prefix}/{splits}-%Y%m%d-%H%M%S{extension}");
  }

void canlog_vfs_autonaming::OutputMsg(CAN_log_message_t& msg)
  {
    size_t logfile_size = GetFileSize();
    int64_t logfile_duration_s = (esp_timer_get_time() - m_logfile_start_time)/1000000;
    ESP_LOGD(TAG, "canlog_vfs_autonaming::OutputMsg() size:%d, log duration: %llds", logfile_size, logfile_duration_s);
    // We check the duration before logging (in case the messages are not so frequent,
    // we can respect the max duration before logging)
    if (m_logfile_max_duration_s &&  logfile_duration_s >= m_logfile_max_duration_s)
      {
      CycleLogfile();
      }
    canlog_vfs::OutputMsg(msg);
    // We check the size after logging (in case the message we just wrote causes us
    // to surpass the max size)
    if (m_logfile_max_size_kb && logfile_size >= (m_logfile_max_size_kb*1024))
      {
      CycleLogfile();
      }
  }