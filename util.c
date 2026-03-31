#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "polyedit.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
    system("clear");
}

void redraw_screen(Cursor *cur, TargetFile *file)
{
    printf("\x1b[1;1H");
    printf("\x1b[J");
    for(int i = cur->scroll; i < cur->scroll + cur->terminal_height && i < file->line_count; i++) {
        printf("%s\n", file->lines[i]);
    }
}

void insert_key(char key, Cursor *cur, TargetFile *file)
{
        file->lines[cur->row] = realloc(file->lines[cur->row], file->line_lengths[cur->row] + 2);

        memmove(&file->lines[cur->row][cur->col + 1], &file->lines[cur->row][cur->col], file->line_lengths[cur->row] - cur->col + 1);
        file->lines[cur->row][cur->col] = key;
        file->line_lengths[cur->row]++;
        cur->col++;
        printf("\x1b[%d;1H", cur->row - cur->scroll + 1);
        printf("\x1b[2K"); // clears line
        printf("%s", file->lines[cur->row]); // prints new line
        printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col+1);
}

void handle_key(char key, Cursor *cur, TargetFile *file)
{
    char c;
    if(key == 27) { // multi-byte incoming
        read(STDIN_FILENO, &c, 1);
        if(c == '[') { // arrow key incoming probably
            read(STDIN_FILENO, &c, 1);
            switch(c) {
            case 'A':
                if(cur->row > 0)
                    cur->row--;
                if(cur->row < cur->scroll) {
                    cur->scroll--;
                    redraw_screen(cur, file);
                }

                if(cur->col > strlen(file->lines[cur->row]))
                    cur->col = strlen(file->lines[cur->row]);
                break;
            case 'B':
                if(cur->row < file->line_count-1)
                    cur->row++;
                if(cur->row - cur->scroll >= cur->terminal_height) {
                    cur->scroll++;
                    redraw_screen(cur, file);
                }
                if(cur->col > strlen(file->lines[cur->row]))
                    cur->col = strlen(file->lines[cur->row]);
                break;
            case 'C':
                if(cur->col < strlen(file->lines[cur->row]))
                    cur->col++;
                break;
            case 'D':
                if(cur->col > 0)
                    cur->col--;
                break;
            }

            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col+1);
        }
    } else {
        if(key == 127) {
            if(cur->col > 0) {
                memmove(&file->lines[cur->row][cur->col - 1], &file->lines[cur->row][cur->col], file->line_lengths[cur->row] - cur->col + 1);
                file->line_lengths[cur->row]--;
                cur->col--;
                printf("\x1b[%d;1H", cur->row - cur->scroll + 1);
                printf("\x1b[2K"); // clear ENTIRE line
                printf("%s", file->lines[cur->row]); // re-render line
                printf("\x1b[%d;%dH", cur->row+1, cur->col+1); // bring it back
                printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col+1);
            }
            else if(cur->col == 0 && cur->row > 0) {
                int prev_len = file->line_lengths[cur->row-1];
                int curr_len = file->line_lengths[cur->row];

                file->lines[cur->row-1] = realloc(file->lines[cur->row-1], prev_len + curr_len + 1);
                strcat(file->lines[cur->row-1], file->lines[cur->row]);

                free(file->lines[cur->row]);

                if(file->line_count - cur->row - 1 > 0) {
                    memmove(&file->lines[cur->row], &file->lines[cur->row+1], (file->line_count - cur->row - 1) * sizeof(char *));
                    memmove(&file->line_lengths[cur->row], &file->line_lengths[cur->row+1], (file->line_count - cur->row - 1) * sizeof(int));
                }

                file->line_lengths[cur->row-1] = prev_len + curr_len;
                file->line_count--;
                cur->col = prev_len;
                cur->row--;

                printf("\x1b[%d;1H", cur->row - cur->scroll + 1);
                printf("\x1b[J");
                for(int i = cur->row; i < file->line_count && i < cur->scroll + cur->terminal_height; i++) {
                    printf("%s\n", file->lines[i]);
                }
                printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col+1);
            }
        } else if(key == 10 || key == 13) {
            char *new_line = strndup(&file->lines[cur->row][cur->col], file->line_lengths[cur->row] - cur->col);
            file->lines[cur->row][cur->col] = '\0';
            file->line_lengths[cur->row] = cur->col;
            file->lines = realloc(file->lines, (file->line_count + 1) * sizeof(char *));
            file->line_lengths = realloc(file->line_lengths, (file->line_count + 1) * sizeof(int));
            memmove(&file->lines[cur->row + 2], &file->lines[cur->row + 1], (file->line_count - cur->row - 1) * sizeof(char *));
            memmove(&file->line_lengths[cur->row + 2], &file->line_lengths[cur->row + 1], (file->line_count - cur->row - 1) * sizeof(int));
            file->lines[cur->row + 1] = new_line;
            file->line_lengths[cur->row + 1] = strlen(new_line);
            file->line_count++;
            cur->row++;
            cur->col = 0;
            printf("\x1b[%d;1H", cur->row); // one above since that line changed too
            printf("\x1b[J");
            for(int i = cur->row - 1; i < file->line_count; i++) {
                printf("%s\n", file->lines[i]);
            }
            printf("\x1b[%d;%dH", cur->row - cur->scroll + 1, cur->col + 1);
        } else if(key == 9) { // handle tab
            for(int i = 0; i<4; i++) {
                insert_key(' ', cur, file);
            }
        } else if(key == 19) { // handle C-s
            FILE *fp = fopen(file->name, "w");
            if(!fp) {
                return;
            }
            for(int i = 0; i<file->line_count; i++) {
                fwrite(file->lines[i], strlen(file->lines[i]), sizeof(char), fp);
                fputc('\n', fp);
            }
            fclose(fp);
        }
        else if(key == 3) {
            disable_raw_mode();
            exit(0);
        }

        if(!isprint(key)) {
            return;
        }
        insert_key(key, cur, file);
    }
}


void handle_signal(int sig)
{
    disable_raw_mode();
    exit(1);
}

void enable_raw_mode()
{
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
    int lines = 0;

    for(int i = 0; i<size; i++) {
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
    if (bytes_read != file_size) {
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
