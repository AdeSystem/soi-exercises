# EXERCISES FOR SOI LABORATORIES 24Z (SYSTEMY OPERACYJNE), WARSAW UNIVERSITY OF TECHNOLOGY
## By Amadeusz Lewandowski

### Exercise I
Implementation of two system calls in Minix System:
- who_most_children - returns PID of process with the most children
- who_most_descendants - returns PID of process with the most descendants (including all levels of child processes)

### Exercise II
Implementation of process priorities in Minix System along with system calls allowing to get a process's priority and set it.
Priority determines the amount of time that is allocated to a process on the processor.

### Exercise III
Implementation of Producer - Consumer problem with semaphores.

Four buffors for four consumers and three producers.
First adds products to the first buffor, second one to the last one and the third one goes through all of them.

### Exercise IV
Implementation of Producer - Consumer problem with monitors.

Four buffors for four consumers and three producers.
First adds products to the first buffor, second one to the last one and the third one goes through all of them.

### Exercise V
Implementation of WORST FIT algorithm for allocating memory for process in Minix System and possibility to choose to between it and FIRST FIT algorithm.

### Exercise VI
Implementation of virutal file system on large binary file, supporting following operations:
- create virtual file system
- delete virtual file system
- copy file to system
- copy file from system
- delete file in system
- show names of files in system
- show system memory map
