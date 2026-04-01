#include <termios.h>
#include <stdio.h>

typedef struct {
    char *contents;
    char **lines;
    char *name;
    long size;
    int line_count;
    int *line_lengths;
    int do_highlight;
} TargetFile;

typedef struct {
    int row;
    int col;

    int terminal_height;
    int scroll;
} Cursor;

extern struct termios original;
