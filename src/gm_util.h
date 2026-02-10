#ifndef __GM_UTIL_H__
#define __GM_UTIL_H__

#include <time.h>

// Function to check if a file exists
int file_exists(const char *filename);

time_t get_file_mtime(const char *path);

#endif // __GM_UTIL_H__