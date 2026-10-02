/* <dirent.h> for BlueWake on Windows (FindFirstFile underneath; see
 * ../bw_posix_compat.c). */
#ifndef BW_COMPAT_DIRENT_H
#define BW_COMPAT_DIRENT_H
#ifdef __cplusplus
extern "C" {
#endif
#define DT_UNKNOWN 0
#define DT_DIR 4
#define DT_REG 8
struct dirent {
    unsigned char d_type;
    char d_name[1040];
};
typedef struct BwDir DIR;
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dir);
int closedir(DIR* dir);
#ifdef __cplusplus
}
#endif
#endif
