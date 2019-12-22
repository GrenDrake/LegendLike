#include <ncurses.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define MAGIC_ID    0x45505954
#define TYPE_COUNT  16
#define NAME_LENGTH 16

char type_names[TYPE_COUNT][NAME_LENGTH];
int types[TYPE_COUNT][TYPE_COUNT];

int read_data();
void write_data();
void mainloop();
void edit_names();
void edit_types();

int main() {
    // initialize data
    memset(type_names, 0, TYPE_COUNT * NAME_LENGTH);
    memset(types, 0, TYPE_COUNT * TYPE_COUNT);

    initscr();
    cbreak();
    keypad(stdscr, 1);

    if (!read_data()) {
        printw("Failed to read data. Enter 'Y' to continue with empty dataset.");
        int key = getch();
        if (key != 'Y') {
            endwin();
            return 0;
        }
    }

    mainloop();
    write_data();

    endwin();
    return 0;
}


void mainloop() {
    while (1) {
        clear();
        printw("N - Edit Type Names\nT - Edit Type Effectivenss\n\nQ - Save and Quit");

        int key = getch();
        switch(key) {
            case 'Q':   return;
            case 'N':
            case 'n':   edit_names();   break;
            case 'T':
            case 't':   edit_types();   break;
        }
    }
}

void edit_names() {
    int cur_name = 0;
    while (1) {
        clear();
        for (int i = 0; i < TYPE_COUNT; ++i) {
            if (cur_name == i)  attron(A_REVERSE);
            else                attroff(A_REVERSE);
            mvprintw(i, 0, "%3d : %s", i, type_names[i]);
        }
        attrset(A_NORMAL);
        mvprintw(20, 0, "Q - Save and return");
        refresh();

        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_UP:
                if (cur_name > 0) {
                    --cur_name;
                }
                break;
            case KEY_DOWN:
                if (cur_name < TYPE_COUNT - 1) {
                    ++cur_name;
                }
                break;
            case KEY_ENTER:
            case '\n':
            case '\r':
                edit_text(cur_name, 6, type_names[cur_name]);
                break;
            case KEY_BACKSPACE:
            case '\x8':
                edit_text_key(type_names[cur_name], key);
                break;
            default:
                if (is_name_char(key)) {
                    edit_text_key(type_names[cur_name], key);
                }
        }
        if (key == 'Q') return;
    }
}

int read_data() {
    FILE *fp = fopen("data/types.dat", "rb");
    if (!fp) return 0;
    int id = read32(fp);
    if (id != MAGIC_ID) {
        fclose(fp);
        return 0;
    }
    int count = read8(fp);
    if (count != TYPE_COUNT) {
        fclose(fp);
        return 0;
    }
    for (int i = 0; i < TYPE_COUNT; ++i) {
        fread(type_names[i], NAME_LENGTH, 1, fp);
    }
    for (int x = 0; x < TYPE_COUNT; ++x) {
        for (int y = 0; y < TYPE_COUNT; ++y) {
            types[x][y] = read8(fp);
        }
    }
    fclose(fp);
    return 1;
}

void write_data() {
    FILE *fp = fopen("data/types.dat", "wb");
    if (!fp) return;
    write32(fp, MAGIC_ID); // file type magic ID
    write8(fp, TYPE_COUNT);
    for (int i = 0; i < TYPE_COUNT; ++i) {
        fwrite(type_names[i], NAME_LENGTH, 1, fp);
    }
    for (int x = 0; x < TYPE_COUNT; ++x) {
        for (int y = 0; y < TYPE_COUNT; ++y) {
            write8(fp, types[x][y]);
        }
    }
    fclose(fp);
}

void edit_types() {
    int cy = 0, cx = 0;
    while (1) {
        clear();
        for (int x = 0; x < TYPE_COUNT; ++x) {
            for (int c = 0; c < 3; ++c) {
                int ch = type_names[x][c];
                if (ch >= 32 && ch < 127) {
                    mvaddch(0, NAME_LENGTH + 1 + c + x * 4, ch);
                }
            }
        }
        for (int y = 0; y < TYPE_COUNT; ++y) {
            int length = strlen(type_names[y]);
            attrset(A_NORMAL);
            mvprintw(y + 1, NAME_LENGTH - length, "%s", type_names[y]);
            for (int x = 0; x < TYPE_COUNT; ++x) {
                if (x == cx && y == cy) attron(A_REVERSE);
                else                    attroff(A_REVERSE);
                mvprintw(y + 1, NAME_LENGTH + 1 + x * 4, "%d", types[x][y]);
            }
        }
        attrset(A_NORMAL);
        refresh();

        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_LEFT:
                if (cx > 0)              --cx;
                else                     cx = TYPE_COUNT - 1;
                break;
            case KEY_RIGHT:
                if (cx < TYPE_COUNT - 1) ++cx;
                else                     cx = 0;
                break;
            case KEY_UP:
                if (cy > 0)              --cy;
                else                     cy = TYPE_COUNT - 1;
                break;
            case KEY_DOWN:
                if (cy < TYPE_COUNT - 1) ++cy;
                else                     cy = 0;
                break;
            case KEY_DC:
            case '\x7f':
                types[cx][cy] = 100;
                break;
            case KEY_BACKSPACE:
            case '\x8':
                types[cx][cy] /= 10;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                types[cx][cy] *= 10;
                types[cx][cy] += key - '0';
                break; }
        }
    }
}
