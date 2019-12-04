/** @author Maya B. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>      /** open */
#include <sys/stat.h>   /** lstat */
#include <dirent.h>     /** DIR*/
#include <errno.h>

#include "utils.h"
#include "fileHandler.h"

/** @brief Checks that the file at PATH exists and is a file.
 * @param path The file's path on the machine.
 */
int isValidPath(const char *path) {
    struct stat* statBuffer = malloc(sizeof(struct stat));
    memset(statBuffer, 0, sizeof(struct stat));
    int status = lstat(path, statBuffer);
    __mode_t mode = statBuffer->st_mode;
    
    free(statBuffer);
    return (status == 0 && S_ISREG(mode)) ? 1 : 0;
}

/** @brief Gets the length of the path.
 * @param path The file's path on the machine.
 */
ull getLength(const char *path) {
    if (isValidPath(path) == 1) {
        ull length;
        struct stat* statBuffer = malloc(sizeof(struct stat));
        memset(statBuffer, 0, sizeof(struct stat));
        int status = lstat(path, statBuffer);
        ssize_t size = statBuffer->st_size;
        free(statBuffer);
        return size;
    } else return 0;
}

/** Gets all of the subfiles/folders of { @param path }
 * @param path          The path of the node
 * @param files         An output array of filenames representing the subfiles/folders at the location described by { @param path }
 * @param numberOfFiles An integer (*ptr) representing the number of files found.
 */
void getFiles(const char *path, char *files[FILENAME_MAX + 1], int *numberOfFiles) {
    int count = 0;  // Count the children (sounds worse than it is).
    int len = 0;
    struct dirent *currentDir;
    struct stat *statBuffer = malloc(sizeof(struct stat));

    // If it's a dir, then it might have children, otherwise... nope.
    if (lstat(path, statBuffer) == 0 && S_ISDIR(statBuffer->st_mode)) {
        DIR *folder = opendir(path); 
        // If the program can access the object at the end of the path.
        if (access(path, F_OK) != -1) {
            if (folder) // Check innulability (yes I just invented a word)
                while ((currentDir = readdir(folder))) {    // If children can be read.
                    len = strlen(currentDir->d_name);
                    if (strcmp(currentDir->d_name, ".") && strcmp(currentDir->d_name, "..") && ((len > 4 && strcmp(currentDir->d_name + len - 5, ".lock")) || len <= 4)) { // And those children don't loop back to self or parent.s
                        files[count] = (char*) malloc(FILENAME_MAX + 1); // Add it to the children's addresses (getting weird looks from the people at the FBI)
                        snprintf(files[count], FILENAME_MAX + 1, "%s", currentDir->d_name);  // Is a basename.
                        count++;
                    }
                }
            free(currentDir);
        }
        closedir(folder);
    } else if (errno < 0)
        printf("ERROR %d : %s\n", errno, strerror(errno));

    *numberOfFiles = count; // Keep track of the number of children.

    free(currentDir); free(statBuffer);
}

/** @brief Creates a lock file (path.lock)
 * @param path The path of the file to be locked.
 */
int lockFile(const char *path) {
    char *lockFilePath = malloc(FILENAME_MAX + 1);
    snprintf(lockFilePath, FILENAME_MAX + 1, "%s.lock", path);    
    
    FILE *fd = fopen(lockFilePath, "w"); 
    fprintf(fd, "%s", "lock");
    fclose(fd);

    ODEBUG("ADDING LOCK FILE [%s]\n", lockFilePath);
    free(lockFilePath);
    return 1;
}

/** @brief Removes the lock file.
 * @param path The path of the file to be unlocked.
 */
int unlockFile(const char *path) {
    int status = -1;
    if (isLocked(path) == 1) {
        char *lockFilePath = malloc(FILENAME_MAX + 1);
        snprintf(lockFilePath, FILENAME_MAX + 1, "%s.lock", path); 
        status = remove(lockFilePath);
        ODEBUG("REMOVING LOCK FILE [%s]\n", lockFilePath);
        free(lockFilePath);
    }
    return (status == 0) ? 1 : 0;
}

/** @brief Check if the lock file exists.
 * @param path The path of the file to be unlocked.
 */
int isLocked(const char *path) {
    char *lockFilePath = malloc(FILENAME_MAX + 1);
    snprintf(lockFilePath, FILENAME_MAX + 1, "%s.lock", path);    
    int len = getLength(lockFilePath);

    ODEBUG("VERIFYING LOCK FILE STATUS [%s] => %s\n", lockFilePath, (len > 0) ? "ACTIVE" : "INACTIVE");
    
    free(lockFilePath);
    return (len > 0) ? 1 : 0;
}