/** @author Maya B. */
#ifndef HEADER_LIGHT_SERVER
#define HEADER_LIGHT_SERVER

#define PORT 4242
#define BACKLOG 10

/** @brief Tries to kill a process at a certain PID, first gently, then less gently, then not.
 * @param pid The pid of the process to be killed.
 */
#define KILL(pid) {\
if (pid != -1)\
    if (kill(pid, SIGKILL) == -1)\
        FAIL_SUCCESFULLY("Immortal process, what's the thing about that ?\n");\
}

/** @brief Forces the parent to wait for the child's death.
 * @param pid The concerned pid.
 */
#define WAIT_FOR_DEATH(pid) {\
int waitStatus;\
do {    /* Wait for job to finish (I think). */\
    waitpid(pid, &waitStatus, WUNTRACED);\
} while (!WIFEXITED(waitStatus) && !WIFSIGNALED(waitStatus));\
}


#endif