#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lib.h>

int main(void) {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        printf("First child process started, PID: %d\n", getpid());
        while (1) {}
    } else {
        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            printf("Second child process started, PID: %d\n", getpid());
            while (1) {}
        } else {
            printf("Parent process finished, PID: %d\n", getpid());
        }
    }

    return 0;
}
