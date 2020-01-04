#include <ctype.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static struct string_table strings;

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


char* my_strdup(const char *original) {
    if (!original) return NULL;
    size_t length = strlen(original);
    char *newtext = malloc(length + 1);
    if (newtext == NULL) return NULL;
    strcpy(newtext, original);
    newtext[length] = 0;
    return newtext;
}

#define BUFFER_SIZE 1024
int load_strings() {
    char buffer[BUFFER_SIZE] = "";

    memset(&strings, 0, sizeof(struct string_table));

    FILE *fp = fopen("data/strings.dat", "rt");
    if (!fp) return 0;
    while (1) {
        fgets(buffer, BUFFER_SIZE - 1, fp);
        if (feof(fp)) break;

        size_t length = strlen(buffer);
        size_t pos = 0;

        while (pos < length && buffer[pos] != '=') ++pos;
        if (pos >= length) continue;
        buffer[pos] = 0;
        char *actual_text = &buffer[pos + 1];

        length = strlen(actual_text);
        if (length > 0 && isspace(actual_text[length - 1])) {
            actual_text[length - 1] = 0;
            --length;
        }

        char *end_ptr;
        int my_ident = strtol(buffer, &end_ptr, 10);
        if (*end_ptr != 0) continue;

        struct string_entry *entry = malloc(sizeof(struct string_entry));
        entry->id = my_ident;
        entry->text = my_strdup(actual_text);
        int bucket = my_ident % STRING_TABLE_BUCKETS;
        entry->next = strings.entries[bucket];
        strings.entries[bucket] = entry;
    }
    return 1;
}

const char* get_string(int ident) {
    int bucket = ident % STRING_TABLE_BUCKETS;

    struct string_entry *here = strings.entries[bucket];
    while (here != NULL) {
        if (here->id == ident) return here->text;
        here = here->next;
    }

    return "???";
}

void unload_strings() {
    for (int i = 0; i < STRING_TABLE_BUCKETS; ++i) {
        struct string_entry *here = strings.entries[i];
        while (here != NULL) {
            struct string_entry *next = here->next;
            free(here->text);
            free(here);
            here = next;
        }
    }
}


// struct string_entry {
//     int id;
//     char *text;
//     struct string_entry *next;
// };

// struct string_table {
//     struct string_entry entries[STRING_TABLE_BUCKETS];
// };

