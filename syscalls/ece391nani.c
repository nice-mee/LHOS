/* ece391nani.c -- source code for text editor nani */
#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"
#include "ece391vt.h"

int32_t ece391_memcpy(void* dest, const void* src, int32_t n);
void ece391_memset(void* memory, char c, int n);
int32_t ece391_getline(char* buf, int32_t fd, int32_t n);

#define NULL 0
#define NUM_TERM_COLS    (80 - 1)
#define NUM_TERM_ROWS    (25 - 2)
#define MAX_COLS         1200
#define MAX_ROWS         1000
#define NANI_STATIC_BUF_ADDR 0x07000000
#define NANI_FILEBUF_ADDR 0x07C00000
#define NANI_STATUS_BAR_LINEINFO_START 55
#define NANI_TAB_SIZE 4
#define COMMAND_BUF_SIZE 51
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

static int8_t keycode_to_printable_char[2][128] =
{
    { // Unshifted version
        '\0',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k','l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u','v', 'w', 'x', 'y', 'z',
        '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/', ' ',
        // Remaining keys are not used but will be mapped to '\0' anyway
    },
    { // Shifted version
        '0',
        ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K','L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U','V', 'W', 'X', 'Y', 'Z',
        '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?', ' ',
        // Remaining keys are not used but will be mapped to '\0' anyway
    }
};

enum NANI_mode {
    NANI_NORMAL,
    NANI_INSERT,
    NANI_COMMAND,
    NANI_SEARCH,
};

enum NANI_highlight {
    HL_NORMAL,
    HL_NUMBER,
    HL_SEARCH_MATCH,
    HL_STRING,
    HL_COMMENT,
    HL_MULTI_COMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2
};

typedef struct {
    int shift;
    int caps;
    int ctrl;
    int alt;
} keyboard_state_t;

typedef struct erow {
    int size;
    int rsize;
    char chars[MAX_COLS];
    char render[NANI_TAB_SIZE * MAX_COLS];
    char hl[NANI_TAB_SIZE * MAX_COLS];
    int hl_open_comment;
} erow_t;

typedef struct {
    char command[COMMAND_BUF_SIZE];
    int len;
} command_t;

struct append_buf {
    int len;
    int cap;
    char buf[10240];
};

struct editor_syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int flags;
};

typedef struct nani_state {
    int screen_x, screen_y;
    int render_x;
    int rowoff;
    int coloff;
    enum NANI_mode mode;
    keyboard_state_t kbd;
    int numrows;
    erow_t *row;
    command_t command;
    char statusmsg[50];
    int search_direction;
    // int search_last_match;
    struct append_buf abuf;
    char filename[33];
    struct editor_syntax *syntax;
    int dirty;
} nani_state_t;

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

static char *C_HL_extensions[] = {".c", ".h", ".cpp", NULL};

static char *C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case",

    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL
};

static char *PY_HL_extensions[] = {".py", NULL};

static char *PY_HL_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
    "else", "except", "exec", "finally", "for", "from", "global", "if", "import",
    "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try",
    "while", "with", "yield", 
    
    NULL
};


static struct editor_syntax HLDB[] = {
    {
        "Plain Text",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0
    },
    {
        "C/C++",
        C_HL_extensions,
        C_HL_keywords,
        "//",
        "/*",
        "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "Python",
        PY_HL_extensions,
        PY_HL_keywords,
        "#",
        "\"\"\"",
        "\"\"\"",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    }
};

char itoa(int n) {
    if (n < 0 || n > 9) return '\0';
    return n + '0';
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int is_seperator(char c) {
    return c == ' ' || c == '\t' || c == '\0' || c == '\n' || c == ',' || c == ';' || c == '.' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '/' || c == '*' || c == '+' || c == '-' || c == '=' || c == '<' || c == '>' || c == '&' || c == '|' || c == '!' || c == '?' || c == ':' || c == '`';
}

nani_state_t NANI;

void command_insert_char(char c) {
    if (NANI.command.len >= COMMAND_BUF_SIZE - 1) return;
    NANI.command.command[NANI.command.len] = c;
    NANI.command.len++;
}

void command_backspace() {
    if (NANI.command.len == 0) return;
    NANI.command.len--;
}

void command_clear() {
    ece391_memset(NANI.command.command, 0, COMMAND_BUF_SIZE);
    NANI.command.len = 0;
}

void abuf_init(struct append_buf *ab) {
    // ab->buf = ece391_malloc(32);
    ab->len = 0;
    ab->cap = 32;
}

void abuf_append(struct append_buf *ab, const char *s, int len) {
    if (ab->len + len > ab->cap) {
        while (ab->len + len > ab->cap) {
            ab->cap *= 2;
        }
        // char *new_buf = ece391_malloc(ab->cap);
        // ece391_memcpy(new_buf, ab->buf, ab->len);
        // ece391_free(ab->buf);
        // ab->buf = new_buf;
    }
    ece391_memcpy(&ab->buf[ab->len], s, len);
    ab->len += len;
}

void abuf_clear(struct append_buf *ab) {
    ab->len = 0;
}

void NANI_syntax_to_color(int hl) {
    switch (hl) {
        case HL_NORMAL:
            abuf_append(&NANI.abuf, "\x1b[37;40M", 8);
            break;
        case HL_NUMBER:
            abuf_append(&NANI.abuf, "\x1b[35;40M", 8);
            break;
        case HL_SEARCH_MATCH:
            abuf_append(&NANI.abuf, "\x1b[30;43M", 8);
            break;
        case HL_STRING:
            abuf_append(&NANI.abuf, "\x1b[32;40M", 8);
            break;
        case HL_COMMENT:
            abuf_append(&NANI.abuf, "\x1b[33;40M", 8);
            break;
        case HL_KEYWORD1:
            abuf_append(&NANI.abuf, "\x1b[94;40M", 8);
            break;
        case HL_KEYWORD2:
            abuf_append(&NANI.abuf, "\x1b[91;40M", 8);
            break;
        case HL_MULTI_COMMENT:
            abuf_append(&NANI.abuf, "\x1b[33;40M", 8);
            break;
        default:
            abuf_append(&NANI.abuf, "\x1b[37;40M", 8);
            break;
    }
}

void NANI_select_syntax() {
    NANI.syntax = &HLDB[0];
    if (NANI.filename[0] == '\0') return;

    int i, len = ece391_strlen((uint8_t *)NANI.filename);
    for (i = len - 1; i >= 0; i--) {
        if (NANI.filename[i] == '.') break;
    }
    if (i == -1) return;

    int j;
    for (j = 1; j < HLDB_ENTRIES; j++) {
        char **exts = HLDB[j].filematch;
        while (*exts) {
            if (ece391_strcmp((uint8_t *)&NANI.filename[i], (uint8_t *)*exts) == 0) {
                NANI.syntax = &HLDB[j];
                return;
            }
            exts++;
        }
    }
}

void erow_update_syntax(erow_t *row) {
    ece391_memset(row->hl, HL_NORMAL, row->rsize);

    char **keywords = NANI.syntax->keywords;

    char *scs = NANI.syntax->singleline_comment_start;
    char *mcs = NANI.syntax->multiline_comment_start;
    char *mce = NANI.syntax->multiline_comment_end;
    int scs_len = scs ? ece391_strlen((uint8_t *)scs) : 0;
    int mcs_len = mcs ? ece391_strlen((uint8_t *)mcs) : 0;
    int mce_len = mce ? ece391_strlen((uint8_t *)mce) : 0;

    int i = 0;
    int prev_is_seperator = 1;
    int in_hex = 0;
    int in_string = 0;
    int row_idx = (row - NANI.row);
    int in_comment = row_idx > 0 && NANI.row[row_idx - 1].hl_open_comment;

    while (i < row->rsize) {
        char c = row->render[i];
        // Highlight comments
        if (scs_len && !in_string && i + scs_len <= row->rsize && scs != NULL && !in_comment) {
            if (!ece391_strncmp((uint8_t *)&row->render[i], (uint8_t *)scs, scs_len)) {
                ece391_memset((uint8_t *)&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string && mcs != NULL && mce != NULL) {
            if (in_comment) {
                row->hl[i] = HL_MULTI_COMMENT;
                if (!ece391_strncmp((uint8_t *)&row->render[i], (uint8_t *)mce, mce_len)) {
                    ece391_memset(&row->hl[i], HL_MULTI_COMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_is_seperator = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!ece391_strncmp((uint8_t *)&row->render[i], (uint8_t *)mcs, mcs_len)) {
                ece391_memset(&row->hl[i], HL_MULTI_COMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        // Highlight strings
        if (NANI.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_is_seperator = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        // Highlight numbers
        if (NANI.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if (in_hex && is_hex_digit(c)) {
                row->hl[i] = HL_NUMBER;
                i++;
                continue;
            }
            if (i + 2 < row->rsize && c == '0' && row->render[i + 1] == 'x' && is_digit(row->render[i + 2]) && (prev_is_seperator || (i > 0 && row->hl[i - 1] == HL_NUMBER))) {
                row->hl[i] = HL_NUMBER;
                row->hl[i + 1] = HL_NUMBER;
                row->hl[i + 2] = HL_NUMBER;
                prev_is_seperator = 0;
                i += 3;
                in_hex = 1;
                continue;
            }
            if ((is_digit(c) && (prev_is_seperator || (i > 0 && row->hl[i - 1] == HL_NUMBER))) || (i > 0 && c == '.' && row->hl[i-1] == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                prev_is_seperator = 0;
                i++;
                continue;
            }
        }
        in_hex = 0;

        // Highlight keywords
        if (prev_is_seperator && keywords != NULL) {
            int j;
            for (j = 0; keywords[j] != NULL; j++) {
                int klen = ece391_strlen((uint8_t *)keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;

                if (i + klen <= row->rsize && !ece391_strncmp((uint8_t *)&row->render[i], (uint8_t *)keywords[j], klen) && is_seperator(row->render[i + klen])) {
                    ece391_memset((uint8_t *)&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }

            if (keywords[j] != NULL) {
                prev_is_seperator = 0;
                continue;
            }
        }

        prev_is_seperator = is_seperator(c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row_idx + 1 < NANI.numrows)
        erow_update_syntax(&NANI.row[row_idx + 1]);
}

void erow_update_render(erow_t *row) {
    int i, j=0;
    for (i = 0; i < row->size; i++) {
        if (row->chars[i] == '\t') {
            row->render[j++] = ' ';
            while (j % NANI_TAB_SIZE != 0) {
                row->render[j++] = ' ';
            }
        } else {
            row->render[j++] = row->chars[i];
        }
    }
    row->rsize = j;

    erow_update_syntax(row);
}

int erow_sx2rx(erow_t *row, int sx) {
    int rx = 0;
    int j;
    for (j = 0; j < sx; j++) {
        if (row->chars[j] == '\t') {
            rx += (NANI_TAB_SIZE - 1) - (rx % NANI_TAB_SIZE);
        }
        rx++;
    }
    return rx;
}

int erow_rx2sx(erow_t *row, int rx) {
    int cur_rx = 0;
    int sx;
    for (sx = 0; sx < row->size; sx++) {
        if (row->chars[sx] == '\t') {
            cur_rx += (NANI_TAB_SIZE - 1) - (cur_rx % NANI_TAB_SIZE);
        }
        cur_rx++;
        if (cur_rx > rx) return sx;
    }
    return sx;
}

void erow_append_string(erow_t *row, char *s, int32_t len) {
    int at = row->size;
    row->size += len;
    ece391_memcpy(&row->chars[at], s, len);
    erow_update_render(row);
    NANI.dirty++;
}

void erow_insert_char(erow_t *row, int at, char c) {
    if (at >= MAX_COLS) return;
    if (at < 0 || at > row->size) at = row->size;
    int i;
    for (i = row->size; i > at; i--) {
        row->chars[i] = row->chars[i - 1];
    }
    row->chars[at] = c;
    row->size++;
    erow_update_render(row);
    NANI.dirty++;
}


void erow_delete_char(erow_t *row, int at) {
    if (at < 0 || at >= row->size) return;
    int i;
    for (i = at; i < row->size - 1; i++) {
        row->chars[i] = row->chars[i + 1];
    }
    row->size--;
    erow_update_render(row);
    NANI.dirty++;
}

int erow_to_filebuf() {
    char *p = (char *)NANI_FILEBUF_ADDR;
    int i, len = 0;
    for (i = 0; i < NANI.numrows; i++) {
        ece391_memcpy(p, NANI.row[i].chars, NANI.row[i].size);
        p += NANI.row[i].size;
        *p = '\n';
        p++;
        len += NANI.row[i].size + 1;
    }
    return len;
}

static int32_t NANI_save() {
    int32_t fd;
    int32_t len = erow_to_filebuf();
    if (-1 == (fd = ece391_open (NANI.filename))) {
    }
    ece391_write(fd, (uint8_t *)NANI_FILEBUF_ADDR, len);
    ece391_close(fd);
    NANI.dirty = 0;
    return 0;
}

void NANI_delete_row(int at) {
    if (at < 0 || at >= NANI.numrows) return;
    int i;
    for (i = at; i < NANI.numrows - 1; i++) {
        ece391_memcpy(&NANI.row[i], &NANI.row[i + 1], sizeof(erow_t));
    }
    NANI.numrows--;
    NANI.dirty++;
}


void NANI_insert_row(int at, char *s, int32_t len) {
    if (at < 0 || at > NANI.numrows || at >= MAX_ROWS) return;
    int i;
    for (i = NANI.numrows; i > at; i--) {
        ece391_memcpy(&NANI.row[i], &NANI.row[i - 1], sizeof(erow_t));
    }

    NANI.row[at].size = len;
    ece391_memcpy(NANI.row[at].chars, s, len);
    NANI.row[at].rsize = 0;
    NANI.row[at].hl_open_comment = 0;
    erow_update_render(&NANI.row[at]);

    NANI.numrows++;
    NANI.dirty++;
}

void NANI_delete_char() {
    if (NANI.screen_y == NANI.numrows) return;
    if (NANI.screen_x == 0 && NANI.screen_y == 0) return;
    erow_t *row = &NANI.row[NANI.screen_y];
    if (NANI.screen_x > 0) {
        erow_delete_char(row, NANI.screen_x - 1);
        NANI.screen_x--;
    } else {
        if (NANI.row[NANI.screen_y - 1].size + NANI.row[NANI.screen_y].size > MAX_COLS) return;
        NANI.screen_x = NANI.row[NANI.screen_y - 1].size;
        erow_append_string(&NANI.row[NANI.screen_y - 1], row->chars, row->size);
        NANI_delete_row(NANI.screen_y);
        NANI.screen_y--;
    }
}

void NANI_insert_newline() {
    if (NANI.screen_x == 0) {
        NANI_insert_row(NANI.screen_y, "", 0);
    } else {
        erow_t *row = &NANI.row[NANI.screen_y];
        NANI_insert_row(NANI.screen_y + 1, &row->chars[NANI.screen_x], row->size - NANI.screen_x);
        row->size = NANI.screen_x;
        erow_update_render(row);
    }
    NANI.screen_y++;
    NANI.screen_x = 0;
}


static void NANI_clear_screen() {
    ece391_write(1, "\x1b[2J", 4);
    ece391_write(1, "\x1b[H", 3);
}

static void NANI_scroll() {
    NANI.render_x = 0;
    if (NANI.screen_y < NANI.numrows) {
        NANI.render_x = erow_sx2rx(&NANI.row[NANI.screen_y], NANI.screen_x);
    }
    if (NANI.screen_y < NANI.rowoff) {
        NANI.rowoff = NANI.screen_y;
    }
    if (NANI.screen_y >= NANI.rowoff + NUM_TERM_ROWS) {
        NANI.rowoff = NANI.screen_y - NUM_TERM_ROWS + 1;
    }
    if (NANI.render_x < NANI.coloff) {
        NANI.coloff = NANI.render_x;
    }
    if (NANI.render_x >= NANI.coloff + NUM_TERM_COLS) {
        NANI.coloff = NANI.render_x - NUM_TERM_COLS + 1;
    }
}

static void NANI_draw_status_bar() {
    char status[79];
    ece391_memset(status, ' ', 79);
    if (NANI.statusmsg[0] != '\0') {
        ece391_memcpy(status, NANI.statusmsg, ece391_strlen((uint8_t *)NANI.statusmsg));
        abuf_append(&NANI.abuf, "\x1b[37;44M", 8);
    } else if (NANI.mode == NANI_NORMAL) {
        ece391_memcpy(status, "-- NORMAL --", 12);
    } else if (NANI.mode == NANI_INSERT) {
        ece391_memcpy(status, "-- INSERT --", 12);
    } else if (NANI.mode == NANI_COMMAND) {
        ece391_memcpy(status, NANI.command.command, NANI.command.len);
    } else if (NANI.mode == NANI_SEARCH) {
        ece391_memcpy(status, NANI.command.command, NANI.command.len);
    }
    abuf_append(&NANI.abuf, status, 50);
    abuf_append(&NANI.abuf, "\x1b[37;40M", 8);
    int i = NANI_STATUS_BAR_LINEINFO_START;
    status[i++] = itoa((NANI.screen_y + 1) / 1000);
    status[i++] = itoa(((NANI.screen_y + 1) / 100) % 10);
    status[i++] = itoa(((NANI.screen_y + 1) / 10) % 10);
    status[i++] = itoa((NANI.screen_y + 1) % 10);
    status[i++] = ',';
    status[i++] = itoa((NANI.screen_x + 1) / 1000);
    status[i++] = itoa(((NANI.screen_x + 1) / 100) % 10);
    status[i++] = itoa(((NANI.screen_x + 1) / 10) % 10);
    status[i++] = itoa((NANI.screen_x + 1) % 10);
    status[i++] = ' ';
    status[i++] = '|';
    status[i++] = ' ';
    for (; i < 79; i++) {
        if (NANI.syntax->filetype[i - NANI_STATUS_BAR_LINEINFO_START - 12] == '\0')
            break;
        status[i] = NANI.syntax->filetype[i - NANI_STATUS_BAR_LINEINFO_START - 12];
    }
    abuf_append(&NANI.abuf, &status[50], 29);
}

static void NANI_update_screen() {
    NANI_scroll();
    
    // Reset cursor to topleft
    abuf_append(&NANI.abuf, "\x1b[H", 3);

    // Draw Title
    abuf_append(&NANI.abuf, "\x1b[30;47M", 8); // Set color to black on white
    abuf_append(&NANI.abuf, "\x1b[K", 4);
    int left_padding = (NUM_TERM_COLS - ece391_strlen((uint8_t *)NANI.filename)) / 2;
    if (NANI.dirty) {
        abuf_append(&NANI.abuf, "[edited]", 8);
        left_padding -= 8;
    }
    while (left_padding--)
        abuf_append(&NANI.abuf, " ", 1);
    abuf_append(&NANI.abuf, NANI.filename, ece391_strlen((uint8_t *)NANI.filename));
    abuf_append(&NANI.abuf, "\x1b[37;40M", 8); // Reset color
    abuf_append(&NANI.abuf, "\n", 1);

    // Draw lines
    int scr_y;
    for (scr_y = 0; scr_y < NUM_TERM_ROWS; scr_y++) {
        int filerow = scr_y + NANI.rowoff;
        if (filerow >= NANI.numrows) {
            if (NANI.numrows == 0 && scr_y == NUM_TERM_ROWS / 3) {
                char welcome[NUM_TERM_COLS] = "NANI -- a simple editor";
                int left_padding = (NUM_TERM_COLS - ece391_strlen((uint8_t *)welcome)) / 2; 
                while (left_padding--)
                    abuf_append(&NANI.abuf, "~", 1);
                abuf_append(&NANI.abuf, welcome, ece391_strlen((uint8_t *)welcome));
            } else {
                abuf_append(&NANI.abuf, "~", 1);
            }
        } else {
            int len = NANI.row[filerow].rsize - NANI.coloff;
            if (len < 0)
                len = 0;
            if (len > NUM_TERM_COLS)
                len = NUM_TERM_COLS;
            char *c = &NANI.row[filerow].render[NANI.coloff];
            char *hl = &NANI.row[filerow].hl[NANI.coloff];
            int current_color = HL_NORMAL;
            int j;
            for (j = 0; j < len; j++) {
                if (current_color != hl[j]) {
                    current_color = hl[j];
                    NANI_syntax_to_color(current_color);
                }
                abuf_append(&NANI.abuf, &c[j], 1);
            }
            NANI_syntax_to_color(HL_NORMAL);
        }
        
        abuf_append(&NANI.abuf, "\x1b[K", 3);
        abuf_append(&NANI.abuf, "\n", 1);
    }

    NANI_draw_status_bar();

    // Draw cursor
    char cursor_cmd[8] = "\x1b[00;00H";
    cursor_cmd[2] = itoa((1 + NANI.screen_y - NANI.rowoff) / 10);
    cursor_cmd[3] = itoa((1 + NANI.screen_y - NANI.rowoff) % 10);
    cursor_cmd[5] = itoa((NANI.render_x - NANI.coloff) / 10);
    cursor_cmd[6] = itoa((NANI.render_x - NANI.coloff) % 10);
    abuf_append(&NANI.abuf, cursor_cmd, 8);

    ece391_write(1, NANI.abuf.buf, NANI.abuf.len);

    abuf_clear(&NANI.abuf);
}

static void NANI_move_cursor(char c) {
    erow_t *row = (NANI.screen_y >= NANI.numrows) ? NULL : &NANI.row[NANI.screen_y];

    switch (c) {
        case 'h':
            if (NANI.screen_x > 0)
                NANI.screen_x--;
            break;
        case 'j':
            if (NANI.screen_y < NANI.numrows)
                NANI.screen_y++;
            break;
        case 'k':
            if (NANI.screen_y > 0)
                NANI.screen_y--;
            break;
        case 'l':
            if (row && NANI.screen_x < row->size) {
                NANI.screen_x++;
            }
            break;
    }

    row = (NANI.screen_y >= NANI.numrows) ? NULL : &NANI.row[NANI.screen_y];
    int rowlen = row ? row->size : 0;
    if (NANI.screen_x > rowlen) {
        NANI.screen_x = rowlen;
    }
}

static void NANI_process_normal_key(char c) {
    if (c & 0x80) return; // ignore releases
    switch (c) {
        case KEY_ARROW_DOWN:
            NANI_move_cursor('j');
            break;
        case KEY_ARROW_UP:
            NANI_move_cursor('k');
            break;
        case KEY_ARROW_LEFT:
            NANI_move_cursor('h');
            break;
        case KEY_ARROW_RIGHT:
            NANI_move_cursor('l');
            break;
        default:
            break;
    }
    char printable_char;
    if (c >= KEY_A && c <= KEY_Z) {
        printable_char = keycode_to_printable_char[NANI.kbd.shift ^ NANI.kbd.caps][(int)c];
    } else {
        printable_char = keycode_to_printable_char[NANI.kbd.shift][(int)c];
    }
    switch (printable_char) {
        case 'b':
        case 'B':
            if (NANI.kbd.ctrl) {
                NANI.screen_y = NANI.rowoff;
                int i = NUM_TERM_ROWS;
                while (i--) {
                    NANI_move_cursor('k');
                }
            }
            break;
        case 'f':
        case 'F':
            if (NANI.kbd.ctrl) {
                NANI.screen_y = NANI.rowoff + NUM_TERM_COLS - 1;
                if (NANI.screen_y >= NANI.numrows)
                    NANI.screen_y = NANI.numrows;
                int i = NUM_TERM_ROWS;
                while (i--) {
                    NANI_move_cursor('j');
                }
            }
            break;
        case '$':
            if (NANI.screen_y < NANI.numrows) {
                NANI.screen_x = NANI.row[NANI.screen_y].size;
            }
            break;
        case 'h':
        case 'j':
        case 'k':
        case 'l':
            NANI_move_cursor(printable_char);
            break;
        case 'i':
            NANI.mode = NANI_INSERT;
            NANI.statusmsg[0] = '\0'; // Clear status message
            break;
        case 'a':
            {
                erow_t *row = (NANI.screen_y >= NANI.numrows) ? NULL : &NANI.row[NANI.screen_y];
                if (row && NANI.screen_x < row->size) {
                    NANI.screen_x++;
                }
            }
            NANI.mode = NANI_INSERT;
            NANI.statusmsg[0] = '\0'; // Clear status message
            break;
        case 'A':
            {
                erow_t *row = (NANI.screen_y >= NANI.numrows) ? NULL : &NANI.row[NANI.screen_y];
                if (row) {
                    NANI.screen_x = row->size;
                } else {
                    NANI.screen_x = 0;
                }
            }
            NANI.mode = NANI_INSERT;
            NANI.statusmsg[0] = '\0'; // Clear status message
            break;
        case ':':
        case '/':
        case '?':
            NANI.mode = NANI_COMMAND;
            NANI.statusmsg[0] = '\0'; // Clear status message
            command_insert_char(printable_char);
            break;
        default:
            break;
    }
}

static void NANI_process_insert_default(char c) {
    if (c & 0x80) return; // ignore releases
    if ((c == KEY_RESERVED || c > KEY_SPACE) && c != KEY_TAB) return; // ignore non-printable characters
    char printable_char;
    if (c >= KEY_A && c <= KEY_Z) {
        printable_char = keycode_to_printable_char[NANI.kbd.shift ^ NANI.kbd.caps][(int)c];
    } else if (c == KEY_TAB) {
        printable_char = '\t';
    } else {
        printable_char = keycode_to_printable_char[NANI.kbd.shift][(int)c];
    }
    if (c == '\0') return;
    if (NANI.screen_y == NANI.numrows) {
        NANI_insert_row(NANI.numrows, "", 0);
    }
    erow_insert_char(&NANI.row[NANI.screen_y], NANI.screen_x, printable_char);
    NANI.screen_x++;
}

static void NANI_process_insert_key(char c) {
    switch (c) {
        case KEY_ESC:
            NANI.mode = NANI_NORMAL;
            break;
        case KEY_BACKSPACE:
            NANI_delete_char();
            break;
        case KEY_ENTER:
            NANI_insert_newline();
            break;
        case KEY_ARROW_DOWN:
            NANI_move_cursor('j');
            break;
        case KEY_ARROW_UP:
            NANI_move_cursor('k');
            break;
        case KEY_ARROW_LEFT:
            NANI_move_cursor('h');
            break;
        case KEY_ARROW_RIGHT:
            NANI_move_cursor('l');
            break;
        default:
            NANI_process_insert_default(c);
    }
}

static void NANI_process_command_default(char c) {
    if (c & 0x80) return; // ignore releases
    if (c == KEY_RESERVED || c > KEY_SPACE) return; // ignore non-printable characters
    char printable_char;
    if (c >= KEY_A && c <= KEY_Z) {
        printable_char = keycode_to_printable_char[NANI.kbd.shift ^ NANI.kbd.caps][(int)c];
    } else {
        printable_char = keycode_to_printable_char[NANI.kbd.shift][(int)c];
    }
    command_insert_char(printable_char);
}

static void NANI_find(char *query) {
    if (query == NULL) return;
    if (NANI.numrows == 0) return;

    int query_len = ece391_strlen((uint8_t *)query);
    static int prev_match_line = -1;
    if (prev_match_line != -1) {
        erow_update_syntax(&NANI.row[prev_match_line]);
    }

    int i;
    int current_y = NANI.screen_y;
    int current_x = erow_sx2rx(&NANI.row[current_y], NANI.screen_x) + NANI.search_direction;
    for (i = 0; i < NANI.numrows + 1; i++) {
        erow_t *row = &NANI.row[current_y];
        char *match = NULL;
        if (NANI.search_direction == 1) {
            for (;current_x < row->rsize - query_len + 1; current_x++) {
                if (ece391_strncmp((uint8_t *)&row->render[current_x], (uint8_t *)query, query_len) == 0) {
                    match = &row->render[current_x];
                    break;
                }
            }
        } else if (NANI.search_direction == -1) {
            for (;current_x >= 0; current_x--) {
                if (current_x + query_len > row->rsize) continue;
                if (ece391_strncmp((uint8_t *)&row->render[current_x], (uint8_t *)query, query_len) == 0) {
                    match = &row->render[current_x];
                    break;
                }
            }
        }
        if (match) {
            NANI.screen_y = current_y;
            NANI.screen_x = erow_rx2sx(row, match - row->render);
            // NANI.rowoff = NANI.numrows;
            prev_match_line = current_y;
            ece391_memset(&row->hl[match - row->render], HL_SEARCH_MATCH, query_len);
            return;
        } else {
            if (NANI.search_direction == 1) {
                current_y = (current_y + NANI.search_direction) % NANI.numrows;
                current_x = 0;
            } else if (NANI.search_direction == -1) {
                current_y = (current_y + NANI.search_direction + NANI.numrows) % NANI.numrows;
                current_x = NANI.row[current_y].rsize - query_len;
            }
        }
    }
}

static void NANI_command_response_colon(char *command, int len) {
    if (command[1] == 'w' && len == 2) {
        if (NANI.filename[0] == '\0') {
            ece391_memcpy(NANI.statusmsg, "E: No filename specified, please force quit", 43);
        } else {
            if (NANI.dirty) NANI_save();
        }
    } else if (command[1] == 'q' && len == 2) {
        if (NANI.dirty) {
            ece391_memcpy(NANI.statusmsg, "E: File has unsaved changes", 27);
        } else {
            NANI_clear_screen();
            ece391_ioctl(1, 0); // Disable raw mode
            ece391_halt(0);
        }
    } else if (command[1] == 'w' && command[2] == 'q' && len == 3) {
        if (NANI.dirty) NANI_save();
        NANI_clear_screen();
        ece391_ioctl(1, 0); // Disable raw mode
        ece391_halt(0);
    } else if (command[1] == 'q' && command[2] == '!' && len == 3) {
        NANI_clear_screen();
        ece391_ioctl(1, 0); // Disable raw mode
        ece391_halt(0);
    } else {
        ece391_memcpy(NANI.statusmsg, "E: Not a valid command", 23);
    }
}

static void NANI_command_response_slash(char *command, int len) {
    if (len == 1) {
        ece391_memcpy(NANI.statusmsg, "E: Not a valid command", 23);
    } else {
        command[len] = '\0';
        NANI.search_direction = 1;
        NANI_find(&command[1]);
    }
}

static void NANI_command_response_question(char *command, int len) {
    if (len == 1) {
        ece391_memcpy(NANI.statusmsg, "E: Not a valid command", 23);
    } else {
        command[len] = '\0';
        NANI.search_direction = -1;
        NANI_find(&command[1]);
    }
}

static void NANI_command_response() {
    char *command = NANI.command.command;
    int len = NANI.command.len;
    if (command[0] == '/') {
        NANI.mode = NANI_SEARCH;
        NANI_command_response_slash(command, len);
    } else if (command[0] == '?') {
        NANI.mode = NANI_SEARCH;
        NANI_command_response_question(command, len);
    } else if (command[0] == ':') {
        NANI_command_response_colon(command, len);
        command_clear();
        NANI.mode = NANI_NORMAL;
    }
}

static void NANI_process_command_key(char c) {
    switch (c) {
        case KEY_ESC:
            command_clear();
            NANI.statusmsg[0] = '\0'; // Clear status message
            NANI.mode = NANI_NORMAL;
            break;
        case KEY_BACKSPACE:
            command_backspace();
            break;
        case KEY_ENTER:
            NANI_command_response();
            break;
        default:
            NANI_process_command_default(c);
    }
}

static void NANI_process_search_key(char c) {
    switch (c) {
        case KEY_ESC:
            command_clear();
            NANI.statusmsg[0] = '\0'; // Clear status message
            NANI.mode = NANI_NORMAL;
            erow_update_syntax(&NANI.row[NANI.screen_y]);
            break;
        case KEY_N:
            NANI_find(&NANI.command.command[1]);
            break;
    }
}


static void NANI_process_key() {
    unsigned char c;

    int nbytes_read = ece391_read(0, &c, 1);
    if (nbytes_read == 0) return;

    switch (c) {
        // Process modifier keys
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            NANI.kbd.shift = 1;
            break;
        case KEY_LEFTCTRL:
            NANI.kbd.ctrl = 1;
            break;
        case KEY_LEFTALT:
            NANI.kbd.alt = 1;
            break;
        case KEY_LEFTSHIFT | 0x80:
        case KEY_RIGHTSHIFT | 0x80:
            NANI.kbd.shift = 0;
            break;
        case KEY_LEFTCTRL | 0x80:
            NANI.kbd.ctrl = 0;
            break;
        case KEY_LEFTALT | 0x80:
            NANI.kbd.alt = 0;
            break;
        case KEY_CAPSLOCK:
            NANI.kbd.caps = !NANI.kbd.caps;
            break;
        // Process other keys
        default:
            switch (NANI.mode) {
                case NANI_NORMAL:
                    NANI_process_normal_key(c);
                    break;
                case NANI_INSERT:
                    NANI_process_insert_key(c);
                    break;
                case NANI_COMMAND:
                    NANI_process_command_key(c);
                    break;
                case NANI_SEARCH:
                    NANI_process_search_key(c);
                    break;
            }
    }
}

static void NANI_init() {
    NANI.screen_x = 0;
    NANI.screen_y = 0;
    NANI.render_x = 0;
    NANI.rowoff = 0;
    NANI.coloff = 0;
    NANI.mode = NANI_NORMAL;
    NANI.kbd.shift = 0;
    NANI.kbd.caps = 0;
    NANI.kbd.ctrl = 0;
    NANI.kbd.alt = 0;
    NANI.numrows = 0;
    NANI.row = (erow_t *)NANI_STATIC_BUF_ADDR;
    NANI.statusmsg[0] = '\0';
    NANI.search_direction = 1;
    NANI.command.len = 0;
    NANI.command.command[0] = '\0';
    NANI.syntax = &HLDB[0];
    abuf_init(&NANI.abuf);
    NANI.dirty = 0;
}

static char line_buf[MAX_COLS + 1];

static int32_t NANI_open(int32_t fd) {
    int linelen;
    NANI_select_syntax();
    while (0 != (linelen = ece391_getline(line_buf, fd, MAX_COLS + 1))) {
        if (NANI.numrows == MAX_ROWS) {
            ece391_fdputs(1, (uint8_t*)"file too long\n");
            return -1;
        }
        if (linelen == -2) {
            ece391_fdputs(1, (uint8_t*)"file too long\n");
            return -1;
        }
        while (linelen > 0 && line_buf[linelen - 1] == '\n')
        linelen--;
        NANI_insert_row(NANI.numrows, line_buf, linelen);
    }
    NANI.dirty = 0;
    return 0;
}

int main()
{
    int32_t fd;
    uint8_t buf[200];
    NANI_init();
    ece391_memset(NANI.filename, 0, 33);
    if (ece391_getargs (buf, 200) != -1) {
        if (-1 == (fd = ece391_open (buf))) {
            ece391_fdputs (1, (uint8_t*)"file not found\n");
            return 2;
        }
        ece391_memset(NANI.filename, 0, 33);
        ece391_memcpy(NANI.filename, buf, ece391_strlen(buf));
        if (-1 == NANI_open(fd)) {
            return 1;
        }
    }
    ece391_ioctl(1, 1); // Enable raw mode
    while (1) {
        NANI_update_screen();
        NANI_process_key();
    }
    ece391_ioctl(1, 0); // Disable raw mode
    return 0;
}


int32_t ece391_memcpy(void* dest, const void* src, int32_t n)
{
    int32_t i;
    char* d = (char*)dest;
    char* s = (char*)src;
    for(i=0; i<n; i++) {
        d[i] = s[i];
    }

    return 0;
}

int32_t ece391_getline(char* buf, int32_t fd, int32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        char c;
        if (1 != ece391_read(fd, &c, 1)) {
            break;
        }
        if (c == '\n') {
            buf[i] = c;
            i++;
            break;
        }
        buf[i] = c;
    }
    if (i == n && buf[i - 1] != '\n')
        i = -2;
    return i;
}

void ece391_memset(void* memory, char c, int n)
{
    char* mem = (char*)memory;
    int i;
    for(i=0; i<n; i++) {
        mem[i] = c;
    }
}
