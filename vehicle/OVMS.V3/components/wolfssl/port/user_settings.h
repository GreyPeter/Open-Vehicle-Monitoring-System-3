/* user_settings.h
 *
 * Part of this file Copyright (C) wolfSSL Inc. (GPL2+)
 * See: https://github.com/wolfSSL/wolfssl/blob/master/IDE/Espressif/ESP-IDF/user_settings.h
 */

// Beginning of file : specific to OVMSv3
// --------------------------------------

// For compatibility of WolfSSH with ESP-IDF

#include "esp_idf_version.h"

#define BUILDING_WOLFSSH
#define WOLFSSH_LWIP
//#define DEFAULT_HIGHWATER_MARK (1024 * 4)
#define DEFAULT_HIGHWATER_MARK ((1024 * 1024 * 1024) - (32 * 1024))
#define DEFAULT_WINDOW_SZ (1024*2)
#define DEFAULT_MAX_PACKET_SZ (1024*2)
#define WOLFSSH_LOG_PRINTF

// // For compatibility of WolfSSL with ESP-IDF

#define BUILDING_WOLFSSL
#define HAVE_VISIBILITY 1
#define NO_DEV_RANDOM
#define NO_MAIN_DRIVER
#define WOLFSSL_LWIP
#define WOLFSSL_KEY_GEN
#define SIZEOF_LONG 4
#define HAVE_GETADDRINFO 1
#define HAVE_GMTIME_R 1

#define USE_WOLFSSL_MEMORY
#define XMALLOC_USER
#define XMALLOC(s, h, t)     wolfSSL_Malloc(s)
#define XFREE(p, h, t)       wolfSSL_Free(p)
#define XREALLOC(p, n, h, t) wolfSSL_Realloc(p, n)

// Inclusion and exclusion of WolfSSL features, may be adjusted

// #define OPENSSL_EXTRA // -> compile error ssl.c:18011:22: error: size of array 'sha_test' is negative
// #define OPENSSL_ALL // -> compile error ssl.c:18011:22: error: size of array 'sha_test' is negative
#define WC_NO_HARDEN
#define HAVE_EX_DATA
#define NO_DES3
#define NO_DSA
#define NO_ERROR_STRINGS
#define NO_HC128
#define NO_MD4
#define NO_PWDBASED
#define NO_RABBIT
#define NO_RC4
#define SMALL_SESSION_CACHE
#define ECC_SHAMIR
#define ECC_TIMING_RESISTANT
#define HAVE_WC_ECC_SET_RNG
//#define HAVE_CHACHA
#define HAVE_EXTENDED_MASTER
#define HAVE_HASHDRBG
#define HAVE_ONE_TIME_AUTH
//#define HAVE_POLY1305
#define HAVE_THREAD_LS
#define TFM_ECC256
#define TFM_TIMING_RESISTANT
#define WC_NO_ASYNC_THREADING
#define WC_RSA_BLINDING
#define WOLFSSL_BASE64_ENCODE
#define WOLFSSL_SHA224
//#define WOLFSSL_SHA3
#define WOLFSSL_SHA384
#define WOLFSSL_CERT_EXT
#define NO_WOLFSSL_STUB
#define WOLFSSL_OLD_PRIME_CHECK


// Below : content from https://github.com/wolfSSL/wolfssl/blob/master/IDE/Espressif/ESP-IDF/user_settings.h
// ---------------------------------------------------------------------------------------------------------

#undef WOLFSSL_ESPIDF
#undef WOLFSSL_ESPWROOM32
#undef WOLFSSL_ESPWROOM32SE
#undef WOLFSSL_ESPWROOM32
#undef WOLFSSL_ESP8266

#define WOLFSSL_ESPIDF

/*
 * choose ONE of these Espressif chips to define:
 *
 * WOLFSSL_ESPWROOM32
 * WOLFSSL_ESPWROOM32SE
 * WOLFSSL_ESP8266
 */

#define WOLFSSL_ESPWROOM32

/* #define DEBUG_WOLFSSL_VERBOSE */

#define BENCH_EMBEDDED
#define USE_CERT_BUFFERS_2048

/* TLS 1.3                                 */
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define WC_RSA_PSS
#define HAVE_HKDF
#define HAVE_AEAD
#define HAVE_SUPPORTED_CURVES

/* when you want to use SINGLE THREAD */
/* #define SINGLE_THREADED */
#define NO_FILESYSTEM

#define HAVE_AESGCM
/* when you want to use SHA384 */
/* #define WOLFSSL_SHA384 */
#define WOLFSSL_SHA512
#define HAVE_ECC
#define HAVE_CURVE25519
#define CURVE25519_SMALL
#define HAVE_ED25519

/* when you want to use pkcs7 */
/* #define HAVE_PKCS7 */

#if defined(HAVE_PKCS7)
    #define HAVE_AES_KEYWRAP
    #define HAVE_X963_KDF
    #define WOLFSSL_AES_DIRECT
#endif

/* when you want to use aes counter mode */
/* #define WOLFSSL_AES_DIRECT */
/* #define WOLFSSL_AES_COUNTER */

/* esp32-wroom-32se specific definition */
#if defined(WOLFSSL_ESPWROOM32SE)
    #define WOLFSSL_ATECC508A
    #define HAVE_PK_CALLBACKS
    /* when you want to use a custom slot allocation for ATECC608A */
    /* unless your configuration is unusual, you can use default   */
    /* implementation.                                             */
    /* #define CUSTOM_SLOT_ALLOCATION                              */
#endif

#if ESP_IDF_VERSION_MAJOR >= 4
/* rsa primitive specific definition */
#if defined(WOLFSSL_ESPWROOM32) || defined(WOLFSSL_ESPWROOM32SE)
    /* Define USE_FAST_MATH and SMALL_STACK                        */
    #define ESP32_USE_RSA_PRIMITIVE
    /* threshold for performance adjustment for hw primitive use   */
    /* X bits of G^X mod P greater than                            */
    #define EPS_RSA_EXPT_XBTIS           36
    /* X and Y of X * Y mod P greater than                         */
    #define ESP_RSA_MULM_BITS            2000
#endif
#endif

/* debug options */
/* #define DEBUG_WOLFSSL */
/* #define WOLFSSL_ESP32WROOM32_CRYPT_DEBUG */
/* #define WOLFSSL_ATECC508A_DEBUG          */

/* date/time                               */
/* if it cannot adjust time in the device, */
/* enable macro below                      */
/* #define NO_ASN_TIME */
/* #define XTIME time */

/* when you want not to use HW acceleration */
/* #define NO_ESP32WROOM32_CRYPT */
/* #define NO_WOLFSSL_ESP32WROOM32_CRYPT_HASH*/
/* #define NO_WOLFSSL_ESP32WROOM32_CRYPT_AES */
/* #define NO_WOLFSSL_ESP32WROOM32_CRYPT_RSA_PRI */

/* adjust wait-timeout count if you see timeout in rsa hw acceleration */
#define ESP_RSA_TIMEOUT_CNT    0x249F00
