#include <ncurses.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define MAGIC_ID    0x454C4954
#define MAX_TILES   100
#define NAME_LENGTH 32

const int TF_SOLID          = 0x01;
const int TF_OPAQUE         = 0x02;
const int TF_ISDOOR         = 0x04;

const char *flagNames[] = {
    "Solid",
    "Opaque",
    "Is Door",
    NULL
};

struct tile_info {
    int ident;
    char name[NAME_LENGTH];
    int art_index;
    int red, green, blue;
    int interact_to;
    int animLength;
    unsigned flags;
};


struct tile_info tiles[MAX_TILES];


int read_data();
void write_data();
int* field_by_index(struct tile_info *tile, int index);
void mainloop();
void edit_tile(int tile_number);

int main() {
    // initialize data
    memset(tiles, 0, sizeof(struct tile_info) * MAX_TILES);
    for (int i = 0; i < MAX_TILES; ++i) {
        tiles[i].ident = -1;
        tiles[i].interact_to = -1;
    }

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
    int height = getmaxy(stdscr);
    int current = 0;

    while (1) {
        int scroll = current - (height / 2);
        clear();
        for (int i = 0; i < height; ++i) {
            int here = i + scroll;
            if (here < 0 || here >= MAX_TILES) continue;
            move(i, 0);
            if (here == current)   attron(A_REVERSE);
            else                attroff(A_REVERSE);
            clrtoeol();
            mvprintw(i, 0,  "%4d %s", tiles[here].ident, tiles[here].name);
            mvprintw(i, 40, "%4d  %3d,%3d,%3d  %c%c%c",
                    tiles[here].art_index,
                    tiles[here].red, tiles[here].green, tiles[here].blue,
                    tiles[here].flags & TF_SOLID  ? 'S' : ' ',
                    tiles[here].flags & TF_OPAQUE ? 'O' : ' ',
                    tiles[here].flags & TF_ISDOOR ? 'D' : ' ');
            if (tiles[here].interact_to >= 0) {
                mvprintw(i, 70, "%3d", tiles[here].interact_to);
            }
            mvprintw(i, 75, "%d", here);
        }
        refresh();

        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_ENTER:
            case '\n':
            case '\r':
                if (current >= 0 && current < MAX_TILES) {
                    edit_tile(current);
                }
                break;
            case KEY_UP:
                --current;
                break;
            case KEY_DOWN:
                ++current;
                break;
            case KEY_PPAGE:
                current -= height / 2;
                break;
            case KEY_NPAGE:
                current += height / 2;
                break;
            case KEY_HOME:
                current = 0;
                break;
            case KEY_END:
                current = MAX_TILES - 1;
                break;
        }

        if (current < 0) current = 0;
        if (current >= MAX_TILES) current = MAX_TILES - 1;
    }
}

int load_data_sort(const void *left_void, const void *right_void) {
    const struct tile_info *left = left_void;
    const struct tile_info *right = right_void;
    if ( (left->ident < 0 && right->ident >= 0) || (left->ident >= 0 && right->ident < 0) ) {
        return left->ident < right->ident;
    } else {
        return left->ident > right->ident;
    }
}

int read_data() {
    FILE *fp = fopen("data/tiles.dat", "rb");
    if (!fp) return 0;
    int id = read32(fp);
    if (id != MAGIC_ID) return 0;

    for (int i = 0; i < MAX_TILES; ++i) {
        struct tile_info *tile = &tiles[i];
        tile->ident = read32(fp);
        if (tile->ident < 0) break;
        tile->art_index = read32(fp);
        tile->interact_to = read32(fp);
        tile->red = read8(fp);
        tile->green = read8(fp);
        tile->blue = read8(fp);
        tile->animLength = read8(fp);
        tile->flags = read32(fp);
        fread(tile->name, NAME_LENGTH, 1, fp);
    }
    qsort(tiles, MAX_TILES, sizeof(struct tile_info), load_data_sort);
    fclose(fp);
    return 1;
}

void write_data() {
    FILE *fp = fopen("data/tiles.dat", "wb");
    if (!fp) return;
    write32(fp, MAGIC_ID);

    for (int i = 0; i < MAX_TILES; ++i) {
        struct tile_info *tile = &tiles[i];
        if (tile->ident >= 0) {
            write32(fp, tile->ident);
            write32(fp, tile->art_index);
            write32(fp, tile->interact_to);
            write8(fp, tile->red);
            write8(fp, tile->green);
            write8(fp, tile->blue);
            write8(fp, tile->animLength);
            write32(fp, tile->flags);
            fwrite(tile->name, NAME_LENGTH, 1, fp);
        }
    }
    write32(fp, -1);
    fclose(fp);
}

int* field_by_index(struct tile_info *tile, int index) {
    switch(index) {
        case 0:     return &tile->ident;
        case 2:     return &tile->art_index;
        case 3:     return &tile->red;
        case 4:     return &tile->green;
        case 5:     return &tile->blue;
        case 6:     return &tile->interact_to;
        case 7:     return &tile->animLength;
        case 8:     return (int*)&tile->flags;
        default:    return NULL;
    }
}

#define MAX_TILE_FIELD      11
void edit_tile(int tile_number) {
    int field = 0;
    struct tile_info *tile = &tiles[tile_number];

    while (1) {
        attrset(A_NORMAL);
        clear();
        mvprintw(0, 75, "%d", tile_number);
        mvprintw(0, 0, "      IDENT: %d%c", tile->ident,       field == 0 ? '_' : ' ');
        mvprintw(1, 0, "       NAME: %s%c", tile->name,        field == 1 ? '_' : ' ');
        mvprintw(2, 0, "        ART: %d%c", tile->art_index,   field == 2 ? '_' : ' ');
        mvprintw(3, 0, "        RED: %d%c", tile->red,         field == 3 ? '_' : ' ');
        mvprintw(4, 0, "      GREEN: %d%c", tile->green,       field == 4 ? '_' : ' ');
        mvprintw(5, 0, "       BLUE: %d%c", tile->blue,        field == 5 ? '_' : ' ');
        mvprintw(6, 0, "   INTERACT: %d%c", tile->interact_to, field == 6 ? '_' : ' ');
        mvprintw(7, 0, "ANIM LENGTH: %d%c", tile->animLength,  field == 7 ? '_' : ' ');
        mvprintw(8, 0, "      FLAGS: 0x%X%c", tile->flags,     field == 8 ? '_' : ' ');
        mvprintw(9, 0, "      SOLID: %s", (tile->flags & TF_SOLID) ? "yes" : "no");
        mvprintw(10, 0, "     OPAQUE: %s", (tile->flags & TF_OPAQUE) ? "yes" : "no");
        mvprintw(11,0, "      ISDOOR: %s", (tile->flags & TF_ISDOOR) ? "yes" : "no");
        mvprintw(field, 0, "->");

        refresh();

        int *t = field_by_index(tile, field);
        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_ENTER:
            case '\n':
            case '\r':
                if (field == 1) {
                    edit_text(1, 13, tile->name);
                } else if (field == 9) {
                    tile->flags ^= TF_SOLID;
                } else if (field == 10) {
                    tile->flags ^= TF_OPAQUE;
                } else if (field == 11) {
                    tile->flags ^= TF_ISDOOR;
                }
                break;
            case KEY_LEFT:
                if (t) --(*t);
                break;
            case KEY_RIGHT:
                if (t) ++(*t);
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
            case '9':
                if (t) {
                    *t *= 10;
                    if (*t < 0) *t -= key - '0';
                    else        *t += key - '0';
                }
                break;
            case '-':
                if (t) *t = -(*t);
                break;
            case '/':
            case KEY_BACKSPACE:
            case '\x8':
                if (field == 1) {
                    edit_text_key(tile->name, key);
                } else if (t && *t != 0) {
                    *t /= 10;
                }
                break;
            case KEY_UP:
                if (field > 0) --field;
                break;
            case '\t':
            case KEY_DOWN:
                if (field < MAX_TILE_FIELD) ++field;
                break;
            case KEY_HOME:
                field = 0;
                break;
            case KEY_END:
                field = MAX_TILE_FIELD;
                break;
            case KEY_PPAGE:
                if (tile_number > 0) {
                    --tile_number;
                    tile = &tiles[tile_number];
                    field = 0;
                }
                break;
            case KEY_NPAGE:
                if (tile_number < MAX_TILES - 1) {
                    ++tile_number;
                    tile = &tiles[tile_number];
                    field = 0;
                }
                break;
            default:
                if (field == 1 && is_name_char(key)) {
                    edit_text_key(tile->name, key);
                }
        }
    }

}

