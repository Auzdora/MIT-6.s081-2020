#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void primes(int pipe_id[], int size){

    if(size/sizeof(int) <= 0) return;

    int buf[size/sizeof(int)];
    read(pipe_id[0], buf, size);
    close(pipe_id[0]);
    close(pipe_id[1]);

    int pipes_id[2];
    pipe(pipes_id);

    int cnt=0;
    for(int i=0; i<size/sizeof(int); i++){
        if(buf[i]%buf[0]==0) continue;
        else cnt++;
    }

    if(fork() == 0){
        int next_buf[cnt];
        cnt=0;
        for(int i=0; i<size/sizeof(int); i++){
            if(buf[i]%buf[0]==0){
                continue;
            }
            else{
                next_buf[cnt] = buf[i];
                cnt++;
            }
        }
        write(pipes_id[1], next_buf, sizeof(next_buf));

        exit(0);

    }
    else{
        // wait((int *)0);
        printf("prime %d\n", buf[0]);

        primes(pipes_id, cnt*sizeof(int));
        return;
    }
    



}

int main(){
    int array[34];

    for(int i=0;i<34;i++){
        array[i] = 2+i;
    }

    int pipe_id[2];
    pipe(pipe_id);
    write(pipe_id[1], array, sizeof(array));

    primes(pipe_id, sizeof(array));

    exit(0);
}