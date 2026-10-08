// BlueWake for Linux: crash reporting into the session log's folder.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void bw_crash_install(const char* data_dir);
void bw_crash_test(void);
#ifdef __cplusplus
}
#endif
