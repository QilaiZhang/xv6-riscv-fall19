#include "kernel/types.h"
#include "user/user.h"

void prime(int fd) {
    int p, num;
    int pid;
    int pipe_fd[2];
    
    // if no number received, exit
    if(read(fd, &p, sizeof(int)) == 0){
        exit();
    }

    // else print the first received number
    printf("prime %d\n",p);

    if(pipe(pipe_fd) < 0){
        fprintf(2, "Pipe Error in process %d!\n",getpid());
        exit();
    }

    pid = fork();
    if(pid > 0){
        // pass forward the other number
        close(pipe_fd[0]);
        while(read(fd, &num, sizeof(int))){
            if(num % p > 0){
                write(pipe_fd[1], &num, sizeof(int));
            }
        }
        close(pipe_fd[1]);
        close(fd);
        wait();
    }else if(pid == 0){
        // the child process recursive call prime()
        close(pipe_fd[1]);
        prime(pipe_fd[0]);
    }else{
        fprintf(2,"Fork Error in process %d!\n",getpid());
    }
    exit();
}

int main (int argc, char *argv[]) {
    int pid;
    int fd[2];

    if(pipe(fd) < 0){
        fprintf(2, "Pipe Error in process %d!\n",getpid());
        exit();
    }

    pid = fork();
    if(pid > 0){
        // main process write numbers between 2 and 35
        close(fd[0]);
        for(int i = 2; i <= 35; i++){
            write(fd[1], &i, sizeof(i));
        }
        close(fd[1]);
        wait();

    }else if(pid == 0){
        // The first child process
        close(fd[1]);
        prime(fd[0]);
    }else {
        fprintf(2,"Fork Error in process %d!\n",getpid());
    }

    exit();
}