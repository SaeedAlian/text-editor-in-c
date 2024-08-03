#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* Macros and definitions */

#define CTRL_KEY(k) ((k) & 0x1f)

/* Editor config */

struct conf {
  int rows;
  int cols;
  struct termios orig_termios;
};

struct conf config;

/* Function declarations */

void draw_rows();
void clear_screen();
void move_cursor_to_top();
void refresh_screen();
int get_cursor_pos(int *rows, int *cols);
int get_term_size(int *rows, int *cols);
void die(const char *s);
void disable_raw_mode();
void enable_raw_mode();
char read_input_key();
void process_key_press();
void init();

/* Main functions */

int main() {
  enable_raw_mode();
  init();

  while (1) {
    refresh_screen();
    process_key_press();
  }

  return 0;
}

/* Function definitions */

int get_cursor_pos(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDIN_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1 || buf[i] == 'R')
      break;
    i++;
  }

  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;

  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int get_term_size(int *rows, int *cols) {
  struct winsize w;

  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &w) != -1 && w.ws_col != 0) {
    *cols = w.ws_col;
    *rows = w.ws_row;
    return 0;
  } else {
    if (write(STDIN_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return get_cursor_pos(rows, cols);
  }
}

void die(const char *s) {
  clear_screen();
  move_cursor_to_top();
  perror(s);
  exit(1);
}

void refresh_screen() {
  clear_screen();
  move_cursor_to_top();
  draw_rows();
  move_cursor_to_top();
}

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &config.orig_termios) == -1)
    die("tcsetattr");
}

void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &config.orig_termios) == -1)
    die("tcgetattr");

  atexit(disable_raw_mode);

  struct termios raw = config.orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char read_input_key() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  return c;
}

void clear_screen() { write(STDIN_FILENO, "\x1b[2J", 4); }

void move_cursor_to_top() { write(STDIN_FILENO, "\x1b[H", 3); }

void process_key_press() {
  char key = read_input_key();

  switch (key) {
  case CTRL_KEY('q'): {
    clear_screen();
    move_cursor_to_top();
    exit(0);
    break;
  }
  }
}

void draw_rows() {
  for (int y = 0; y < config.rows; y++) {
    write(STDOUT_FILENO, "~", 1);

    if (y < config.rows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void init() {
  if (get_term_size(&config.rows, &config.cols) == -1)
    die("get_term_size");
}
