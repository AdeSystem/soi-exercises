#include <stdio.h>
#include <lib.h>
int main(void) {
    message m;

    int ret = _syscall(MM, WHO_MOST_CHILDREN, &m);
    printf("MOST CHILDREN PID: %d\n", m.m2_i1);
    printf("MOST CHILDREN: %d\n", ret);
    return 0;
}