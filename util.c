#include <stdio.h>
#include <string.h>
#include "polyedit.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>


void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
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
    raw.c_lflag &= ~(ECHO | ICANON);
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
