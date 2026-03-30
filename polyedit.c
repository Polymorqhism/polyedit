/*

  CREDITS:
  disable_raw_mode() and enable_raw_mode() were directly taken from:
  https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
  with minimal modification

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

struct termios original;

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

typedef struct {
    char *contents;
    long size;
} TargetFile;

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

    printf("Read %zu bytes, file size was %ld. File contents:\n\n%s\n",
           bytes_read, file_size, file_contents);

    target->contents = file_contents;
    target->size = bytes_read;

    fclose(fp);
    return 0;
}

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

    char c;

    while (read(STDIN_FILENO, &c, 1) == 1) {
        printf("%c", c);
    }


    free(file->contents);
    free(file);
    return 0;
}
