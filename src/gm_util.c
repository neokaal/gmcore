#include <stdio.h>
#include <sys/stat.h>

#include "gm_util.h"

int file_exists(const char *filename)
{
    FILE *file;
    if ((file = fopen(filename, "r")) != NULL)
    {
        fclose(file);
        return 1; // File exists
    }
    return 0; // File does not exist or cannot be opened
}

time_t get_file_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return st.st_mtime;
}
