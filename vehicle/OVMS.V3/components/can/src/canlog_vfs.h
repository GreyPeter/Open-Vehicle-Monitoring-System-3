/*
;    Project:       Open Vehicle Monitor System
;    Module:        CAN logging framework
;    Date:          18th January 2018
;
;    (C) 2018       Michael Balzer
;    (C) 2019       Mark Webb-Johnson
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

#ifndef __CANLOG_VFS_H__
#define __CANLOG_VFS_H__

#include "canlog.h"


class canlog_vfs_conn: public canlogconnection
  {
  public:
    canlog_vfs_conn(canlog* logger, std::string format, canformat::canformat_serve_mode_t mode);
    virtual ~canlog_vfs_conn();

  public:
    virtual void OutputMsg(CAN_log_message_t& msg, std::string &result);
    virtual std::string GetStats();

  public:
    FILE*               m_file;
    size_t              m_file_size;
  };


class canlog_vfs : public canlog
  {
  public:
    canlog_vfs(std::string path, std::string format);
    virtual ~canlog_vfs();

  public:
    virtual bool Open();
    virtual void Close();
    virtual std::string GetInfo();
    virtual size_t GetFileSize();

  public:
    virtual void MountListener(std::string event, void* data);
    virtual std::string GetStats();

  public:
    std::string         m_path;
  };

/**
 * This class has the same target (VFS) than `canlog_vfs`, and adds the
 * possibility of "cycling" the logs following some conditions. ("cycling" meaning
 * that the log file will be closed ; and a new log file will be opened)
 * The file can be cycled after:
 * * a specific size has been reached (configuration item `[can] log.file.maxsize_kb`)
 * * a specific duration of logs has been reached (configuration item `[can] log.file.maxduration_s`)
 *
 * An additional configuration item is introduced: `[can] log.file.keep_empty` where
 * `true` will cycle the file name even if the file is empty (no messages logged),
 * while `false` will prevent the cycling of an empty file - in order NOT to create
 * files with a size of 0 (or having only their header written to storage)
 *
 * The file name pattern can also be configured with the configuration item
 * `[can] log.file.pattern` which can be customized with some or part of the following
 * parameters:
 * * `{vehicleid}` will be replaced by the configured vehicle id
 * * `{session}` will be replaced by the counter of restarts of the module (always incrementing)
 * * `{prefix}` is an argument to the log command, so that you can configure multiple logs in parallel
 * * `{splits}` is a counter of the number of cycles that occurred to this log file
 * * `{extension}` is the preferred extension for the choosen log format
 *
 * Also in this pattern it's possible to use time-based conversion specifications
 * from `strftime` like `%Y%m%d-%H%M%S`.
 *
 * This pattern can of course have multiple directories and subdirectories ; those will
 * be created if necessary of the choosen VFS.
 *
 * Finally, a new event can be raised: `can.log.rotate_files` to force the rotation
 * of the file name (while respecting the `[can] log.file.keep_empty` configuration item)
 */
class canlog_vfs_autonaming : public canlog_vfs
  {
  public:
    canlog_vfs_autonaming(std::string prefix, std::string format);
    virtual ~canlog_vfs_autonaming() {};

  public:
    virtual bool Open();
    virtual void ReadConfig();
    virtual void EventHandler(std::string event, void* data);
    virtual void OutputMsg(CAN_log_message_t& msg);

  protected:
    virtual std::string compute_log_file_name();
    virtual void CycleLogfile();

  protected:
    std::string         m_prefix;
    std::string         m_file_name_pattern;
    std::size_t         m_file_name_pattern_hash = 0;
    uint64_t            m_file_nb_splits = 1;
    bool                m_keep_empty_files = true;
    size_t              m_logfile_max_size_kb = 0;
    int64_t             m_logfile_start_time = 0;
    size_t              m_logfile_max_duration_s = 0;
  };

#endif // __CANLOG_VFS_H__
