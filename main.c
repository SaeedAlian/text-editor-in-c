#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* ------ Macros and definitions ------ */

#define CTRL_KEY(k) ((k) & 0x1f)

#define VERSION "0.0.1"

/* ------ Editor config ------ */

struct conf {
  int cx;
  int cy;
  int rows;
  int cols;
  struct termios orig_termios;
};

struct conf config;

enum editor_keys { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };

/* ------ Appendable buffer ------ */

struct ap_buf {
  char *b;
  int len;
};

#define AP_BUF_INIT {NULL, 0}

/* ------ Function declarations ------ */

/* --- appendable buffer --- */

/*
 * append a string to the appendable buffer. It will receive
 * the buffer struct pointer, new string and the new string length.
 */
void ap_buf_append(struct ap_buf *buf, const char *s, int len);

/*
 * free the buffer of the appendable buffer struct. It will receive
 * the buffer struct pointer.
 */
void free_ap_buf(struct ap_buf *buf);

/* --- screen update --- */

/*
 * Draw tildes on the left side of the screen.
 * It will receive the appendable buffer pointer.
 */
void draw_rows(struct ap_buf *buf);

/*
 * It will force write to the screen to clear it.
 * This function will use the write function immediately,
 * and it won't use the appendable buffer.
 */
void clear_screen();

/*
 * It will force write to the screen to move the cursor to the given position.
 * This function will use the write function immediately,
 * and it won't use the appendable buffer.
 * It will receive the x and y of the position
 */
void move_cursor(int x, int y);

/*
 * Updates the cursor position based on the given key,
 * to left, right, up or down.
 * It will update the cx and cy values in the editor config.
 * It will receives the pressed key as a parameter.
 */
void update_cursor_pos(int key);

/*
 * Refresh the screen by first hiding the cursor,
 * then move the cursor to the top of screen
 * and draw the leftside tildes (clears each line before drawing),
 * and the move the cursor to the defined position in the config struct.
 * It will use the appendable buffer to do all of this with
 * a single write to the screen.
 */
void refresh_screen();

/* --- get screen info --- */

/*
 * Gets the cursor position in the screen and copy the
 * values in the rows and cols parameters which it receives.
 */
int get_cursor_pos(int *rows, int *cols);

/*
 * Gets the terminal size and copy the values
 * in the rows and cols parameters which it receives.
 */
int get_term_size(int *rows, int *cols);

/* --- raw mode --- */

/*
 * Disables the raw mode in the terminal
 */
void disable_raw_mode();

/*
 * Enables the raw mode in the terminal
 */
void enable_raw_mode();

/* --- key press event handlers --- */

/*
 * Reads each key press and returns the character which has been pressed.
 */
int read_input_key();

/*
 * Reads the pressed key, then assign the special keys to certain actions.
 */
void process_key_press();

/* --- error handling --- */

/*
 * Clears the screen, moves the cursor to the top,
 * and prints the error on the screen and then sends the exit signal.
 * It will receive the error string.
 */
void die(const char *s);

/* --- init function --- */

/*
 * Initializes the editor configuration.
 */
void init();

/* --- Main function --- */

int main() {
  enable_raw_mode();
  init();

  while (1) {
    refresh_screen();
    process_key_press();
  }

  return 0;
}

/* ------ Function definitions ------ */

void clear_screen() { write(STDIN_FILENO, "\x1b[2J", 4); }

void move_cursor(int x, int y) {
  char temp_buf[32];
  snprintf(temp_buf, sizeof(temp_buf), "\x1b[%d;%dH", y + 1, x + 1);
  write(STDIN_FILENO, temp_buf, strlen(temp_buf));
}

void ap_buf_append(struct ap_buf *buf, const char *s, int len) {
  char *new = realloc(buf->b, buf->len + len);

  if (new == NULL)
    return;

  memcpy(&new[buf->len], s, len);
  buf->b = new;
  buf->len += len;
}

void free_ap_buf(struct ap_buf *buf) { free(buf->b); }

int get_cursor_pos(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  // the 'n' command (Device Status Report) will
  // give the terminal status information
  // and with the argument '6' it will give the
  // cursor position and it can be read from the
  // standard input.
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

void draw_rows(struct ap_buf *buf) {
  for (int y = 0; y < config.rows; y++) {

    if (y == config.rows / 3) {
      char welcome[80];
      int welcome_len = snprintf(welcome, sizeof(welcome),
                                 "Text Editor In C - Version %s", VERSION);

      if (welcome_len > config.cols) {
        welcome_len = config.cols;
      }

      ap_buf_append(buf, "~", 1);
      for (int i = 0; i < (config.cols - welcome_len) / 2; i++) {
        ap_buf_append(buf, " ", 1);
      }

      ap_buf_append(buf, welcome, welcome_len);
    } else {

      ap_buf_append(buf, "~", 1);
    }

    // clears each line
    ap_buf_append(buf, "\x1b[K", 3);

    if (y < config.rows - 1) {
      ap_buf_append(buf, "\r\n", 2);
    }
  }
}

void refresh_screen() {
  struct ap_buf buf = AP_BUF_INIT;

  // hides the cursor
  ap_buf_append(&buf, "\x1b[?25l", 6);

  // moves cursor to the top
  ap_buf_append(&buf, "\x1b[H", 3);

  draw_rows(&buf);

  // moves cursor to the defined position in the config struct
  char temp_buf[32];
  snprintf(temp_buf, sizeof(temp_buf), "\x1b[%d;%dH", config.cy + 1,
           config.cx + 1);
  ap_buf_append(&buf, temp_buf, strlen(temp_buf));

  // shows the cursor
  ap_buf_append(&buf, "\x1b[?25h", 6);

  write(STDIN_FILENO, buf.b, buf.len);
  free_ap_buf(&buf);
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

void update_cursor_pos(int key) {
  switch (key) {
  case ARROW_RIGHT: {
    config.cx++;
    break;
  }
  case ARROW_LEFT: {
    config.cx--;
    break;
  }
  case ARROW_UP: {
    config.cy--;
    break;
  }
  case ARROW_DOWN: {
    config.cy++;
    break;
  }
  }
}

int read_input_key() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b') {
    char esc[3];

    if (read(STDIN_FILENO, &esc[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &esc[1], 1) != 1)
      return '\x1b';

    if (esc[0] == '[') {
      switch (esc[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      }
    }

    return '\x1b';
  }

  return c;
}

void process_key_press() {
  int key = read_input_key();

  switch (key) {
  case CTRL_KEY('q'): {
    clear_screen();
    move_cursor(0, 0);
    exit(0);
    break;
  }

  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    update_cursor_pos(key);
    break;
  }
}

void die(const char *s) {
  clear_screen();
  move_cursor(0, 0);
  perror(s);
  exit(1);
}

void init() {
  config.cx = 0;
  config.cy = 0;

  if (get_term_size(&config.rows, &config.cols) == -1)
    die("get_term_size");
}
