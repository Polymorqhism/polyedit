#include <termios.h>

typedef struct {
    char *contents;
    char **lines;
    long size;
    int line_count;
} TargetFile;

typedef struct {
    int row;
    int col;

    int sc_row;
    int sc_col;
} Cursor;

extern struct termios original;
