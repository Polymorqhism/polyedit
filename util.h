#include "polyedit.h"

void disable_raw_mode();
void handle_signal(int sig);
void enable_raw_mode();
long get_file_size(char *file_name);
int get_lines(const char *buf, int size);
int handle_file_intake(char *file_name, TargetFile *target);
void handle_key(char key, Cursor *cur, TargetFile *file);
void redraw_screen(Cursor *cur, TargetFile *file);
void get_height(Cursor *cur);
int get_width();
