#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* ------ Macros and definitions ------ */

#define CTRL_KEY(k) ((k) & 0x1f)

#define VERSION "0.0.1"

#define TAB_STOP 4

#define FORCE_QUIT_TIMES 2

/* ------ Types ------ */

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

typedef struct search_match {
  int cx;
  int cy;
} search_match;

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
  int modified;
  erow *editor_rows;
  char *filename;
  char status_msg[160];
  time_t status_time;
  search_match *search_matches;
  int search_match_found;
  int current_search_idx;
  struct termios orig_termios;
};

struct conf config;

enum editor_keys {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DEL_KEY
};

/* ------ Appendable buffer ------ */

struct ap_buf {
  char *b;
  int len;
};

#define AP_BUF_INIT {NULL, 0}

/* ------ Function declarations ------ */

/* --- searching --- */

/*
 * Computes LPS array for KMP pattern matching algorithm.
 */
int *compute_lps(char *pattern, size_t len);

/*
 * KMP matching algorithm function.
 * It will receive a string, a pattern, length of the string,
 * length of the pattern and a integer pointer for setting the
 * number of matches that has been found.
 * It will return an array of integer containing the indexes
 * which the pattern has been found within the string.
 */
int *kmp_matching(char *str, char *pattern, size_t slen, size_t plen,
                  int *matches_len);

/*
 * It will prompt the user to enter a pattern for searching
 * through the current file.
 */
void editor_search();

/*
 * It will move the cursor at the front of the matching pattern
 * of the search result. It will receive the index of the matches.
 */
void move_cursor_to_search_match(int match_idx);

/*
 * Increments the match index for the search result.
 * Also moves the cursor at that position.
 */
void increment_search();

/*
 * Decrements the match index for the search result.
 * Also moves the cursor at that position.
 */
void decrement_search();

/* --- editor rows --- */

/*
 * Appends a new editor row to the editor_row array in the
 * editor config struct.
 * It will receive the line string and the line length.
 */
void append_erow(char *s, size_t len);

/*
 * Inserts a new editor row at the given position with
 * the given string. It will receive the position, the string
 * contents of the new row and the length of the string.
 */
void insert_erow(int at, char *s, size_t len);

/*
 * Will delete a row at the given index.
 * It will free the row and move the rows on step backward.
 * It will receive the row index.
 */
void delete_erow(int at);

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

/*
 * Converts rx into cx by counting the tab stops.
 * It will receive the row pointer and the current rx.
 */
int row_rx_to_cx(erow *row, int rx);

/*
 * Inserts a character at the given row and the given
 * position. It will receive the row pointer, position index,
 * and the character.
 */
void insert_char_at_row(erow *row, int at, char c);

/*
 * Inserts a string at the given row and the given
 * position. It will receive the row pointer, position index,
 * the string and the length of the string.
 */
void insert_str_at_row(erow *row, int at, char *c, size_t len);

/*
 * Removes a character at the given row and the given position.
 * It will receive the row pointer and the position index.
 */
void remove_char_at_row(erow *row, int at);

/* --- editor operations --- */

/*
 * Converts all editor_rows elements into a one
 * single buffer, and returns it. Also set the buflen
 * parameter to the buffer length.
 * It will receive the buflen parameter and returns a string.
 */
char *erows_to_str(int *buflen);

/*
 * Inserts a character in the editor screen by using
 * the insert_char_at_row function.
 * If the cursor is at the end of file it will append a row.
 * It will receive the character.
 */
void insert_char(int c);

/*
 * Inserts a new line in the editor screen at the cursor position.
 * If there is any remaining character at the front of the cursor,
 * it will put them on the new line.
 */
void insert_new_line();

/*
 * Deletes a character from the editor screen.
 * It will delete the character of a row
 * until there is no character left behind the cursor,
 * after that it will append the remaining characters after
 * the cursor to the previous row.
 */
void delete_char();

/*
 * Gives a prompt on the status message line for the user,
 * and waits for the user to press enter, and the returns
 * the value of that prompt. If user cancels the prompt
 * with ESCAPE key it will return NULL pointer.
 * It will receive the prompt and a default value for the prompt.
 * Remember to include %s at the end of the prompt for the formatting
 * to work properly.
 */
char *editor_prompt(char *prompt, char *default_value);

/* --- appendable buffer --- */

/*
 * Append a string to the appendable buffer. It will receive
 * the buffer struct pointer, new string and the new string length.
 */
void ap_buf_append(struct ap_buf *buf, const char *s, size_t len);

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
 * It will receive the x and y of the position.
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
 * Sets the status message based on the given formatted string.
 * Also updates the status time.
 * It will receive the formatted string alongside with the
 * string arguments.
 */
void set_status_msg(const char *fmt, ...);

/*
 * Draws the status line in the bottom of the terminal screen.
 * It will receive the appendable buffer pointer.
 */
void draw_status_line(struct ap_buf *buf);

/*
 * It will draw the status line for showing the status message
 * below the main status line.
 * It will receive the appendable buffer pointer.
 */
void draw_status_msg_line(struct ap_buf *buf);

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
 * Disables the raw mode in the terminal.
 */
void disable_raw_mode();

/*
 * Enables the raw mode in the terminal.
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

/*
 * Saves the current buffer into the file.
 */
void editor_save();

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

  set_status_msg("HELP: Ctrl-S = save | Ctrl-Q = quit");

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

int *compute_lps(char *pattern, size_t len) {
  int i = 1;
  int j = 0;
  int *lps = malloc(sizeof(int) * len);
  lps[0] = 0;

  while (i < len) {
    if (pattern[i] == pattern[j]) {
      j++;
      lps[i] = j;
      i++;
    } else {
      if (j != 0) {
        j = lps[j - 1];
      } else {
        i++;
      }
    }
  }

  return lps;
}

int *kmp_matching(char *str, char *pattern, size_t slen, size_t plen,
                  int *matches_len) {
  int matches_idx = 0;
  int m_len = 5;
  int *matches = malloc(sizeof(int) * m_len);

  for (int i = 0; i < m_len; i++) {
    matches[i] = -1;
  }

  int *lps = compute_lps(pattern, plen);

  int i = 0;
  int j = 0;

  while ((slen - i) >= (plen - j)) {
    if (i >= slen) {
      free(lps);
      return matches;
    }

    if (str[i] == pattern[j]) {
      j++;
      i++;
    }

    if (j == plen) {
      if (matches_idx >= m_len - 1) {
        m_len *= 2;
        matches = realloc(matches, sizeof(int) * m_len);

        for (int k = matches_idx; k < m_len; k++) {
          matches[k] = -1;
        }
      }

      matches[matches_idx++] = i - j;
      j = 0;
    } else if (i < slen && str[i] != pattern[j]) {
      if (j != 0) {
        j = lps[j - 1];
      } else {
        i++;
      }
    }
  }

  free(lps);
  *matches_len = matches_idx;
  return matches;
}

void editor_search() {
  char *pattern = editor_prompt("Search: %s", "");

  if (pattern == NULL)
    return;

  size_t plen = strlen(pattern);
  if (config.search_matches) {
    free(config.search_matches);
  }

  int matches_len = 10;
  int found_match = 0;
  config.search_matches = malloc(sizeof(search_match) * matches_len);

  for (int i = 0; i < config.numrows; i++) {
    erow *row = &config.editor_rows[i];
    int m_len;
    int *matches = kmp_matching(row->render, pattern, row->rsize, plen, &m_len);

    if (m_len == 0)
      continue;

    for (int j = 0; j < m_len; j++) {
      search_match new_match;
      new_match.cx = row_rx_to_cx(row, matches[j] + plen);
      new_match.cy = i;

      if (matches_len - 1 <= found_match) {
        matches_len *= 2;
        config.search_matches =
            realloc(config.search_matches, sizeof(search_match) * matches_len);
      }

      config.search_matches[found_match++] = new_match;
    }
  }

  config.search_match_found = found_match;
  move_cursor_to_search_match(0);
}

void move_cursor_to_search_match(int match_idx) {
  if (match_idx < 0 || match_idx + 1 > config.search_match_found)
    return;

  search_match match = config.search_matches[match_idx];
  config.cx = match.cx;
  config.cy = match.cy;
  config.current_search_idx = match_idx;
}

void increment_search() {
  int new_idx = (config.current_search_idx + 1) % config.search_match_found;

  move_cursor_to_search_match(new_idx);
}

void decrement_search() {
  int new_idx =
      config.current_search_idx == 0
          ? config.search_match_found - 1
          : (config.current_search_idx - 1) % config.search_match_found;

  move_cursor_to_search_match(new_idx);
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
  config.modified++;
}

void insert_erow(int at, char *s, size_t len) {
  if (at < 0 || at > config.numrows)
    return;

  config.editor_rows =
      realloc(config.editor_rows, sizeof(erow) * (config.numrows + 1));

  memmove(&config.editor_rows[at + 1], &config.editor_rows[at],
          sizeof(erow) * (config.numrows - at));

  erow new_row;
  new_row.size = len;
  new_row.chars = malloc(len + 1);
  new_row.rsize = 0;
  new_row.render = NULL;

  memcpy(new_row.chars, s, len);
  new_row.chars[len] = '\0';
  update_erow(&new_row);
  config.editor_rows[at] = new_row;

  config.numrows++;
  config.modified++;
}

void delete_erow(int at) {
  if (at < 0 || at > config.numrows)
    return;

  erow *row = &config.editor_rows[at];

  free(row->chars);
  free(row->render);
  memmove(&config.editor_rows[at], &config.editor_rows[at + 1],
          sizeof(erow) * (config.numrows - at - 1));
  config.numrows--;
  config.modified++;
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

int row_rx_to_cx(erow *row, int rx) {
  int current_cx = 0;
  int cx;

  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t') {
      current_cx += (TAB_STOP - 1) - (current_cx % TAB_STOP);
    }

    current_cx++;

    if (current_cx > rx)
      return cx;
  }

  return cx;
}

void insert_char_at_row(erow *row, int at, char c) {
  if (at < 0 || at > row->size)
    at = row->size;

  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  update_erow(row);
  config.modified++;
}

void insert_str_at_row(erow *row, int at, char *c, size_t len) {
  if (at < 0 || at > row->size)
    return;

  row->chars = realloc(row->chars, row->size + len + 1);

  memmove(&row->chars[at + len], &row->chars[at], row->size - at + 1);
  memcpy(&row->chars[at], c, len);

  row->size += len;
  update_erow(row);
  config.modified++;
}

void remove_char_at_row(erow *row, int at) {
  if (at < 0 || at >= row->size)
    return;

  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  update_erow(row);
  config.modified++;
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

void ap_buf_append(struct ap_buf *buf, const char *s, size_t len) {
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

void set_status_msg(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(config.status_msg, sizeof(config.status_msg), fmt, ap);
  va_end(ap);
  config.status_time = time(NULL);
}

void draw_status_line(struct ap_buf *buf) {
  ap_buf_append(buf, "\x1b[7m", 4);

  char status[160], rstatus[80];

  int len = snprintf(status, sizeof(status), "%.20s %s - %d lines",
                     config.filename ? config.filename : "[No Name]",
                     config.modified ? "(modified)" : "", config.numrows);

  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", config.cy + 1,
                      config.numrows);

  if (len > config.cols)
    len = config.cols;

  ap_buf_append(buf, status, len);

  while (len < config.cols) {
    if (config.cols - len == rlen) {
      ap_buf_append(buf, rstatus, rlen);
      break;
    } else {
      ap_buf_append(buf, " ", 1);
      len++;
    }
  }

  ap_buf_append(buf, "\x1b[m", 3);
  ap_buf_append(buf, "\r\n", 2);
}

void draw_status_msg_line(struct ap_buf *buf) {
  ap_buf_append(buf, "\x1b[K", 3);
  int msg_len = strlen(config.status_msg);

  if (msg_len > config.cols)
    msg_len = config.cols;

  if (msg_len && time(NULL) - config.status_time < 7) {
    ap_buf_append(buf, config.status_msg, msg_len);
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
  draw_status_line(&buf);
  draw_status_msg_line(&buf);

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

char *erows_to_str(int *buflen) {
  char *buffer = NULL;

  int total_len = 0;

  for (int i = 0; i < config.numrows; i++) {
    erow row = config.editor_rows[i];
    total_len += row.size + 1;
  }

  *buflen = total_len;
  buffer = malloc(total_len);
  char *p = buffer;

  for (int i = 0; i < config.numrows; i++) {
    erow row = config.editor_rows[i];
    memcpy(p, row.chars, row.size);
    p += row.size;
    *p = '\n';
    p++;
  }

  return buffer;
}

void insert_char(int c) {
  if (config.cy == config.numrows) {
    insert_erow(config.numrows, "", 0);
  }

  insert_char_at_row(&config.editor_rows[config.cy], config.cx, c);
  config.cx++;
}

void insert_new_line() {
  if (config.cx == 0) {
    insert_erow(config.cy, "", 0);

  } else {
    erow *row = &config.editor_rows[config.cy];
    insert_erow(config.cy + 1, &row->chars[config.cx], row->size - config.cx);
    row = &config.editor_rows[config.cy];
    row->size = config.cx;
    row->chars[row->size] = '\0';
    update_erow(row);
  }

  config.cy++;
  config.cx = 0;
}

void delete_char() {
  if (config.cy == config.numrows)
    return;

  erow *row = &config.editor_rows[config.cy];

  if (config.cy == 0 && config.cx == 0) {
    if (row->size == 0)
      delete_erow(config.cy);

    return;
  }

  if (config.cx > 0) {
    remove_char_at_row(row, config.cx - 1);
    config.cx--;
  } else {
    erow *prev_row = &config.editor_rows[config.cy - 1];
    config.cx = prev_row->size;
    insert_str_at_row(prev_row, prev_row->size, row->chars, row->size);
    delete_erow(config.cy);
    config.cy--;
  }
}

char *editor_prompt(char *prompt, char *default_value) {
  size_t bufsize = 128;
  size_t buflen = strlen(default_value);

  while (buflen > bufsize) {
    if (bufsize > 256)
      break;
    bufsize *= 2;
  }

  char *buf = malloc(bufsize);

  for (int i = 0; i < buflen; i++) {
    buf[i] = default_value[i];
  }

  buf[buflen] = '\0';

  while (1) {
    set_status_msg(prompt, buf);
    refresh_screen();

    int c = read_input_key();

    if (c == DEL_KEY || c == BACKSPACE) {
      if (buflen != 0)
        buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      set_status_msg("");
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        set_status_msg("");
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }

      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
  }
}

void editor_open(char *filename) {
  free(config.filename);
  config.filename = strdup(filename);

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

  config.modified = 0;
  free(line);
  fclose(f);
}

void editor_save() {
  char *temp_filename = NULL;

  if (config.filename == NULL) {
    temp_filename = editor_prompt("Save as: %s (Esc to cancel)", "");
  } else {
    temp_filename =
        editor_prompt("Save as: %s (Esc to cancel)", config.filename);
  }

  if (temp_filename == NULL) {
    set_status_msg("Save operation cancelled.");
    return;
  }

  int len;
  char *buf = erows_to_str(&len);

  int fd = open(temp_filename, O_RDWR | O_CREAT, 0644);

  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        config.filename = temp_filename;

        set_status_msg("%d bytes saved on %s.", len, config.filename);

        config.modified = 0;
        return;
      }
    }
    close(fd);
  }

  free(buf);
  free(temp_filename);
  set_status_msg("Error on save: %s", strerror(errno));
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
          case '3':
            return DEL_KEY;
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
  static int quit_count = FORCE_QUIT_TIMES;
  int key = read_input_key();

  switch (key) {

  case CTRL_KEY('w'): {
    editor_save();
    break;
  }

  case CTRL_KEY('f'): {
    editor_search();
    break;
  }

  case CTRL_KEY('p'): {
    decrement_search();
    break;
  }

  case CTRL_KEY('n'): {
    increment_search();
    break;
  }

  case CTRL_KEY('q'): {
    if (config.modified > 0 && quit_count > 0) {
      set_status_msg("The file has unsaved changes, if you want to force quit "
                     "press Ctrl-Q %d times more.",
                     quit_count);
      quit_count--;
      return;
    }

    if (config.search_matches) {
      free(config.search_matches);
    }

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

  case DEL_KEY:
  case BACKSPACE: {
    if (key == DEL_KEY)
      update_cursor_pos(ARROW_RIGHT);

    delete_char();
    break;
  }

  case '\r': {
    insert_new_line();
    break;
  }

  case CTRL_KEY('l'):
  case '\x1b':
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

  default: {
    insert_char(key);
    break;
  }
  }

  quit_count = FORCE_QUIT_TIMES;
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
  config.filename = NULL;
  config.status_msg[0] = '\0';
  config.status_time = 0;
  config.modified = 0;
  config.search_matches = NULL;
  config.search_match_found = -1;
  config.current_search_idx = -1;

  if (get_term_size(&config.rows, &config.cols) == -1)
    die("get_term_size");

  config.rows -= 2;
}
