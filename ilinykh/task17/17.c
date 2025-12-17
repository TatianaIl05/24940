#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE 40
#define ERASE   0x7F    // Backspace 
#define KILL CTRL('U')
#define WORD_ERASE CTRL('W')
#define EOF_KEY CTRL('D')

struct termios orig_termios;
char line[MAX_LINE + 1] = {0};  
int pos = 0;                    

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_terminal);

    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  
    raw.c_cc[VMIN] = 1;      
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void beep(void) {
    write(STDOUT_FILENO, "\a", 1);
}

void erase_word(void) {
    while (pos > 0 && line[pos-1] == ' ') {
        pos--;
        write(STDOUT_FILENO, "\b \b", 3);  
    }
    while (pos > 0 && line[pos-1] != ' ') {
        pos--;
        write(STDOUT_FILENO, "\b \b", 3);
    }
}

void wrap_line(void) {
    if (pos <= MAX_LINE) return;

    int word_start = pos;
    while (word_start > 0 && line[word_start-1] != ' ') word_start--;
    
    if (word_start == 0) {
        word_start = MAX_LINE;
    }
    
    int word_len = pos - word_start;
    char word_to_move[word_len + 1];
    memcpy(word_to_move, &line[word_start], word_len);
    word_to_move[word_len] = '\0';
    
    pos = word_start;
    line[pos] = '\0';
    
    write(STDOUT_FILENO, "\r\033[K", 4);
    write(STDOUT_FILENO, line, pos);
    write(STDOUT_FILENO, "\r\n", 2);
    
    memcpy(line, word_to_move, word_len);
    pos = word_len;
    line[pos] = '\0';
    
    write(STDOUT_FILENO, line, pos);
}

int main(void) {
    enable_raw_mode();
    printf("CTRL+U - стирает последнюю строку\nCTRL+W - стирает последнее слово\nCTRL+D в начале строки - выход\n");
    
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == EOF_KEY && pos == 0) {
            printf("\r\nВыход по Ctrl+D\r\n");
            break;
        }

        if (c == ERASE && pos > 0) {
            pos--;
            line[pos] = '\0';
            write(STDOUT_FILENO, "\b \b", 3);
            continue;
        }

        if (c == KILL) {
            while (pos > 0) {
                write(STDOUT_FILENO, "\b \b", 3);
                pos--;
            }
            line[0] = '\0';
            continue;
        }

        if (c == WORD_ERASE) {
            erase_word();
            line[pos] = '\0';
            continue;
        }

        if (c == '\r' || c == '\n') {
            printf("\r\n");
            pos = 0;
            line[0] = '\0';
            continue;
        }

        if (isprint(c) && pos < MAX_LINE) {
            line[pos++] = c;
            line[pos] = '\0';
            write(STDOUT_FILENO, &c, 1);
        } else if (isprint(c)) {
            line[pos++] = c;
            line[pos] = '\0';
            wrap_line();
        } else {
            beep();
        }
    }

    restore_terminal();
    return 0;
}
