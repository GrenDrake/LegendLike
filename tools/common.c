#include <ctype.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

void edit_text_key(char *text, int key) {
    unsigned long pos = strlen(text);
    if (pos < 31 && is_name_char(key)) {
        text[pos] = key;
        ++pos;
        text[pos] = 0;
    }
    if (key == KEY_BACKSPACE || key == '\x8') {
        if (pos > 0) {
            --pos;
            text[pos] = 0;
        }
    }
}

void edit_text(int y, int x, char *text) {
    while (1) {
        move(y, x);
        attron(A_REVERSE);
        clrtoeol();
        mvprintw(y, x, "%s", text);
        int key = getch();
        if (key == KEY_ENTER || key == '\n' || key == '\r') {
            return;
        }
        edit_text_key(text, key);
    }
}

void force_bool(int *value) {
    if (*value != 0) *value = 1;
}

int is_name_char(int c) {
    if (c < 0 || c > 127) return 0;
    return isalnum(c) || ispunct(c) || c == ' ';
}

uint32_t read32(FILE *fp) {
    uint32_t value;
    fread(&value, 4, 1, fp);
    return value;
}

uint16_t read16(FILE *fp) {
    uint16_t value;
    fread(&value, 2, 1, fp);
    return value;
}

uint8_t  read8(FILE  *fp) {
    uint8_t value;
    fread(&value, 1, 1, fp);
    return value;
}

int sign(int x) {
    return (x > 0) - (x < 0);
}

void write32(FILE *fp, uint32_t value) {
    fwrite(&value, 4, 1, fp);
}

void write16(FILE *fp, uint16_t value) {
    fwrite(&value, 2, 1, fp);
}

void write8(FILE  *fp, uint8_t  value) {
    fwrite(&value, 1, 1, fp);
}
