#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <строка>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pipefd[2];
    pid_t pid;
    char buffer[1024];
    const char *text = argv[1];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(pipefd[1]);

        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            for (ssize_t i = 0; i < n; i++) {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }
            if (write(STDOUT_FILENO, buffer, n) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        if (n == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    } else { 
        close(pipefd[0]); 

        ssize_t len = strlen(text);
        if (write(pipefd[1], text, len) != len) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]); 

        wait(NULL); 
    }

    return 0;
}
