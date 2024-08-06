#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

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

#define TAB_STOP 4

/* ------ Types ------ */

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

/* ------ Editor config ------ */

struct conf {
  int cx;
  int rx;
  int cy;
  int rows;
  int cols;
  int rowoff;
  int coloff;
  int numrows;
  erow *editor_rows;
  struct termios orig_termios;
};

struct conf config;

enum editor_keys {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY
};

/* ------ Appendable buffer ------ */

struct ap_buf {
  char *b;
  int len;
};

#define AP_BUF_INIT {NULL, 0}

/* ------ Function declarations ------ */

/* --- editor rows --- */

/*
 * Appends a new editor row to the editor_row array in the
 * editor config struct.
 * It will receive the line string and the line length.
 */
void append_erow(char *s, size_t len);

/*
 * Formats each row characters, and copy it into
 * the render field of the row.
 * It will receive the row pointer.
 */
void update_erow(erow *row);

/*
 * Converts cx into rx by counting the tab stops.
 * It will receive the row pointer and the current cx.
 */
int row_cx_to_rx(erow *row, int cx);

/* --- appendable buffer --- */

/*
 * Append a string to the appendable buffer. It will receive
 * the buffer struct pointer, new string and the new string length.
 */
void ap_buf_append(struct ap_buf *buf, const char *s, int len);

/*
 * Free the buffer of the appendable buffer struct. It will receive
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
 * Updates the coloff or rowoff variables from the config,
 * when the cx or cy reaches to the edge of the screen.
 */
void update_scroll();

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

/* --- file i/o --- */

/*
 * Opens a file with the given filename, and then appends
 * all the lines to the editor_row array.
 * It will receive the filename as parameter.
 */
void editor_open(char *filename);

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

int main(int argc, char *argv[]) {
  enable_raw_mode();
  init();

  if (argc >= 2) {
    editor_open(argv[1]);
  }

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

void append_erow(char *s, size_t len) {
  config.editor_rows =
      realloc(config.editor_rows, sizeof(erow) * (config.numrows + 1));

  erow new_row;
  new_row.size = len;
  new_row.chars = malloc(len + 1);
  new_row.rsize = 0;
  new_row.render = NULL;

  memcpy(new_row.chars, s, len);
  new_row.chars[len] = '\0';
  update_erow(&new_row);
  config.editor_rows[config.numrows] = new_row;

  config.numrows++;
}

int row_cx_to_rx(erow *row, int cx) {
  int rx = 0;

  for (int i = 0; i < cx; i++) {
    if (row->chars[i] == '\t') {
      rx += (TAB_STOP - 1) + (rx % TAB_STOP);
    }

    rx++;
  }

  return rx;
}

void update_erow(erow *row) {
  int tabs = 0;

  for (int i = 0; i < row->size; i++) {
    if (row->chars[i] == '\t')
      tabs++;
  }

  free(row->render);
  row->render = malloc(row->size + (tabs * (TAB_STOP - 1)) + 1);

  int j = 0;
  for (int i = 0; i < row->size; i++) {
    if (row->chars[i] == '\t') {
      row->render[j++] = ' ';

      while (j % TAB_STOP != 0)
        row->render[j++] = ' ';
    } else {
      row->render[j++] = row->chars[i];
    }
  }

  row->render[j] = '\0';
  row->rsize = j;
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
    int filerow = y + config.rowoff;

    if (filerow >= config.numrows) {
      if (config.numrows == 0 && y == config.rows / 3) {
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
    } else {
      int len = config.editor_rows[filerow].rsize - config.coloff;
      if (len < 0)
        len = 0;
      if (len > config.cols)
        len = config.cols;

      ap_buf_append(buf, &config.editor_rows[filerow].render[config.coloff],
                    len);
    }

    // clears each line
    ap_buf_append(buf, "\x1b[K", 3);

    ap_buf_append(buf, "\r\n", 2);
  }
}

void update_scroll() {
  config.rx = 0;

  if (config.cy < config.numrows) {
    config.rx = row_cx_to_rx(&config.editor_rows[config.cy], config.cx);
  }

  if (config.cy < config.rowoff) {
    config.rowoff = config.cy;
  }

  if (config.cy >= config.rowoff + config.rows) {
    config.rowoff = config.cy - config.rows + 1;
  }

  if (config.rx < config.coloff) {
    config.coloff = config.rx;
  }

  if (config.rx >= config.coloff + config.cols) {
    config.coloff = config.rx - config.cols + 1;
  }
}

void refresh_screen() {
  struct ap_buf buf = AP_BUF_INIT;

  update_scroll();

  // hides the cursor
  ap_buf_append(&buf, "\x1b[?25l", 6);

  // moves cursor to the top
  ap_buf_append(&buf, "\x1b[H", 3);

  draw_rows(&buf);

  // moves cursor to the defined position in the config struct
  char temp_buf[32];
  snprintf(temp_buf, sizeof(temp_buf), "\x1b[%d;%dH",
           config.cy - config.rowoff + 1, config.rx - config.coloff + 1);
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

void editor_open(char *filename) {
  FILE *f = fopen(filename, "r");

  if (!f)
    die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  size_t linelen;

  while ((linelen = getline(&line, &linecap, f)) != -1) {
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
      linelen--;
    }

    append_erow(line, linelen);
  }

  free(line);
  fclose(f);
}

void update_cursor_pos(int key) {
  erow *current_row =
      config.cy >= config.numrows ? NULL : &config.editor_rows[config.cy];

  switch (key) {
  case ARROW_RIGHT: {
    if (current_row && config.cx < current_row->size) {
      config.cx++;
    } else if (current_row && config.cx == current_row->size) {
      config.cy++;
      config.cx = 0;
    }
    break;
  }
  case ARROW_LEFT: {
    if (config.cx > 0) {
      config.cx--;
    } else if (config.cy > 0) {
      config.cy--;
      config.cx = config.editor_rows[config.cy].size;
    }
    break;
  }
  case ARROW_UP: {
    if (config.cy > 0) {
      config.cy--;
    }
    break;
  }
  case ARROW_DOWN: {
    if (config.cy < config.numrows) {
      config.cy++;
    }
    break;
  }
  }

  current_row =
      config.cy >= config.numrows ? NULL : &config.editor_rows[config.cy];
  int rowlen = current_row ? current_row->size : 0;

  if (config.cx > rowlen) {
    config.cx = rowlen;
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
      if (esc[1] >= '0' && esc[1] <= '9') {
        if (read(STDIN_FILENO, &esc[2], 1) != 1)
          return '\x1b';

        if (esc[2] == '~') {
          switch (esc[1]) {
          case '1':
            return HOME_KEY;
          case '4':
            return END_KEY;
          case '5':
            return PAGE_UP;
          case '6':
            return PAGE_DOWN;
          case '7':
            return HOME_KEY;
          case '8':
            return END_KEY;
          }
        }

      } else {
        switch (esc[1]) {
        case 'A':
          return ARROW_UP;
        case 'B':
          return ARROW_DOWN;
        case 'C':
          return ARROW_RIGHT;
        case 'D':
          return ARROW_LEFT;
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    } else if (esc[0] == 'O') {
      switch (esc[1]) {
      case 'H':
        return HOME_KEY;
      case 'F':
        return END_KEY;
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

  case HOME_KEY:
    config.cx = 0;
    break;
  case END_KEY:
    if (config.cy < config.numrows)
      config.cx = config.editor_rows[config.cy].size;
    break;

  case PAGE_UP:
  case PAGE_DOWN: {
    int i = config.rows;
    while (i--) {
      update_cursor_pos(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
    break;
  }

  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT: {
    update_cursor_pos(key);
    break;
  }
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
  config.rx = 0;
  config.cy = 0;
  config.numrows = 0;
  config.editor_rows = NULL;
  config.rowoff = 0;
  config.coloff = 0;

  if (get_term_size(&config.rows, &config.cols) == -1)
    die("get_term_size");
}
