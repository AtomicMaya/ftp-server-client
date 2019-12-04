/** @author Maya B. */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#include "builtins.h"
#include "../common/utils.h"
#include "../common/fileHandler.h"
#include "../common/crc.h"

int clientConnected = 1;

/** @brief Handles signals.... 
 * @param signo The signal number.
 */
void signalHandler(int signo) {
    if (signo == SIGINT || signo == SIGTERM || signo == SIGQUIT || signo == SIGHUP) {
        clientConnected = 0;
        usleep(1);
    } else
        printf("WTF\n");
}

/** @brief Program ingress point.
 * @param argc The size of the argument vector.
 * @param argv The argument vector.
*/
int main(int argc, char **argv) {
    // SETUP
    crcInit();
    if (argc != 2)
        FAIL_SUCCESFULLY("Insufficient arguments.");

    int client = atoi(argv[1]);
    DEBUG("START");
    

    struct sigaction action;    // Define signal handlers and sh*zz.
    memset(&action, 0, sizeof(action));
    action.sa_handler = signalHandler;
    sigaction(SIGINT, &action, NULL);

    // END SETUP

    do {
        unsigned char *instruction = malloc(INSTR_SIZE + 1), *data = malloc(BUFFER_SIZE + 1);
        C_ALL(instruction, data);

        int size = recvData(client, instruction, data);
        ODEBUG("INSTR |%s|\n", instruction);

        // HANDLE CLIENT QUERIES
        if (size == 0 && strncmp(instruction, CMD_EXIT, CMD_LEN) == 0) {
            printColorized("The client exited.", 92, 40, 0, 1);
            clientConnected = 0;
        } else if (size == 0 && strncmp(instruction, CMD_LIST, CMD_LEN) == 0) {
            printColorized("The client asked for the file list.", 92, 40, 0, 1);
            queryList(client, argv[0]);
        } else if (size == 0 && strncmp(instruction, CMD_UPLOAD, CMD_LEN) == 0) {
            printColorized("The client wants to upload a file.", 92, 40, 0, 1);
            receiveUpload(client, argv[0], instruction);
        } else if (size == 0 && strncmp(instruction, CMD_DOWNLOAD, CMD_LEN) == 0) {
            printColorized("The client wants to download a file.", 92, 40, 0, 1);
            printf("CLIENT WANTS TO DOWNLOAD\n");
            pushDownload(client, argv[0], instruction);
        } else if (size == 0 && strncmp(instruction, CMD_DELETE, CMD_LEN) == 0) {
            printColorized("The client wants to delete a file.", 92, 40, 0, 1);
            deleteFile(client, argv[0], instruction);
        }

        sleep(1);
        free(instruction); free(data);
    } while (clientConnected == 1);

    close(client);

    printf("\033[38;2;0;255;0m%s\033[0m\n", "Server closing.");
    DEBUG("SERVER INSTANCE STOPPED");
    exit(EXIT_SUCCESS);
}