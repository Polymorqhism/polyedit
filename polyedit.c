/*

  CREDITS:
  disable_raw_mode() and enable_raw_mode() were directly taken from:
  https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
  with minimal modification

*/

#include <stdio.h>
#include "util.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

char lines[] = {};

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

    enable_raw_mode();

    int num_lines = get_lines(file->contents, file->size);
    file->line_count = num_lines;

    file->lines = malloc(num_lines * sizeof(char *));
    int line_index = 0;

    file->lines[line_index++] = file->contents;

    for(int i = 0; i < file->size; i++) {
        if (file->contents[i] == '\n') {
            file->contents[i] = '\0';

            if (line_index < num_lines) {
                file->lines[line_index++] = &file->contents[i + 1];
            }
        }
    }

    for(int i = 0; i<file->line_count; i++) {
        printf("%s\n", file->lines[i]);
    }

    enable_raw_mode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if((unsigned char) c == 4) {
            disable_raw_mode();
            break;
        }
        printf("%c", c);
    }

    free(file->lines);
    free(file->contents);
    free(file);
    return 0;
}
