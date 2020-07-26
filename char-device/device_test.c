#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define BUFSIZE 1024

int main() {
    char wbuf[BUFSIZE], rbuf[BUFSIZE];
    int n;
    int fd = open("/dev/my-device", O_RDWR);
    strcpy(wbuf, "hello, world!");
    n = write(fd, wbuf, strlen(wbuf));
    printf("write %d bytes\n", n);
    lseek(fd, 0, SEEK_SET);
    n = read(fd, rbuf, BUFSIZE);
    printf("read %d bytes\n", n);
    close(fd);
    printf("read from dev: %s\n", rbuf);
    return 0;
}