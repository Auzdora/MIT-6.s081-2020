#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]){

    int pipe_pid[2];
    pipe(pipe_pid);

    int pid = fork();

    if(pid == 0){
        char buf;
        read(pipe_pid[0], &buf, 1);


        if(buf == 'A'){
            int child_pid = getpid();
            printf("%d: received ping\n", child_pid);
            write(pipe_pid[1], "B", 1);
        }

        exit(0);

    }
    else{
        write(pipe_pid[1], "A", 1);
        wait((int *) 0);

        char buf;
        read(pipe_pid[0], &buf, 1);
        if(buf == 'B'){
            int parent_pid = getpid();
            printf("%d: received pong\n", parent_pid);
            
        }

        close(pipe_pid[0]);
        close(pipe_pid[1]);

        exit(0);
    }

}