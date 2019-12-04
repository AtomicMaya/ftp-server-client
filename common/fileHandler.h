/** @author Maya B. */
#ifndef HEADER_COMMON_FILES
#define HEADER_COMMON_FILES

#include "utils.h"

int isValidPath(const char *path);

ull getLength(const char *path);

void getFiles(const char *path, char *files[FILENAME_MAX + 1], int *numberOfFiles);

int lockFile(const char *path);

int unlockFile(const char *path);

int isLocked(const char *path);

#endif