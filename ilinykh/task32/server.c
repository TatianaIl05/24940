#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <aio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 100
#define READ_SIZE 1

struct client {
    int          fd;
    struct aiocb cb;
    char         byte;
    int          active;
    int          id;        
};

static struct client clients[MAX_CLIENTS];

static void cleanup(int sig) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    unlink(SOCKET_PATH);
    exit(0);
}

int setup_aio_read(struct client *c) {
    memset(&c->cb, 0, sizeof(struct aiocb));
    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf    = &c->byte;
    c->cb.aio_nbytes = READ_SIZE;
    c->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    
    if (aio_read(&c->cb) == -1) {
        if (errno != EAGAIN && errno != EINPROGRESS) {
            perror("aio_read");
            return -1;
        }
    }
    return 0;
}

int main(void) {
    int listen_fd, new_fd;
    struct sockaddr_un addr;
    int slot;

    signal(SIGINT,  cleanup);
    signal(SIGTERM, cleanup);

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) { perror("socket"); exit(1); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    unlink(SOCKET_PATH);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) { 
        perror("bind"); exit(1); 
    }
    if (listen(listen_fd, 10) == -1) { perror("listen"); exit(1); }

    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    printf("Сервер запущен. Ожидание подключений...\n");
    printf("Нажмите Ctrl+C для завершения работы.\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].id = i + 1;
        clients[i].active = 0;
    }

    while (1) {
        new_fd = accept(listen_fd, NULL, NULL);
        if (new_fd != -1) {
            for (slot = 0; slot < MAX_CLIENTS; slot++)
                if (clients[slot].fd == -1) break;
            
            if (slot == MAX_CLIENTS) { 
                printf("Достигнуто максимальное количество клиентов\n");
                close(new_fd); 
            } else {
                clients[slot].fd = new_fd;
                clients[slot].active = 1;
                
                printf("Клиент %d подключился\n", clients[slot].id);
                
                if (setup_aio_read(&clients[slot]) == -1) {
                    close(clients[slot].fd);
                    clients[slot].fd = -1;
                    clients[slot].active = 0;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == -1 || !clients[i].active) continue;

            int err = aio_error(&clients[i].cb);
            
            if (err == EINPROGRESS) {
                continue;
            } else if (err == EAGAIN) {
                if (setup_aio_read(&clients[i]) == -1) {
                    printf("Клиент %d отключился (ошибка чтения)\n", clients[i].id);
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                }
                continue;
            } else if (err != 0) {
                if (err != ECANCELED) {
                    perror("aio_error");
                }
                printf("Клиент %d отключился\n", clients[i].id);
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                continue;
            }

            ssize_t ret = aio_return(&clients[i].cb);
            
            if (ret > 0) {
                char c = toupper((unsigned char)clients[i].byte);
                printf("%c", c);
                fflush(stdout);
                
                if (setup_aio_read(&clients[i]) == -1) {
                    printf("Клиент %d отключился\n", clients[i].id);
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                }
                
            } else if (ret == 0) {
                printf("\nКлиент %d отключился (EOF)\n", clients[i].id);
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                
            } else {
                perror("read error");
                printf("Клиент %d отключился\n", clients[i].id);
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
            }
        }
        
        usleep(10000);
    }
    
    return 0;
}
