#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    int wait_status;
    pid_t pid = fork();

    if (pid < 0) {  
        perror("fork failed");
        return 1;
    } else if (pid == 0) { 
        printf("PID=%ld (child)\n", (long)getpid());
        execvp(argv[1], &argv[1]);
        perror("execlp failed");
        exit(1);
    } else {  
        printf("PID=%ld (parent)\n", (long)getpid());
        
        wait(&wait_status);
        if (WIFEXITED(status)) {
            printf("Child exited with status: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Terminated by signal: %d\n", WTERMSIG(status));
        }
        

        printf("success\n");
    }

    return 0;
}

