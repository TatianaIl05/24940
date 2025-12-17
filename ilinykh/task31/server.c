#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <termio.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define POLL_TIMEOUT 1000

volatile sig_atomic_t keep_running = 1;
int client_count = 0;  


void signal_handler(int sig) {
    if (client_count == 0)
    {
        keep_running = 0;
        printf("\nПолучен сигнал завершения. Завершаю работу...\n");
    }
}

int main() {
    struct termios old_termios, new_termios;
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~ECHOCTL;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    struct pollfd fds[MAX_CLIENTS + 1];  
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int nfds;  
    
    signal(SIGINT, signal_handler);   
    signal(SIGTERM, signal_handler); 
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен. Ожидание подключений...\n");
    printf("Нажмите Ctrl+C для завершения работы.\n");

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;  

    for (int i = 1; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1; 
    }
    nfds = 1;  

    while (keep_running) {
        int ret = poll(fds, nfds, POLL_TIMEOUT);
        
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }
        
        if (ret == 0) {
            continue;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (fds[i].fd == -1) {
                continue;
            }
            
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    client_len = sizeof(client_addr);
                    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    
                    if (client_fd == -1) {
                        perror("accept");
                        continue;
                    }

                    int j;
                    for (j = 1; j <= MAX_CLIENTS; j++) {
                        if (fds[j].fd == -1) {
                            fds[j].fd = client_fd;
                            fds[j].events = POLLIN;
                            
                            if (j >= nfds) {
                                nfds = j + 1;
                            }
                            
                            client_count++;
                            break;
                        }
                    }
                    
                    if (j > MAX_CLIENTS) {
                        fprintf(stderr, "Достигнуто максимальное число клиентов\n");
                        close(client_fd);
                        continue;
                    }
                    
                    printf("Новый клиент подключен (fd=%d). Всего клиентов: %d\n", 
                           client_fd, client_count);
                }
                else {
                    bytes_read = read(fds[i].fd, buffer, BUFFER_SIZE);
                    
                    if (bytes_read > 0) {
                        for (ssize_t j = 0; j < bytes_read; j++) {
                            buffer[j] = toupper((unsigned char)buffer[j]);
                        }
                        
                        printf("[Клиент %d]: ", fds[i].fd);
                        fwrite(buffer, 1, bytes_read, stdout);
                        fflush(stdout);
                    }
                    else if (bytes_read == 0 || (bytes_read == -1 && errno != EINTR)) {
                        printf("Клиент %d отключился\n", fds[i].fd);
                        
                        close(fds[i].fd);
                        client_count--;
                        
                        fds[i].fd = -1;
                        
                        if (i == nfds - 1) {
                            while (nfds > 1 && fds[nfds - 1].fd == -1) {
                                nfds--;
                            }
                        }
                        
                        printf("Осталось клиентов: %d\n", client_count);
                    }
                }
            }
        }
    }

    for (int i = 1; i < nfds; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }
    
    close(server_fd);

    unlink(SOCKET_PATH);
    
    printf("Сервер завершил работу.\n");

    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    
    return 0;
}
