#pragma once

#include <stdarg.h>

#define VA_BEG va_list va;va_start(va,msg);
#define VA_RST va_end(va);va_start(va,msg);
#define VA_END va_end(va);
#define MAX_STRLEN 1024
#define DT_STRLEN 20

#ifdef _DEBUG
#define LOG_DEBUG_INFO(...) log_console_info(__VA_ARGS__)
#define LOG_DEBUG_WARNING(...) log_console_info(__VA_ARGS__)
#define LOG_DEBUG_ERROR(...) log_console_error(__VA_ARGS__)
#define LOG_DEBUG_FATAL(...) log_console_fatal(__VA_ARGS__)
#else
#define LOG_DEBUG_INFO(...)
#define LOG_DEBUG_WARNING(...)
#define LOG_DEBUG_ERROR(...)
#define LOG_DEBUG_FATAL(...)
#endif

void log_console_info(const char *, ...);
void log_console_warning(const char *, ...);
void log_console_error(const char *, ...);
void log_console_fatal(const char *, ...);
