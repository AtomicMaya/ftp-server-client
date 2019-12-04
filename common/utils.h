/** @author Maya B. */
#ifndef HEADER_COMMON_UTILS
#define HEADER_COMMON_UTILS

#include <stdio.h> /*   fprintf     */
#include <unistd.h> /* getpid() */

typedef unsigned long long ull;
typedef signed long long sll;

#define DEBUGMODE       1
#define EXIT_FAILURE    1
#define SERVER_IP       "127.0.0.1"

#define BUFFER_SIZE         512 // Size of a packet
#define INSTR_SIZE          32  // Size of the instruction
#define DATA_OFFSET         (INSTR_SIZE + 4)    // + CRC
#define PACKET_SIZE         (BUFFER_SIZE + DATA_OFFSET)

#define CMD_LEN         4

// Commands to be sent over the wire
#define CMD_EXIT        "EXIT"
#define CMD_UPLOAD      "UPLD"
#define CMD_DOWNLOAD    "DNLD"
#define CMD_BROADCAST   "ALRT"
#define CMD_NAME        "NAME"
#define CMD_LIST        "LIST"
#define CMD_OVERWRITE   "OVWR"
#define CMD_FILES       "FILS"
#define CMD_FILE        "FILE"
#define CMD_DELETE      "DELE"

// Status update codes
#define STATUS_OK       "OKOK"
#define STATUS_DONE     "DONE"
#define STATUS_ERR      "ERRR"
#define STATUS_EMPTY    "EPTY"
#define STATUS_RESINUSE "REIU"
#define STATUS_DENY     "DENY"
#define STATUS_ABORT    "ABRT"

#define SPLIT_PLACES " \t"

/** Prints a nice message and crashes out the application. */
#define FAIL_SUCCESFULLY(msg)       { printColorized(msg, 31, 40, 0, 1); exit(EXIT_FAILURE); }

/** Prints a debug message. */
#define DEBUG(msg) { if((DEBUGMODE) == 1) (ODEBUG("%s", msg)); }

/** Prints a formatted debug message. */
#define ODEBUG(fmt, ...) { if((DEBUGMODE) == 1) { printColorized(("D: "), 95, 40, 0, 0); char *pid = malloc(10); printf("\033[38;2;255;0;0m[%d]\033[0m  |  ", getpid()); char *msg = malloc(256); snprintf(msg, 256, fmt, __VA_ARGS__); printColorized(msg, 95, 40, 0, 1); free(msg); }}

/** Clears the data buffer. */
#define CDATA(data) { memset((data), 0, (BUFFER_SIZE + 1)); }

/** Clears the instruction buffer. */
#define CINST(instruction) { memset((instruction), 0, (INSTR_SIZE + 1)); }

/** Clears all of the buffers. */
#define C_ALL(instruction, data) { CINST((instruction)); CDATA((data)); }

void printColorized(char *string, int ANSI_FGCOLOR, int ANSI_BGCOLOR, int ANSI_DECO, int newLine);

int isNumeric(char* potential);

int isValidIPV4(const char *s);

int sendData(int localSocket, unsigned char data[BUFFER_SIZE], int contentLen, char *fmt, ...);

int recvData(int localSocket, unsigned char instruction[INSTR_SIZE], unsigned char data[BUFFER_SIZE]);

char **splitLine(char *line, int *count, char *tokens);

char *getFilesFolder(char *argv0);

int pushFile(int localSocket, int fd);

int pullFile(int localSocket, int fd, ull fileSize);

#endif