#include <stdio.h>
#include <lib.h>
int main(void) {
    message m;
    int ret;
    pid_t excluded_pid = 1;
    m.m1_i1 = excluded_pid;
    ret = _syscall(MM, WHO_MOST_DESCENDANTS, &m);
    printf("MOST DESCENDANTS PID: %d\n", m.m2_i1);
    printf("MOST DESCENDANTS: %d\n", ret);
    return 0;
}