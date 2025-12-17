#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>

#define SOCKET_PATH "./socket"

void set_raw_mode(int enable) {
    static struct termios original;
    static int initialized = 0;
    
    if (!initialized) {
        tcgetattr(STDIN_FILENO, &original);
        initialized = 1;
    }
    
    struct termios raw = original;
    if (enable) {
        raw.c_lflag &= ~(ICANON);
    } else {
        raw = original;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

int main() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    printf("Подключено. Набирайте текст (по символу):\n");

    set_raw_mode(1);
    
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        write(sockfd, &c, 1);
        if (c == 4) break;  
    }

    set_raw_mode(0);
    
    close(sockfd);
    return 0;
}
