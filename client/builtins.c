/** @author Maya B. */
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>      /* open(), O_CREATE, O_RDWR */
#include <errno.h>

#include "../common/utils.h"
#include "../common/fileHandler.h"
#include "builtins.h"
#include "lineReader.h"

/** @brief Prints a nice prompt.
 * @param user The user name
 * @param host The Server IP
 * @param cwd The current working directory
 */
void print_prompt(char *user, char *host, char *cwd) {
    printColorized(user, 32, 40, 0, 0);
    printf("@");
    printColorized(host, 35, 40, 0, 0);
    printf(":");
    printColorized(cwd, 36, 40, 0, 0);
    printf("$ ");
}

/** @brief Queries a list of files from the server.
 * @note Max 0xFFFF files.
 * @param all See @link { executeBuiltin }
 */
char* list(int localSocket, int *_argc, char *argv0, char **_argv) {
    // Initial exit case
    if (*_argc != 1)
        (WARN("-list: too many arguments.", "list"));

    unsigned char *instruction = malloc(INSTR_SIZE + 1); unsigned char *data = malloc(BUFFER_SIZE + 1);
    C_ALL(instruction, data);

    // 00 OUT : LIST -> Query the server for the list.
    sendData(localSocket, "", 0, "%s", CMD_LIST);
    DEBUG("=> LIST");

    // 01 IN  : FILES <num> -> Get the <num> of files that the server will pipe over.
    C_ALL(instruction, data);
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    long fCount = strtol(instruction + CMD_LEN + 1, NULL, 0);   // Get the encoded hex value.

    // 02 OUT : OK -> Confirm reception of the data.
    C_ALL(instruction, data);
    sendData(localSocket, "", 0, "%s", STATUS_OK);
    DEBUG("=> OK");

    printf("[%ld] Remote Files\n", fCount);
    for (long i = 0; i < fCount; i++) {
        // 03 IN  : FILE <name> -> Get the file name from the server.
        C_ALL(instruction, data);
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);
        printf("  [%ld] => %s\n", i + 1, data);

        // 04 OUT : OK -> Confirm reception
        C_ALL(instruction, data);
        sendData(localSocket, "", 0, "%s", STATUS_OK);
        DEBUG("=> OK");
    }

    // Memory cleanup.
    C_ALL(instruction, data);
    free(instruction); free(data);
    return "builtin list";
}

/** @brief Does the whole handshake to upload a file to the server, allows renaming it, overwriting it,
 *      handles concurrent modification with a locking system, etc.
 * @param all See @link { executeBuiltin }
 */
char* upload(int localSocket, int *_argc, char *argv0, char **_argv) {
    // Initial exit case
    if (*_argc != 2 && *_argc != 3)
        (WARN("-upload: invalid amount of arguments.", "ul <file> [<file new name>]"));

    char *localFilePath = malloc(FILENAME_MAX + 1);
     memset(localFilePath, 0, FILENAME_MAX + 1);

    // Get the local file path, relevant to from where the executable was called from (executable_dir/files/)
    snprintf(localFilePath, FILENAME_MAX, "%s/%s", getFilesFolder(argv0), _argv[1]);
    int flag = 0;
    for (int i = 0; i < strlen(localFilePath) - 1; i++)
        if (localFilePath[i] == '.' && localFilePath[i + 1] == '.') {
            flag = 1;
            break;
        }
    if (flag == 1) {
        free(localFilePath);
        (WARN("-upload: hacky argument '..'.", "ul <file> [<file new name>]"));
    }

    unsigned char *instruction = malloc(INSTR_SIZE + 1); unsigned char *data = malloc(BUFFER_SIZE + 1);
    C_ALL(instruction, data);

    int renameMode = (*_argc == 3) ? 1 : 0;
    ull fileSize = getLength(localFilePath);

    if (fileSize > 0)   // The file exists.
        printf("Transfer of %s (%lld bytes)\n", _argv[1], fileSize);
    else
        (WARN("-upload: specified file does not exist.", "ul <file> [<file new name>]"));

    // 00 OUT : UPLOAD -> Inform the server the client wishes to upload a file.
    sendData(localSocket, "", 0, "%s 0x%.17llx", CMD_UPLOAD, fileSize);
    DEBUG("=> UPLOAD");
    
    // 00.1 IN : OK -> Receive confirmation from the server.
    C_ALL(instruction, data);
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    // 01 OUT : NAME <value> -> Send the name of the file to the server.s
    C_ALL(instruction, data);
    snprintf(data, BUFFER_SIZE + 1, "%s", (renameMode == 1) ? _argv[2] : _argv[1]);
    int len = strlen((renameMode == 1) ? _argv[2] : _argv[1]);
    sendData(localSocket, data, len, "%s", CMD_NAME);
    DEBUG("=> NAME");

    // 02|05|06 IN  : [OVERWRITE|ERROR|OK] -> Get the ongoing status : Does the file exist (ie. query user on overwrite), or is it unopenable.
    C_ALL(instruction, data);
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    // Query user on overwrite.
    if (strncmp(instruction, CMD_OVERWRITE, CMD_LEN) == 0) {
        printf("`%s` already exists on the server. OVERWRITE ? [Y/n] > ", (renameMode == 1) ? _argv[2] : _argv[1]);
        char *result;
        int change = -1;
        while (change == -1) { // Let the user be able  to screw up this little part.
            result = getLine();
            if (strlen(result) == 1 && (strncmp(result, "Y", 1) == 0 || strncmp(result, "y", 1) == 0))
                change = 1;
            else if (strlen(result) == 1 && (strncmp(result, "N", 1) == 0 || strncmp(result, "n", 1) == 0))
                change = 0;
            else
                printf("Invalid answer. OVERWRITE ? [Y/n] > ");
            free(result);
        }

        CINST(instruction);
        if (change == 0) {
            // 03 OUT : ABORT -> Inform the server to cease procedure.
            printf("Client chose to ABORT.\n");
            sendData(localSocket, "", 0, "%s", STATUS_ABORT);
            DEBUG("=> ABORT");

            C_ALL(instruction, data);
            free(localFilePath); free(instruction); free(data);
            return "NO OVERWRITE EXIT"; // Terminate this execution.
        } else {
            // 04 OUT : OK -> Inform the user that the file will be overwritten.
            sendData(localSocket, "", 0, "%s", STATUS_OK);
            DEBUG("=> OK");
        }

        // 05|06|10 IN  : [ERROR|OK|RESSOURCE_IN_USE] -> Get wheher the file is unopenable, or is currently used by another process.
        C_ALL(instruction, data);
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);
    }

    // Server notifies that the file is locked.
    if (strncmp(instruction, STATUS_RESINUSE, CMD_LEN) == 0) {
        printColorized("The file is currently unavailable (java.io.ConcurrentModificationException)", 32, 40, 0, 1);
        free (localFilePath); free(instruction); free(data);
        return "RIU";
    } else if (strncmp(instruction, STATUS_ERR, CMD_LEN) == 0) { // Server notifies that the file is locked.
        printColorized("Could not create the file server side.", 32, 40, 0, 1);
        free (localFilePath); free(instruction); free(data);
        return "ERR";
    } else if (strncmp(instruction, STATUS_OK, CMD_LEN) == 0) {
        // Open the file.
        int fd = open(localFilePath, O_RDONLY, 0600);

        if (fd == -1 || isLocked(localFilePath) == 1) {
            // 07 OUT : ERROR -> Failure point of the process.
            if (isLocked(localFilePath) == 1) errno = EAGAIN;
            else errno = EINVAL;

            perror("FOPEN");
            sendData(localSocket, "", 0, "%s", STATUS_ERR);
            DEBUG("=> ERROR");
        } else {
            lockFile(localFilePath);

             // 08 OUT : OK -> Tell the server to proceed.
            sendData(localSocket, "", 0, "%s", STATUS_OK);
            DEBUG("=> OK");

            // 08.1 IN  : OK -> Get confirmation of proceeding.
            recvData(localSocket, instruction, data);
            ODEBUG("<= %s", instruction);

            pushFile(localSocket, fd);

            // 09 IN  : DONE -> Inform server of completion.
            C_ALL(instruction, data);
            recvData(localSocket, instruction, data);
            ODEBUG("<= %s", instruction);

            printf("\033[38;2;0;255;0m%s %s %s\033[0m\n", "Transfer of", _argv[1], "finished.");
            unlockFile(localFilePath);
        }
        close(fd);  // Cleanup.
    }

    // Memory cleanup.
    C_ALL(instruction, data);
    free(localFilePath); free(instruction); free(data);
    return "builtin upload";
}

/** @brief Does the whole handshake to download a file from the server, allows overwriting the local copy,
 *      handles concurrent modification with a locking system, etc.
 * @param all See @link { executeBuiltin }
 */
char* download(int localSocket, int *_argc, char *argv0, char **_argv) {
    // Initial exit case
    if (*_argc != 2)
        (WARN("-download: invalid amount of arguments.", "ul <file> [<file new name>]"));

    char *localFilePath = malloc(FILENAME_MAX + 1);
    memset(localFilePath, 0, FILENAME_MAX + 1);
    snprintf(localFilePath, FILENAME_MAX, "%s/%s", getFilesFolder(argv0), _argv[1]);

    int flag = 0;
    for (int i = 0; i < strlen(localFilePath) - 1; i++)
        if (localFilePath[i] == '.' && localFilePath[i + 1] == '.') {
            flag = 1;
            break;
        }
    if (flag == 1) {
        free(localFilePath);
        (WARN("-download: hacky argument '..'.", "dl <file>"));
    }

    unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
    C_ALL(instruction, data);

    // 00 OUT : DOWNLOAD -> Inform the server that the client wishes to download a file.
    sendData(localSocket, "", 0, "%s", CMD_DOWNLOAD);
    DEBUG("=> DOWNLOAD");

    // 00.1 IN : OK -> Get confirmation from the server.
    C_ALL(instruction, data);
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    // 01 OUT : NAME <value> -> Transmit the filename
    C_ALL(instruction, data);
    snprintf(data, BUFFER_SIZE + 1, "%s", _argv[1]);
    sendData(localSocket, data, strlen(_argv[1]), "%s", CMD_NAME);
    DEBUG("=> NAME");

    // 02|03|04|05 IN  : [ERROR|RESOURCE_IN_SOURCE|DENY|OK <size>] -> Either get a blocking status, or an OK with the filesize.
    C_ALL(instruction, data);
    recvData(localSocket, instruction, data);
    ODEBUG("<= %s", instruction);

    if (strncmp(instruction, STATUS_RESINUSE, CMD_LEN) == 0) {
        printColorized("The file is currently unavailable.", 32, 40, 0, 1);
        return "RIU";
    } else if (strncmp(instruction, STATUS_ERR, CMD_LEN) == 0) {
        printColorized("The file does not exist on the server.", 32, 40, 0, 1);
        return "ERR";
    } else if (strncmp(instruction, STATUS_DENY, CMD_LEN) == 0) {
        printColorized("The file cannot be read on the server.", 32, 40, 0, 1);
        return "DENY";
    } else {
        // Check file exists, prompt override / abort
        ull fileSize = strtoull(instruction + CMD_LEN + 1, NULL, 0);
        ODEBUG("FSIZE %llu", fileSize);

        // Should the client overwrite his local copy.
        if (isValidPath(localFilePath) == 1) {
            printf("`%s` already exists locally. OVERWRITE ? [Y/n] > ", _argv[1]);
            char *result;
            int change = -1;
            while (change == -1) {
                result = getLine();
                if (strlen(result) == 1 && (strncmp(result, "Y", 1) == 0 || strncmp(result, "y", 1) == 0))
                    change = 1;
                else if (strlen(result) == 1 && (strncmp(result, "N", 1) == 0 || strncmp(result, "n", 1) == 0))
                    change = 0;
                else
                    printf("Invalid answer. OVERWRITE ? [Y/n] > ");
                free(result);
            }

            if (change == 0) {
                // 06 OUT : ABORT -> Inform the server that the client chose to abort.
                printf("Client chose to ABORT.\n");
                sendData(localSocket, "", 0, "%s", STATUS_ABORT);
                DEBUG("=> ABORT");

                C_ALL(instruction, data);
                free(localFilePath); free(instruction); free(data);
                return "NO OVERWRITE EXIT";
            }
        }

        C_ALL(instruction, data);
        remove(localFilePath);  // Delete the local copy ! Danger, no backups are made.

        int fd = open(localFilePath, O_CREAT | O_RDWR, 0600);
        if (fd == -1 || isLocked(localFilePath) == 1) {
            // 07 OUT : DENY -> Inform the server that the client cannot edit his copy.
            printf("File Innacessible.\n");
            sendData(localSocket, "", 0, "%s", STATUS_DENY);
            DEBUG("=> DENY");

            C_ALL(instruction, data);
            free(localFilePath); free(instruction); free(data);
            return "FILE INNACESSIBLE";
        } else {
            lockFile(localFilePath);

            // 08 OUT : OK -> Send readiness confirmation to the server.
            sendData(localSocket, "", 0, "%s", STATUS_OK);
            DEBUG("=> OK");

            pullFile(localSocket, fd, fileSize);

            // 09 OUT : DONE -> Inform the server the upload process is done.
            C_ALL(instruction, data);
            sendData(localSocket, "", 0, "%s", STATUS_DONE);
            DEBUG("=> DONE");
            printf("\033[38;2;0;255;0m%s %s %s\033[0m\n", "Transfer of", _argv[1], "finished.");

            unlockFile(localFilePath);
        }
        close(fd);
    }

    // Memory cleanup.
    C_ALL(instruction, data); memset(localFilePath, 0, FILENAME_MAX + 1);
    free(instruction); free(data); free(localFilePath);

    return "builtin download";
}

/** @brief Deletes a file locally or distantly, depending on the options.
 * @param all See @link { executeBuiltin }
 */
char *deleteFile(int localSocket, int *_argc, char *argv0, char **_argv) {
    // Initial exit case
    if (*_argc != 3)
        (WARN("-delete: invalid amount of arguments.", "del (l: local|d: distant) <file name>"));

    if (strncmp(_argv[1], "l", 1) == 0 || strncmp(_argv[1], "L", 1) == 0) { // Local deletion.
        char *localFilePath = malloc(FILENAME_MAX + 1);
        memset(localFilePath, 0, FILENAME_MAX + 1);

        snprintf(localFilePath, FILENAME_MAX, "%s/%s", getFilesFolder(argv0), _argv[2]);

        if (isValidPath(localFilePath) == 1) {  // Edge cases.
            if (remove(localFilePath) == -1) {
                printColorized("`", 32, 40, 0, 0); printColorized(_argv[2], 32, 40, 0, 0); printColorized("`", 32, 40, 0, 0); printColorized(" could not be deleted.", 32, 40, 0, 1);
            } else {
                printColorized("`", 32, 40, 0, 0); printColorized(_argv[2], 32, 40, 0, 0); printColorized("`", 32, 40, 0, 0); printColorized(" successfully deleted.", 32, 40, 0, 1);
            }
        } else {
            printColorized("`", 32, 40, 0, 0); printColorized(_argv[2], 32, 40, 0, 0); printColorized("`", 32, 40, 0, 0); printColorized(" does not exist locally.", 32, 40, 0, 1);
        }

        free(localFilePath);
    } else if (strncmp(_argv[1], "d", 1) == 0 || strncmp(_argv[1], "D", 1) == 0) {  // Distant deletion.
        unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
        C_ALL(instruction, data);

        // 00 OUT : DELETE -> Inform the server that the client wants to delete a file.
        sendData(localSocket, "", 0, "%s", CMD_DELETE);
        DEBUG("=> DELETE");

        // 00.1 IN  : OK -> Receive servers confirmation.
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);

        // 01 OUT : NAME <value> -> Send the filename to the server.
        C_ALL(instruction, data);
        snprintf(data, BUFFER_SIZE + 1, "%s", _argv[2]);
        sendData(localSocket, data, strlen(data), "%s", CMD_NAME);
        DEBUG("=> NAME");

        // [02|03|04|05] IN  : [ERROR|RIU|DENY|OK] -> Get status clauses or confirmation.
        C_ALL(instruction, data);
        recvData(localSocket, instruction, data);
        ODEBUG("<= %s", instruction);

        if (strncmp(instruction, STATUS_ERR, CMD_LEN) == 0)
            printf("\033[38;2;255;0;0m`%s` does not exist on the server.\033[0m\n", _argv[2]);
        else if (strncmp(instruction, STATUS_RESINUSE, CMD_LEN) == 0)
            printf("\033[38;2;255;0;0m`%s` is currently in use on the server.\033[0m\n", _argv[2]);
        else if (strncmp(instruction, STATUS_DENY, CMD_LEN) == 0)
            printf("\033[38;2;255;0;0m`%s` could not be deleted from the server.\033[0m\n", _argv[2]);
        else
            printf("\033[38;2;0;255;0m`%s` successfully deleted from the server.\033[0m\n", _argv[2]);

        // Memory Cleanup
        C_ALL(instruction, data);
        free(instruction); free(data);
    } else
        (WARN("-delete: Invalid locale specifier.", "del (l: local|d: distant) <file name>"));

    return "DELETE FILE";
}

/** @brief Queries the system for help.
 * @param all See @link { executeBuiltin }
 */
char* getHelp(int localSocket, int *_argc, char *argv0, char **_argv) {
    printf("Built-ins :\n- help\n- list\n- ul <file> [<file new name>]\n- dl <file>\n- del (l: local|d: distant) <file name>\n");
    return "HELP";
}

/** Define the functions */
char* (*builtin[]) (int, int *, char*, char **) = {
    &list,
    &upload,
    &download,
    &deleteFile,
    &getHelp
};

/** Define the builtins */
char *builtins[] = {
    "list",
    "ul",
    "dl",
    "del",
    "help"
};

/** @brief How intuitively well-named can a function be ?
 * @param _argv The arguments vector.
 * @return NULL if non existing else the error or status.
 */
char* executeBuiltin(int localSocket, int *_argc, char *argv0, char **_argv) {
    for (int i = 0; i < BUILTINS_COUNT; i++)
        if (strcmp(_argv[0], builtins[i]) == 0)
            return (*builtin[i])(localSocket, _argc, argv0, _argv);
    return NULL;
}