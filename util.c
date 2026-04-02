#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "polyedit.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct termios original;

void get_height(Cursor *cur)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cur->terminal_height = ws.ws_row;
}

int get_width()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}

void disable_raw_mode()
{
    printf("\x1b]112\x07");
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

const char *keywords[34] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

int keyw_len = 34;

void highlight_line(const char *line, int line_length)
{
    int i = 0;
    while(i < line_length) {
        if(isspace(line[i])) {
            printf("%c", line[i]);
            i++;
        } else if(isalpha(line[i]) || line[i] == '_') {
            char tmp[32];
            int l = 0;

            while(i < line_length && (isalnum(line[i]) || line[i] == '_')) {
                tmp[l] = line[i];
                i++;
                l++;
            }
            tmp[l] = '\0';

            int found = 0;
            for(int i = 0; i<keyw_len; i++) {
                if(strcmp(tmp, keywords[i]) == 0) {
                    printf("\x1b[1;38;2;255;221;51m%s\x1b[0m", tmp);
                    found = 1;
                    break;
                }
            }

            if(!found) {
                printf("%s", tmp);
            }
        } else if(line[i] == '/' && i + 1 < line_length && line[i + 1] == '/') {
            printf("\x1b[38;2;188;129;55m");
            printf("%s", &line[i]);
            printf("\x1b[0m");
            break;
        } else if(line[i] == '"') {
            printf("\x1b[38;2;112;196;53m");
            printf("%c", line[i]);
            i++;

            while(i < line_length && line[i] != '"') {
                printf("%c", line[i]);
                i++;
            }

            if(i < line_length) {
                printf("%c", line[i]);
                i++;
            }

            printf("\x1b[0m");
        } else {
            printf("%c", line[i]);
            i++;
        }
    }
}

void redraw_screen(Cursor *cur, TargetFile *file)
{
    get_height(cur);    
    printf("\x1b[2J");  // Clear entire screen
    printf("\x1b[H");   // Move cursor to top-left

    int start = cur->scroll;
    int end = start + cur->terminal_height;
    if (end > file->line_count) {
        end = file->line_count;
    }

    for (int i = start; i < end; i++) {
        // printf("%s", file->lines[i]);

        if(file->do_highlight == 1) {
            highlight_line(file->lines[i], file->line_lengths[i]);
        } else {
            printf("%s", file->lines[i]);
        }

        if (i < end - 1) {
            printf("\n");
        }    
    }

    fflush(stdout);
}

void insert_key(char key, Cursor *cur, TargetFile *file)
{
    file->lines[cur->row] = realloc(file->lines[cur->row], file->line_lengths[cur->row] + 2);
    memmove(&file->lines[cur->row][cur->col + 1], &file->lines[cur->row][cur->col], file->line_lengths[cur->row] - cur->col + 1);
    file->lines[cur->row][cur->col] = key;
    file->line_lengths[cur->row]++;
    cur->col++;
    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, 1);
    printf("\x1b[K");
    highlight_line(file->lines[cur->row], file->line_lengths[cur->row]);
    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
    fflush(stdout);
}

void cur_up(Cursor *cur, TargetFile *file)
{
    if(cur->row > 0)
        cur->row--;
    if(cur->row < cur->scroll) {
        cur->scroll = cur->row;
        redraw_screen(cur, file);
    }
    if(cur->col > file->line_lengths[cur->row])
        cur->col = file->line_lengths[cur->row];
    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
    fflush(stdout);
}

void cur_down(Cursor *cur, TargetFile *file)
{
    if(cur->row < file->line_count-1)
        cur->row++;

    if(cur->row - cur->scroll >= cur->terminal_height) {
        cur->scroll = cur->row - cur->terminal_height + 1;
        redraw_screen(cur, file);
    }

    if(cur->col > file->line_lengths[cur->row])
        cur->col = file->line_lengths[cur->row];

    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
    fflush(stdout);
}

void cur_forward(Cursor *cur, TargetFile *file)
{
    if(cur->col < file->line_lengths[cur->row])
        cur->col++;
    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
    fflush(stdout);
}

void cur_backward(Cursor *cur)
{
    if(cur->col > 0)
        cur->col--;
    printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
    fflush(stdout);
}

void handle_key(char key, Cursor *cur, TargetFile *file)
{
    char c;
    if(key == 27) {
        read(STDIN_FILENO, &c, 1);
        if(c == '[') {
            read(STDIN_FILENO, &c, 1);
            switch(c) {
            case 'A':
                cur_up(cur, file);
                break;
            case 'B':
                cur_down(cur, file);
                break;
            case 'C':
                cur_forward(cur, file);
                break;
            case 'D':
                cur_backward(cur);
                break;
            }
        } else if(c == 'b') {
            if(cur->col > 0) {
                cur->col--;
                while(cur->col > 0 && (isalnum(file->lines[cur->row][cur->col]) || file->lines[cur->row][cur->col] == '_') ) {
                    cur->col--;
                }
            }
            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
            fflush(stdout);
        } else if(c == 'f') {
            if(cur->col < file->line_lengths[cur->row]) {
                cur->col++;
                while(cur->col < file->line_lengths[cur->row] && (isalnum(file->lines[cur->row][cur->col]) || file->lines[cur->row][cur->col] == '_') ) {
                    cur->col++;
                }
            }
            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
            fflush(stdout);
        }
    } else if(key == 127) {
        if(cur->col > 0) {
            int len = file->line_lengths[cur->row];
            memmove(&file->lines[cur->row][cur->col - 1], &file->lines[cur->row][cur->col], len - cur->col + 1);
            file->line_lengths[cur->row]--;
            cur->col--;
            redraw_screen(cur, file);
            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
            fflush(stdout);
        }
        else if(cur->col == 0 && cur->row > 0) {
            int prev_len = file->line_lengths[cur->row-1];
            int curr_len = file->line_lengths[cur->row];

            char *merged = malloc(prev_len + curr_len + 1);
            memcpy(merged, file->lines[cur->row-1], prev_len);
            memcpy(merged + prev_len, file->lines[cur->row], curr_len);
            merged[prev_len + curr_len] = '\0';

            free(file->lines[cur->row-1]);
            free(file->lines[cur->row]);

            int new_count = file->line_count - 1;
            char **new_lines = malloc(new_count * sizeof(char *));
            int *new_lengths = malloc(new_count * sizeof(int));

            for (int i = 0; i < cur->row - 1; i++) {
                new_lines[i] = file->lines[i];
                new_lengths[i] = file->line_lengths[i];
            }

            new_lines[cur->row - 1] = merged;
            new_lengths[cur->row - 1] = prev_len + curr_len;

            for (int i = cur->row + 1; i < file->line_count; i++) {
                new_lines[i - 1] = file->lines[i];
                new_lengths[i - 1] = file->line_lengths[i];
            }

            free(file->lines);
            free(file->line_lengths);

            file->lines = new_lines;
            file->line_lengths = new_lengths;
            file->line_count = new_count;

            cur->row--;
            cur->col = prev_len;

            if (cur->row < cur->scroll) {
                cur->scroll = cur->row;
            }

            redraw_screen(cur, file);
            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
            fflush(stdout);
        }
    } else if(key == 10 || key == 13) {
        int remaining_len = file->line_lengths[cur->row] - cur->col;
        char *new_line = malloc(remaining_len + 1);

        if (remaining_len > 0) {
            memcpy(new_line, &file->lines[cur->row][cur->col], remaining_len);
            new_line[remaining_len] = '\0';
            file->lines[cur->row][cur->col] = '\0';
            file->line_lengths[cur->row] = cur->col;
        } else {
            new_line[0] = '\0';
        }

        int new_count = file->line_count + 1;
        char **new_lines = malloc(new_count * sizeof(char *));
        int *new_lengths = malloc(new_count * sizeof(int));

        for (int i = 0; i <= cur->row; i++) {
            new_lines[i] = file->lines[i];
            new_lengths[i] = file->line_lengths[i];
        }

        new_lines[cur->row + 1] = new_line;
        new_lengths[cur->row + 1] = remaining_len;

        for (int i = cur->row + 1; i < file->line_count; i++) {
            new_lines[i + 1] = file->lines[i];
            new_lengths[i + 1] = file->line_lengths[i];
        }

        free(file->lines);
        free(file->line_lengths);

        file->lines = new_lines;
        file->line_lengths = new_lengths;
        file->line_count = new_count;

        cur->row++;
        cur->col = 0;

        if (cur->row >= cur->scroll + cur->terminal_height) {
            cur->scroll = cur->row - cur->terminal_height + 1;
        }

        redraw_screen(cur, file);
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
        fflush(stdout);
    } else if(key == 9) {
        for(int i = 0; i<4; i++) {
            insert_key(' ', cur, file);
        }
    } else if(key == 19) {
        FILE *fp = fopen(file->name, "w");
        if(fp) {
            for(int i = 0; i < file->line_count; i++) {
                fwrite(file->lines[i], file->line_lengths[i], 1, fp);
                if (i < file->line_count - 1 || file->line_lengths[i] > 0) {
                    fputc('\n', fp);
                }
            }
            fclose(fp);
        }
    } else if(key == 16){
        cur_up(cur, file);
    } else if(key == 14) {
        cur_down(cur, file);
    } else if(key == 2) {
        cur_backward(cur);
    } else if(key == 6) {
        cur_forward(cur, file);
    } else if(key == 3) {
        disable_raw_mode();
        exit(0);
    } else if(key == 5) {
        cur->col = file->line_lengths[cur->row];
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
        fflush(stdout);
    } else if(key == 24) {
        free(file->lines[cur->row]);

        int new_count = file->line_count - 1;

        char **new_lines = malloc(sizeof(char* ) * new_count);
        int *new_length = malloc(sizeof(int) * new_count);

        for(int i = 0; i<cur->row; i++) {
            new_lines[i] = file->lines[i];
            new_length[i] = file->line_lengths[i];
        }

        for(int i = cur->row+1; i<file->line_count; i++) {
            new_lines[i-1] = file->lines[i];
            new_length[i-1] = file->line_lengths[i];
        }

        free(file->lines);
        free(file->line_lengths);

        file->lines = new_lines;
        file->line_lengths = new_length;
        file->line_count = new_count;

        if(cur->row >= file->line_count) {
            cur->row = file->line_count - 1;
        }

        cur->col = 0;
        redraw_screen(cur, file);
    } else if(key == 23) {
        if(cur->col > 0) {
            int start_col = cur->col;
            cur->col--;
            while(cur->col > 0 && (isalnum(file->lines[cur->row][cur->col]) || file->lines[cur->row][cur->col] == '_')) {
                cur->col--;
            }

            if(!(isalnum(file->lines[cur->row][cur->col]) || file->lines[cur->row][cur->col] == '_') && cur->col < file->line_lengths[cur->row]) {
                cur->col++;
            }

        int len = file->line_lengths[cur->row];
        int delete_len = start_col - cur->col;
        memmove(&file->lines[cur->row][cur->col], &file->lines[cur->row][start_col], len - start_col + 1);
        file->line_lengths[cur->row] -= delete_len;
        
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, 1);
        printf("\x1b[K");
        highlight_line(file->lines[cur->row], file->line_lengths[cur->row]);
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
        fflush(stdout);
        }
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
        fflush(stdout);
    } else if(isprint(key)) {
        insert_key(key, cur, file);
    }
}

void handle_signal(int sig)
{
    (void)sig;
    disable_raw_mode();
    exit(1);
}

void enable_raw_mode()
{
    printf("\x1b]12;#ffdd33\x07");
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    struct termios raw = original;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

long get_file_size(char *file_name)
{
    struct stat st;
    if (stat(file_name, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

int get_lines(const char *buf, int size)
{
    if (size == 0) return 1;
    int lines = 0;
    for(int i = 0; i < size; i++) {
        if(buf[i] == '\n') {
            lines++;
        }
    }
    if(size > 0 && buf[size-1] != '\n') {
        lines++;
    }
    return lines;
}

int handle_file_intake(char *file_name, TargetFile *target)
{
    FILE *fp = fopen(file_name, "r");
    if (!fp) {
        perror("File not found");
        return 1;
    }

    long file_size = get_file_size(file_name);
    if (file_size == -1) {
        perror("Could not get file size");
        fclose(fp);
        return 1;
    }

    char *file_contents = malloc(file_size + 1);
    if (!file_contents) {
        perror("malloc failed");
        fclose(fp);
        return 1;
    }

    size_t bytes_read = fread(file_contents, 1, file_size, fp);
    if ((long)bytes_read != file_size) {
        perror("Could not read entire file");
        free(file_contents);
        fclose(fp);
        return 1;
    }

    file_contents[bytes_read] = '\0';
    target->contents = file_contents;
    target->size = bytes_read;
    fclose(fp);
    return 0;
}
