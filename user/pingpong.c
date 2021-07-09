#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
    int p1[2], p2[2], pid;
    char g[1];
    if(argc > 1) {
        fprintf(2, "usage: pingpong\n"); 
    }
    else {
        if(pipe(p1) < 0) {
            fprintf(2, "pipe error\n");
            exit(1);
        }
        if(pipe(p2) < 0) {
            fprintf(2, "pipe error\n");
            exit(1);
        }
        if((pid = fork()) < 0) {
            fprintf(2, "fork error\n");
            exit(1);
        }
        if(pid == 0){ // parent progress
            if(read(p1[0], g, 1) > 0) {
                fprintf(1, "%d: received ping\n", getpid());
                write(p2[1], "b", 1);
            }
            close(p2[0]);
            close(p2[1]);
            close(p1[0]);
            close(p1[1]);
        }
        else {
            write(p1[1], "a", 1);
            if(read(p2[0], g, 1) > 0) {
                fprintf(1, "%d: received pong\n", getpid());
            }
            close(p2[0]);
            close(p2[1]);
            close(p1[0]);
            close(p1[1]);
        }
    }
    exit(0);
}