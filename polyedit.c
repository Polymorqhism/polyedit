/*

  CREDITS:
  disable_raw_mode() and enable_raw_mode() were directly taken from:
  https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
  with minimal modification

*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

struct termios original;

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    atexit(disable_raw_mode);
    signal(SIGINT, handle_signal);

    if (argc != 2) {
        printf("Invalid command usage.\n\n%s [file name]\n", argv[0]);
        return 1;
    }

    TargetFile *file = malloc(sizeof(TargetFile));
    if (!file) {
        perror("malloc failed");
        return 1;
    }

    system("clear");
    if (handle_file_intake(argv[1], file)) {
        free(file);
        return 1;
    }

    int num_lines = get_lines(file->contents, file->size);
    file->line_count = num_lines;

    file->lines = malloc(num_lines * sizeof(char *));
    file->line_lengths = malloc(num_lines * sizeof(int));
    int line_index = 0;

    int n = strchr(file->contents, '\n') - file->contents;
    file->lines[line_index++] = strndup(file->contents, n);
    file->line_lengths[0] = strlen(file->lines[0]);

    for(int i = 0; i < file->size; i++) {
        if (file->contents[i] == '\n') {
            file->contents[i] = '\0';

            if (line_index < num_lines) {
                int indexi = line_index++;
                int n = strchr(&file->contents[i+1], '\n') - &file->contents[i+1];
                file->lines[indexi] = strndup(&file->contents[i+1], n);
                file->line_lengths[indexi] = strlen(file->lines[indexi]);
            }
        }
    }

    for(int i = 0; i<file->line_count; i++) {
        printf("%s\n", file->lines[i]);
    }

    enable_raw_mode();
    printf("\x1b[1;1H");

    Cursor cursor = {0, 0, 0, 0};

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        handle_key(c, &cursor, file);
    }

    free(file->lines);
    free(file->contents);
    free(file);
    return 0;
}
