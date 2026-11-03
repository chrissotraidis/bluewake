/* <strings.h> for BlueWake on Windows. */
#ifndef BW_COMPAT_STRINGS_H
#define BW_COMPAT_STRINGS_H
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
