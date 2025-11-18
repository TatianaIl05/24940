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

    pid_t pid = fork();

    if (pid < 0) {  
        perror("fork failed");
        return 1;
    } else if (pid == 0) { 
        printf("PID=%ld (child)\n", (long)getpid());
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp failed");
        exit(1);
    } else {  
        printf("PID=%ld (parent)\n", (long)getpid());
        
        wait(NULL);

        printf("success\n");
    }

    return 0;
}
