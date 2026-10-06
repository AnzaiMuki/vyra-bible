// Test stub of the plugin's logging header.
#pragma once
#define LOG_DEBUG 400
#define LOG_INFO 300
#define LOG_ERROR 100
#ifdef __cplusplus
extern "C" {
#endif
void obs_log(int log_level, const char *format, ...);
#ifdef __cplusplus
}
#endif
