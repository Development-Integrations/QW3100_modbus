#ifndef LOGGER_H
#define LOGGER_H

/*
 * logger.h — macros de log con timestamp y nivel.
 *
 * LOG_INFO  → stdout  (capturado por journald como priority INFO)
 * LOG_WARN  → stderr  (capturado por journald como priority WARNING)
 * LOG_ERROR → stderr  (capturado por journald como priority ERR)
 *
 * Formato: [YYYY-MM-DD HH:MM:SS] [LEVEL] mensaje
 */

#include <stdio.h>
#include <time.h>

static inline void _log_ts(char *buf, size_t sz)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", t);
}

#define LOG_INFO(fmt, ...) \
    do { char _ts[26]; _log_ts(_ts, sizeof(_ts)); \
         printf("[%s] [INFO]  " fmt "\n", _ts, ##__VA_ARGS__); \
         fflush(stdout); } while (0)

#define LOG_WARN(fmt, ...) \
    do { char _ts[26]; _log_ts(_ts, sizeof(_ts)); \
         fprintf(stderr, "[%s] [WARN]  " fmt "\n", _ts, ##__VA_ARGS__); \
         fflush(stderr); } while (0)

#define LOG_ERROR(fmt, ...) \
    do { char _ts[26]; _log_ts(_ts, sizeof(_ts)); \
         fprintf(stderr, "[%s] [ERROR] " fmt "\n", _ts, ##__VA_ARGS__); \
         fflush(stderr); } while (0)

#endif /* LOGGER_H */
