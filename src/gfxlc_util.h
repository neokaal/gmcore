#ifndef __GFCLC_UTIL_H__
#define __GFCLC_UTIL_H__

#include <time.h>

// Function to check if a file exists
int file_exists(const char *filename);

time_t get_file_mtime(const char *path);

#endif // __GFCLC_UTIL_H__