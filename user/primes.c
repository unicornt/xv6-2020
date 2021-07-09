#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes() {
    int prime, ret, pid, tmp;
    int p[2];
    tmp = 0;
    ret = read(0, &prime, sizeof(int));
    if(ret < sizeof(int)) {
        exit(0);
    }
    printf("prime %d\n", prime);
    if(pipe(p) < 0){
        fprintf(2, "pipe error\n");
        exit(1);
    }
    if((pid = fork()) < 0) {
        fprintf(2, "fork error\n");
        exit(1);
    }
    if(pid == 0) { // child
        close(0);
        dup(p[0]);
        close(p[1]);
        close(p[0]);
        primes();
        wait(0);
    }
    else {
        close(1);
        dup(p[1]);
        close(p[1]);
        close(p[0]);
        while(read(0, &tmp, sizeof(int)) == sizeof(int)) {
            if(tmp % prime != 0) {
                write(1, &tmp, sizeof(int));
            }
        }
        close(1);
    }
}

int
main(int argc, char **argv)
{
    int p[2], i, pid;
    if(argc > 1) {
        fprintf(2, "usage: primes\n");
        exit(1);
    }
    else {
        if(pipe(p) < 0){
            fprintf(2, "pipe error\n");
            exit(1);
        }
        if((pid = fork()) < 0) {
            fprintf(2, "fork error\n");
        }
        if(pid == 0){
            close(1);
            dup(p[1]);
            close(p[1]);
            close(p[0]);
            for(i = 2; i < 36; i++) {
                write(1, &i, sizeof(int));
            }
        }
        else {
            close(0);
            dup(p[0]);
            close(p[1]);
            close(p[0]);
            primes();
            wait(0);
        }
        wait(0);
    }
    exit(0);
}