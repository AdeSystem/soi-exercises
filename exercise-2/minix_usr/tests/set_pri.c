#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lib.h>

int main(int argc, char *argv[]) {
    message m;
    pid_t pid;
    int priority;

    if (argc < 3) {
        printf("Usage: ./set_pri <PID> <PRIORITY>\n");
        return -1;
    }

    pid = (pid_t)atoi(argv[1]);
    priority = atoi(argv[2]);

    m.m1_i1 = pid;
    m.m1_i2 = priority;

    _syscall(MM, SETPRI, &m);

    return 0;
}