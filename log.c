#include "log.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

void write_log(const char *type, const char *msg, va_list va) {
  time_t timestamp;
  GetSystemTimeAsFileTime((FILETIME *)&timestamp);
  timestamp -= 116444736000000000i64; // 1 Jan 1601 to 1 Jan 1970
  struct timespec ts = {
    (time_t)(timestamp / 10000000i64),
    (time_t)(timestamp % 10000000i64 * 100)
  };
  struct tm t;
  localtime_s(&t, &ts.tv_sec);
  char dt_str[DT_STRLEN];
  strftime(dt_str, DT_STRLEN, "%Y-%m-%d %H:%M:%S", &t);
  int ms = (int)(ts.tv_nsec / 1.0e6);
  // Log to system console
  int format_size = _scprintf("[%s.%03d] %5s %s\n", dt_str, ms, type, msg) + 1;
  char *format = _alloca(format_size);
  snprintf(format, format_size, "[%s.%03d] %5s %s\n", dt_str, ms, type, msg);
  // NB If format is invalid (e.g. %l instead of %ld) _vscprintf throws an
  // exception when it tries to find the length of the corresponding argument
  int buffer_size = _vscprintf(format, va) + 1;
  char *buffer = format;
  if (buffer_size > format_size)
    buffer = _alloca(buffer_size);
  vsnprintf(buffer, buffer_size, format, va);
  OutputDebugString(buffer);
}

void log_console_info(const char *msg, ...) {
  VA_BEG
    write_log("INFO", msg, va);
  VA_END
}

void log_console_warning(const char *msg, ...) {
  VA_BEG
    write_log("WARN", msg, va);
  VA_END
}

void log_console_error(const char *msg, ...) {
  VA_BEG
    write_log("ERROR", msg, va);
  VA_END
}

void log_console_fatal(const char *msg, ...) {
  VA_BEG
    write_log("FATAL", msg, va);
  VA_END
}
