#ifndef PV_H
#define PV_H 
#include <sys/syscall.h>
#define sem_p(semid, sem_num) syscall(333, semid, sem_num)
#define sem_v(semid, sem_num) syscall(334, semid, sem_num)
#endif