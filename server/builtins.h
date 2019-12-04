/** @author Maya B. */
#ifndef HEADER_SERVER_BUILTINS
#define HEADER_SERVER_BUILTINS

#include "../common/utils.h"

void queryList(int localSocket, char *argv0);

void receiveUpload(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]);

void pushDownload(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]);

void deleteFile(int localSocket, char *argv0, unsigned char init[INSTR_SIZE]);

#endif