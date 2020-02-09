#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
// #include <ncurses.h>

#define MAXLINE 64

unsigned long mem_total;
int curr_max_pid;
int PID_MAX;

int display_mem();
int display_load();
void display_uptime();
void display_task(char *s_pid);
void format_size(unsigned long l_size, char *s_size);
void format_command(char *src, char *dest);
void format_time(unsigned long l_time, char *s_time);

int main() {
    chdir("/proc");
    /*
    initscr ();
    curs_set (0);
    while (1) {
        int curr_max_pid = display_load();
        display_uptime();
        sleep(1);
        refresh();
    }
    endwin();
    */
    char buf[BUFSIZ];
    FILE *fp = fopen("/proc/sys/kernel/pid_max", "r");
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    sscanf(buf, "%d", &PID_MAX);
    unsigned long time_recode[PID_MAX];

    mem_total = display_mem();
    curr_max_pid = display_load();
    display_uptime();
    struct stat stat_buf;
    char s_pid[MAXLINE];
    printf("  PID      USER  PRI   NI  VIRT   RES   SHR  S  MEM%%      TIME+  Command\n");
    for (int pid = 0; pid <= curr_max_pid; ++pid) {
        sprintf(s_pid, "%d", pid);
        if (stat(s_pid, &stat_buf) < 0) {
            continue;
        }
        display_task(s_pid);
    }
    exit(0);
}


int display_mem() {
    char buf[BUFSIZ], line[MAXLINE];
    unsigned long mem_total, mem_used, mem_free, mem_shared, mem_buff, mem_cache, mem_available;
    unsigned long swap_total, swap_used, swap_free;
    FILE *fp = fopen("meminfo", "r");
    fgets(buf, BUFSIZ, fp);  // MemTotal
    sscanf(buf, "%s%lu", line, &mem_total);
    fgets(buf, BUFSIZ, fp);  // MemFree
    sscanf(buf, "%s%lu", line, &mem_free);
    fgets(buf, BUFSIZ, fp);  // MemAvailable
    sscanf(buf, "%s%lu", line, &mem_available);
    fgets(buf, BUFSIZ, fp);  // Buffers
    sscanf(buf, "%s%lu", line, &mem_buff);
    fgets(buf, BUFSIZ, fp);  // Cached
    sscanf(buf, "%s%lu", line, &mem_cache);
    fgets(buf, BUFSIZ, fp);  // SwapCached
    fgets(buf, BUFSIZ, fp);  // Active
    fgets(buf, BUFSIZ, fp);  // Inactive
    fgets(buf, BUFSIZ, fp);  // Active(anon)
    fgets(buf, BUFSIZ, fp);  // Inactive(anon)
    fgets(buf, BUFSIZ, fp);  // Active(file)
    fgets(buf, BUFSIZ, fp);  // Inactive(file)
    fgets(buf, BUFSIZ, fp);  // Unevictable
    fgets(buf, BUFSIZ, fp);  // Mlocked
    fgets(buf, BUFSIZ, fp);  // SwapTotal
    sscanf(buf, "%s%lu", line, &swap_total);
    fgets(buf, BUFSIZ, fp);  // SwapFree
    sscanf(buf, "%s%lu", line, &swap_free);
    fgets(buf, BUFSIZ, fp);  // Dirty
    fgets(buf, BUFSIZ, fp);  // Writeback
    fgets(buf, BUFSIZ, fp);  // AnonPages
    fgets(buf, BUFSIZ, fp);  // Mapped
    fgets(buf, BUFSIZ, fp);  // Shmem
    sscanf(buf, "%s%lu", line, &mem_shared);
    fclose(fp);

    mem_used = mem_total - mem_free - mem_buff - mem_cache;
    swap_used = swap_total - swap_free;
    printf("           Total        Used        Free      Shared  Buff/Cache   Available\n");
    printf("Mem:%11luK%11luK%11luK%11luK%11luK%11luK\n", 
    mem_total, mem_used, mem_free, mem_shared, mem_buff+mem_cache, mem_available);
    printf("Swp:%11luK%11luK%11luK\n", swap_total, swap_used, swap_free);
    return mem_total;
}


int display_load() {
    char buf[BUFSIZ];
    float load_1, load_5, load_15;
    char tasks[MAXLINE];
    int max_pid;
    FILE *fp = fopen("loadavg", "r");
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    sscanf(buf, "%f %f %f %s %d", &load_1, &load_5, &load_15, tasks, &max_pid);
    printf("Tasks: %s\n", tasks);
    printf("Load average: %.2f %.2f %.2f\n", load_1, load_5, load_15);
    // mvprintw(0, 0, "Tasks: %s\n", tasks);
    // mvprintw(1, 0, "Load average: %.2f %.2f %.2f\n", load_1, load_5, load_15);
    return max_pid;
}


void display_uptime() {
    char buf[BUFSIZ];
    int uptime, hs, ms, ss;
    FILE *fp = fopen("uptime", "r");
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    sscanf(buf, "%d", &uptime);
    hs = uptime/3600;
    ms = (uptime%3600)/60;
    ss = uptime%60;
    // mvprintw(2, 0, "Uptime: %2d:%02d:%02d\n", hs, ms, ss);
    printf("Uptime: %2d:%02d:%02d\n", hs, ms, ss);
}


void display_task(char *s_pid) {
    char buf[BUFSIZ], line[MAXLINE];
    int uid;
    chdir(s_pid);
    FILE *fp = fopen("status", "r");
    for (int i = 0; i < 9; ++i) {
        fgets(buf, BUFSIZ, fp);
    }
    fclose(fp);
    sscanf(buf, "%s%d", line, &uid);

    int pid, ppid, pgrp, session, tty_nr, tpgid;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, priority, nice, num_threads; 
    unsigned int flags;
    char state, comm[MAXLINE], time[MAXLINE];
    fp = fopen("stat", "r");
    fgets(buf, BUFSIZ, fp);
    sscanf(buf, "%d%s %c%d%d%d%d%d%u%lu%lu%lu%lu%lu%lu%ld%ld%ld%ld%ld",
    &pid, comm, &state, 
    &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, 
    &minflt, &cminflt, &majflt, &cmajflt, 
    &utime, &stime, &cutime, &cstime, 
    &priority, &nice, 
    &num_threads);
    fclose(fp);
    format_time(utime+stime, time);

    unsigned long virt, res, shr;
    char s_virt[MAXLINE], s_res[MAXLINE], s_shr[MAXLINE];
    fp = fopen("statm", "r");
    fgets(buf, BUFSIZ, fp);
    sscanf(buf, "%lu%lu%lu", &virt, &res, &shr);
    fclose(fp);
    virt <<= 2;
    res <<= 2;
    shr <<= 2;
    format_size(virt, s_virt);
    format_size(res, s_res);
    format_size(shr, s_shr);

    char command[MAXLINE];
    fp = fopen("cmdline", "r");
    memset(buf, 0, BUFSIZ);
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    format_command(buf, command);
    
    double mem_per = (double)res/mem_total*100;
    if (strlen(command) == 0) {
        strcpy(command, comm);
    }
    printf("%5d%10s%5ld%5ld%6s%6s%6s%3c%6.1f%12s %s\n", 
    pid, getpwuid(uid)->pw_name, 
    priority, nice, 
    s_virt, s_res, s_shr,
    state,
    mem_per,
    time,
    command);
    chdir("..");
}


void format_time(unsigned long l_time, char *s_time) {
    unsigned long m;
    float s;
    m = l_time/60000;
    s = (float)l_time/1000 - m*60;
    sprintf(s_time, "%ld:%05.2f", m, s);
}


void format_size(unsigned long l_size, char *s_size) {
    if (l_size < 10000) {
        if (l_size == 0) {
            strcpy(s_size, "0");
        } else {
            sprintf(s_size, "%luK", l_size);
        }
    } else {
        l_size >>= 10;
        if (l_size < 10000) {
            sprintf(s_size, "%luM", l_size);
        } else {
            l_size >>= 10;
            sprintf(s_size, "%luG", l_size);
        }
    }
}


void format_command(char *src, char *dest) {
    int cnt = 0, i = 0;
    while (cnt != 2 && i < MAXLINE) {
        if (*src == 0) {
            ++cnt;
            *dest++ = ' '; 
        } else {
            cnt = 0;
            *dest++ = *src++;
        }
        ++i;
    }
    *(dest-2) = 0;
}
