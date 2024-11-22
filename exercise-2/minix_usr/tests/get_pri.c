#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lib.h>

int main(int argc, char *argv[]) {
    message m;
    pid_t pid;
    int priority;

    if (argc < 2) {
        printf("Usage: ./get_pri <PID>\n");
        return -1;
    }

    pid = (pid_t)atoi(argv[1]);
    m.m1_i1 = pid;

    priority = _syscall(MM, GETPRI, &m);

    printf("PID: %d, PRIORITY: %d\n", pid, priority);

    return 0;
}