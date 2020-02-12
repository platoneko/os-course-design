#ifndef TASKINFO_H
#define TASKINFO_H

#include <string>

struct TaskInfo {
    int pid;
    int uid;
    int pri;
    int ni;
    unsigned long virt;
    unsigned long res;
    unsigned long shr;
    char s;
    float cpu;
    float mem;
    unsigned long time;
    std::string command;
    std::string comm;
    unsigned char dirty;
    TaskInfo() {}
    TaskInfo(int pid, int uid, int pri, int ni,
             unsigned long virt,
             unsigned long res,
             unsigned long shr,
             char s, float mem,
             unsigned long time,
             std::string command,
             std::string comm):
        pid(pid),
        uid(uid),
        pri(pri),
        ni(ni),
        virt(virt),
        res(res),
        shr(shr),
        s(s),
        cpu(0),
        mem(mem),
        time(time),
        command(command),
        comm(comm),
        dirty(1) {
    }
};
#endif // TASKINFO_H
