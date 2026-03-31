/*

  CREDITS:
  disable_raw_mode() and enable_raw_mode() were directly taken from:
  https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
  with minimal modification

*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/ioctl.h>
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

struct termios original;

void get_height(Cursor *cur)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cur->terminal_height = ws.ws_row;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Invalid command usage.\n\n%s [file name]\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("File not found");
        return 1;
    }
    fclose(fp);

    setbuf(stdout, NULL);
    atexit(disable_raw_mode);
    signal(SIGINT, handle_signal);

    TargetFile *file = malloc(sizeof(TargetFile));
    if (!file) {
        perror("malloc failed");
        return 1;
    }

    file->name = argv[1];

    if (handle_file_intake(argv[1], file)) {
        free(file);
        return 1;
    }

    if(file->size == 0) {
        file->line_count = 1;
        file->lines = malloc(sizeof(char *));
        file->line_lengths = malloc(sizeof(int));
        file->lines[0] = strdup("");
        file->line_lengths[0] = 0;
    } else {
        int line_count = 0;
        for(int i = 0; i < file->size; i++) {
            if(file->contents[i] == '\n') {
                line_count++;
            }
        }
        if(file->contents[file->size - 1] != '\n') {
            line_count++;
        }

        file->line_count = line_count;
        file->lines = malloc(line_count * sizeof(char *));
        file->line_lengths = malloc(line_count * sizeof(int));

        int line_index = 0;
        int start = 0;

        for(int i = 0; i < file->size; i++) {
            if(file->contents[i] == '\n') {
                int len = i - start;
                file->lines[line_index] = malloc(len + 1);
                memcpy(file->lines[line_index], &file->contents[start], len);
                file->lines[line_index][len] = '\0';
                file->line_lengths[line_index] = len;
                line_index++;
                start = i + 1;
            }
        }

        if(start < file->size) {
            int len = file->size - start;
            file->lines[line_index] = malloc(len + 1);
            memcpy(file->lines[line_index], &file->contents[start], len);
            file->lines[line_index][len] = '\0';
            file->line_lengths[line_index] = len;
        }
    }

    Cursor cursor = {0, 0, 0, 0};
    get_height(&cursor);

    enable_raw_mode();

    printf("\x1b[2J");
    printf("\x1b[H");

    for(int i = 0; i < file->line_count && i < cursor.terminal_height; i++) {
        printf("%s\n", file->lines[i]);
    }

    cursor.scroll = 0;
    cursor.row = 0;
    cursor.col = 0;

    printf("\x1b[H");
    fflush(stdout);

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        handle_key(c, &cursor, file);
    }

    for(int i = 0; i < file->line_count; i++) {
        free(file->lines[i]);
    }

    free(file->lines);
    free(file->line_lengths);
    free(file->contents);
    free(file);
    return 0;
}
