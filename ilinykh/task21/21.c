#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

volatile sig_atomic_t sigint_count = 0;

void sigint_handler(int signum) {
    write(STDOUT_FILENO, "\a", 1);
    sigint_count++;
}

void sigquit_handler(int signum) {
    char buffer[100];
    int len = snprintf(buffer, sizeof(buffer), "Сигнал прозвучал %d раз(а).\n", sigint_count);
    write(STDOUT_FILENO, buffer, len);
    _exit(0);
}

int main() {
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &t) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    
    t.c_lflag &= ~(ECHOCTL); 

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    struct sigaction sa_int, sa_quit;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    sigaction(SIGQUIT, &sa_quit, NULL);

    while(1) {
        pause();
    }

    return 0;
}
