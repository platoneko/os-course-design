#include "mainwindow.h"

#include <QApplication>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/sem.h>

int srcfd, tgtfd;
int readMu, writeMu;


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: process-demo <source> <target>\n");
        exit(-1);
    }
    if ((srcfd = open(argv[1], O_RDONLY)) < 0) {
        fprintf(stderr, "open %s failed\n", argv[1]);
        exit(-1);
    }
    tgtfd = open(argv[2], O_CREAT | O_WRONLY, 0644);
    int pid[3];
    if ((readMu = semget(IPC_PRIVATE, 1, IPC_CREAT|0600)) == -1) {
        fprintf(stderr, "readMu create failed\n");
        exit(-1);
    }
    if ((writeMu = semget(IPC_PRIVATE, 1, IPC_CREAT|0600)) == -1) {
        fprintf(stderr, "writeMu create failed\n");
        exit(-1);
    }
    semctl(readMu, 0, SETVAL, 1);
    semctl(writeMu, 0, SETVAL, 1);
    if ((pid[0] = fork()) == 0) {
        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        return a.exec();
    } else if ((pid[1] = fork()) == 0) {
        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        return a.exec();
    } else if ((pid[2] = fork()) == 0) {
        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        return a.exec();
    }
    ::close(srcfd);
    ::close(tgtfd);
    waitpid(pid[0], nullptr, 0);
    waitpid(pid[1], nullptr, 0);
    waitpid(pid[2], nullptr, 0);
    semctl(readMu, 0, IPC_RMID);
    semctl(writeMu, 0, IPC_RMID);
    exit(0);
}
