#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

#define BUFSIZE 8192

int src_fd, tgt_fd;
char buf_1[BUFSIZE], buf_2[BUFSIZE];
int read_n_1 = -1, read_n_2 = -1, read_eof = 0;
sem_t r_mtx, w_mtx, w_syn;


void *buf_1_thread() {
    while (1) {
        if (read_eof) {
            return NULL;
        }
        sem_wait(&r_mtx);
        read_n_1 = read(src_fd, buf_1, BUFSIZE);
        if (read_n_1 != BUFSIZE) {
            read_eof = 1;
        }
        sem_wait(&w_syn);
        sem_post(&r_mtx);
        sem_wait(&w_mtx);
        if (read_n_1 > 0) {
            write(tgt_fd, buf_1, read_n_1);
        }
        sem_post(&w_syn);
        sem_post(&w_mtx);
    }
}


void *buf_2_thread() {
    while (1) {
        if (read_eof) {
            return NULL;
        }
        sem_wait(&r_mtx);
        read_n_2 = read(src_fd, buf_2, BUFSIZE);
        if (read_n_2 != BUFSIZE) {
            read_eof = 1;
        }
        sem_wait(&w_syn);
        sem_post(&r_mtx);
        sem_wait(&w_mtx);
        if (read_n_2 > 0) {
            write(tgt_fd, buf_2, read_n_2);
        }
        sem_post(&w_mtx);
        sem_post(&w_syn);
    }
}


int main(int argc, char *argv[]) {
    struct stat sbuf;
    pthread_t tid_1, tid_2;
    if (argc != 3) {
        fprintf(stderr, "usage: %s <source> <target>\n", argv[0]);
        exit(0);
    }
    if (stat(argv[1], &sbuf) < 0) {
        fprintf(stderr, "%s: cannot stat '%s': No such file or directory\n", argv[0], argv[1]);
        exit(0);
    }
    src_fd = open(argv[1], O_RDONLY);
    tgt_fd = open(argv[2], O_CREAT | O_WRONLY, 0644);
    sem_init(&r_mtx, 0, 1);
    sem_init(&w_mtx, 0, 1);
    sem_init(&w_syn, 0, 1);
    pthread_create(&tid_1, NULL, buf_1_thread, NULL);
    pthread_create(&tid_2, NULL, buf_2_thread, NULL);
    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);
    sem_close(&r_mtx);
    sem_close(&w_mtx);
    sem_close(&w_syn);
    close(src_fd);
    close(tgt_fd);
    exit(0);
}