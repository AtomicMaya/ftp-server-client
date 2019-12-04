/** @author Maya B. */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>  /* open, O_CREAT, O_RDWR */

#include "builtins.h"
#include "../common/utils.h"
#include "../common/fileHandler.h"

/** @brief Respond to the client query.
 *  @param localSocket The client socket.
 *  @param argv0 The first argument from the argument vector.
 */
void queryList(int localSocket, char *argv0) {
    char *instruction = malloc(INSTR_SIZE + 1); char *data = malloc(BUFFER_SIZE + 1);
    char *files[FILENAME_MAX];
    int *fileCount = malloc(sizeof(int));

    C_ALL(instruction, data);

    // Get the files stored locally.
    getFiles(getFilesFolder(argv0), files, fileCount);

    // 01 OUT : FILES <num>
    sendData(localSocket, "", 0, "%s 0x%.4x", CMD_FILES, *fileCount);
    DEBUG("=> FILES");

    // 02 IN  : OK
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s\n", instruction);

    for (int i = 0; i < *fileCount; i++) {
        ODEBUG("OBJECT %i: %s", i, files[i]);
        usleep(1);
        // 03 OUT : FILE <name>
        C_ALL(instruction, data);
        snprintf(data, BUFFER_SIZE + 1, "%s", files[i]);
        sendData(localSocket, data, strlen(files[i]), "%s 0x%.4x", CMD_FILE, i + 1);
        DEBUG("=> FILE");

        // 04 IN  : OK
        C_ALL(instruction, data);
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);
    }

    C_ALL(instruction, data);
    for (int i = 0; i < *fileCount; i++)
        files[i] = realloc(files[i], 0);

    free(fileCount);
    free(instruction); free(data);
}

/** @brief Respond to the client query.
 *  @param localSocket The client socket.
 *  @param argv0 The first argument from the argument vector.
 *  @param init The initial command.
 */
void receiveUpload(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]) {
    ull fileSize = strtoull(init + CMD_LEN + 1, NULL, 0);
    printf("FILE SIZE %llu\n", fileSize);

    unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
    char *localFilePath = malloc(FILENAME_MAX + 1);
    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);
    
    // 00.1 OUT : OK
    usleep(1);
    sendData(localSocket, "", 0, "%s", STATUS_OK);
    DEBUG("=> OK");

    // 01 IN  : NAME <value>
    int len = recvData(localSocket, instruction, data);
    snprintf(localFilePath, FILENAME_MAX + 1, "%s/%s", getFilesFolder(argv0), data);
    ODEBUG("<= %s", instruction);

    C_ALL(instruction, data);

    // Determine if file should be overwritten, ie. if it exists locally.
    if (isValidPath(localFilePath) == 1) {
        // 02 OUT : OVERWRITE
        sendData(localSocket, "", 0, "%s", CMD_OVERWRITE);
        DEBUG("=> OVERWRITE");

        // 03|04 IN  : [ABORT|OK]
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);

        if (strncmp(instruction, STATUS_ABORT, CMD_LEN) == 0) {
            free(localFilePath); free(instruction); free(data);
            return;
        } else
            remove(localFilePath);
    }

    // If the file is not being used by another client (upload or download), because data corruption.
    if (isLocked(localFilePath) == 0) {
        lockFile(localFilePath);
        int fd = open(localFilePath, O_CREAT | O_RDWR, 0600);

        if (fd == -1) { // If the file could not be created or opened.
            // 05 OUT : ERROR
            perror("FOPEN");
            sendData(localSocket, "", 0, "%s", STATUS_ERR);
            DEBUG("=> ERROR");
        } else {
            // 06 OUT : OK
            sendData(localSocket, "", 0, "%s", STATUS_OK);
            DEBUG("=> OK");

            // 07|08 IN  : [ERROR|OK]
            recvData(localSocket, instruction, data);
            ODEBUG("<= %s", instruction);
            if (strncmp(instruction, STATUS_ERR, CMD_LEN) == 0)
                printf("CLIENT COULD NOT OPEN FILE, ABORT\n");
            else {
                // 08.1 OUT : OK
                sendData(localSocket, "", 0, "%s", STATUS_OK);
                DEBUG("=> OK");
                pullFile(localSocket, fd, fileSize);
            }
        }
        // 09 OUT : DONE
        C_ALL(instruction, data);
        sendData(localSocket, "", 0, "%s", STATUS_DONE);
        DEBUG("=> DONE");

        close(fd);
        unlockFile(localFilePath);
    } else {
        // 10 OUT : RESSOURCE_IN_USE
        sendData(localSocket, "", 0, "%s", STATUS_RESINUSE);
        DEBUG("=> RIU");
    }

    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);
    free(instruction); free(data); free(localFilePath);
}

/** @brief Respond to the client query.
 *  @param localSocket The client socket.
 *  @param argv0 The first argument from the argument vector.
 *  @param init The initial command.
 */
void pushDownload(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]) {
    unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
    char *localFilePath = malloc(FILENAME_MAX + 1);
    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);

    // 00.1 OUT : OK
    usleep(1);
    sendData(localSocket, "", 0, "%s", STATUS_OK);
    DEBUG("=> OK");

    // 01 IN  : NAME <value>
    C_ALL(instruction, data);
    int len = recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    snprintf(localFilePath, FILENAME_MAX + 1, "%s/%s", getFilesFolder(argv0), data);

    C_ALL(instruction, data);
    if (isValidPath(localFilePath) == 0) {
        // 02 OUT : ERROR
        sendData(localSocket, "", 0, "%s", STATUS_ERR);
        DEBUG("=> RIU");
    } else if (isLocked(localFilePath) == 1) {
        // 03 OUT : RESOURCE_IN_USE
        sendData(localSocket, "", 0, "%s", STATUS_RESINUSE);
        DEBUG("=> RIU");
    } else {
        int fd = open(localFilePath, O_CREAT | O_RDWR, 0600);
        C_ALL(instruction, data);
        if (fd == -1) {
            // 04 OUT : DENY
            sendData(localSocket, "", 0, "%s", STATUS_DENY);
            DEBUG("=> DENY");
        } else {
            // 05 OUT : OK <size>
            ull fileSize = getLength(localFilePath);
            sendData(localSocket, "", 0, "%s 0x%.17llx", STATUS_OK, fileSize);
            ODEBUG("=> %s", instruction);

            // [06|07|08] IN  : [ABORT|DENY|OK]
            C_ALL(instruction, data);
            recvData(localSocket, instruction, data);
            ODEBUG("<= %s", instruction);

            if (strncmp(instruction, STATUS_ABORT, CMD_LEN) == 0) {
                printf("CLIENT CHOSE TO ABORT\n");
            } else if (strncmp(instruction, STATUS_DENY, CMD_LEN) == 0) {
                printf("CLIENT COULD NOT OPEN FILE\n");
            } else {
                lockFile(localFilePath);
                DEBUG("PUSH");
                pushFile(localSocket, fd);

                // 09 IN  : DONE
                C_ALL(instruction, data);
                recvData(localSocket, instruction, data);
                ODEBUG("<= %s", instruction);
                unlockFile(localFilePath);
            }
        }
        close(fd);
    }

    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);
    free(instruction); free(data); free(localFilePath);
}

/** @brief Respond to the client query.
 *  @param localSocket The client socket.
 *  @param argv0 The first argument from the argument vector.
 *  @param init The initial command.
 */
void deleteFile(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]) {
    unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
    char *localFilePath = malloc(FILENAME_MAX + 1);
    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);

    // 00.1 OUT : OK
    sendData(localSocket, "", 0, "%s", STATUS_OK);
    DEBUG("=> OK");

    // 01 IN  : NAME <value>
    recvData(localSocket, instruction, data);
    ODEBUG("<= NAME %s", data);

    snprintf(localFilePath, FILENAME_MAX, "%s/%s", getFilesFolder(argv0), data);

    C_ALL(instruction, data);
    if (isValidPath(localFilePath) == 0)
        // 02 OUT : ERROR
        snprintf(instruction, INSTR_SIZE + 1, "%s", STATUS_ERR);
    else if (isLocked(localFilePath) == 1)
        // 03 OUT : RIU
        snprintf(instruction, INSTR_SIZE + 1, "%s", STATUS_RESINUSE);
    else {
        if (remove(localFilePath) == -1)
            // 04 OUT : DENY
            snprintf(instruction, INSTR_SIZE + 1, "%s", STATUS_DENY);
        else
            // 05 OUT : OK
            snprintf(instruction, INSTR_SIZE + 1, "%s", STATUS_OK);
    }

    // [02|03|04|06] OUT : [ERROR|RIU|DENY|OK]
    sendData(localSocket, "", 0, "%s", instruction);

    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);
    free(instruction); free(data); free(localFilePath);
}