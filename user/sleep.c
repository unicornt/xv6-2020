#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
    int t;
    if(argc < 2 || argc > 2) {
        fprintf(2, "usage: sleep time\n");
    }
    else{
        t = atoi(argv[1]);
        sleep(t);
    }
    exit(0);
}