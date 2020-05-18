#include <ncurses.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define MAGIC_ID    0x45564F4D
#define MAX_MOVES   300


struct move_info {
    int ident;
    int name_string;
    int accuracy;
    int speed;
    int cost;
    int type;
    int min_range;
    int max_range;
    int damage;
    int size;
    int shape;
    int form;
    int player_use;
    int other_use;
    int damage_icon;
    unsigned flags;
};


struct move_info moves[MAX_MOVES];


int read_data();
void write_data();
int* field_by_index(struct move_info *tile, int index);
void mainloop();
const char* get_form_name(int type);
const char* get_shape_name(int type);
const char* get_type_name(int type);
void edit_tile(int tile_number);

int main() {
    // initialize data
    memset(moves, 0, sizeof(struct move_info) * MAX_MOVES);
    for (int i = 0; i < MAX_MOVES; ++i) {
        moves[i].ident = -1;
    }

    initscr();
    cbreak();
    keypad(stdscr, 1);

    if (!load_strings()) {
        endwin();
        fprintf(stderr, "Failed to load string data.\n");
        return 0;
    }
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
    unload_strings();

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
            if (here < 0 || here >= MAX_MOVES) continue;
            move(i, 0);
            if (here == current)   attron(A_REVERSE);
            else                attroff(A_REVERSE);
            clrtoeol();
            mvprintw(i, 5,  "%.34s", get_string(moves[here].name_string));
            mvprintw(i, 0,  "%4d", moves[here].ident);
            mvprintw(i, 39, " %4d  %3d,%3d,%3d ",
                    moves[here].type,
                    moves[here].accuracy,
                    moves[here].speed,
                    moves[here].cost);
            mvprintw(i, 74, " %d ", here);
        }
        refresh();

        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_ENTER:
            case '\n':
            case '\r':
                if (current >= 0 && current < MAX_MOVES) {
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
                current = MAX_MOVES - 1;
                break;
        }

        if (current < 0) current = 0;
        if (current >= MAX_MOVES) current = MAX_MOVES - 1;
    }
}

int load_data_sort(const void *left_void, const void *right_void) {
    const struct move_info *left = left_void;
    const struct move_info *right = right_void;
    if ( (left->ident < 0 && right->ident >= 0) || (left->ident >= 0 && right->ident < 0) ) {
        return left->ident < right->ident;
    } else {
        return left->ident > right->ident;
    }
}

int read_data() {
    FILE *fp = fopen("data/moves.dat", "rb");
    if (!fp) return 0;
    int id = read32(fp);
    if (id != MAGIC_ID) return 0;

    for (int i = 0; i < MAX_MOVES; ++i) {
        struct move_info *a_move = &moves[i];
        a_move->ident = read32(fp);
        if (a_move->ident < 0) break;
        a_move->name_string = read32(fp);
        a_move->accuracy    = read32(fp);
        a_move->speed       = read32(fp);
        a_move->cost        = read32(fp);
        a_move->type        = read32(fp);
        a_move->min_range   = read32(fp);
        a_move->max_range   = read32(fp);
        a_move->damage      = read32(fp);
        a_move->size        = read32(fp);
        a_move->shape       = read32(fp);
        a_move->form        = read32(fp);
        a_move->player_use  = read32(fp);
        a_move->other_use   = read32(fp);
        a_move->damage_icon = read32(fp);
        a_move->flags       = read32(fp);
    }
    qsort(moves, MAX_MOVES, sizeof(struct move_info), load_data_sort);
    fclose(fp);
    return 1;
}

void write_data() {
    FILE *fp = fopen("data/moves.dat", "wb");
    if (!fp) return;
    write32(fp, MAGIC_ID);

    for (int i = 0; i < MAX_MOVES; ++i) {
        struct move_info *a_move = &moves[i];
        if (a_move->ident >= 0) {
            write32(fp, a_move->ident);
            write32(fp, a_move->name_string);
            write32(fp, a_move->accuracy);
            write32(fp, a_move->speed);
            write32(fp, a_move->cost);
            write32(fp, a_move->type);
            write32(fp, a_move->min_range);
            write32(fp, a_move->max_range);
            write32(fp, a_move->damage);
            write32(fp, a_move->size);
            write32(fp, a_move->shape);
            write32(fp, a_move->form);
            write32(fp, a_move->player_use);
            write32(fp, a_move->other_use);
            write32(fp, a_move->damage_icon);
            write32(fp, a_move->flags);
        }
    }
    write32(fp, -1);
    fclose(fp);
}

int* field_by_index(struct move_info *a_move, int index) {
    switch(index) {
        case 0:     return &a_move->ident;
        case 1:     return &a_move->name_string;
        case 2:     return &a_move->accuracy;
        case 3:     return &a_move->speed;
        case 4:     return &a_move->cost;
        case 5:     return &a_move->type;
        case 6:     return &a_move->min_range;
        case 7:     return &a_move->max_range;
        case 8:     return &a_move->damage;
        case 9:     return &a_move->size;
        case 10:    return &a_move->shape;
        case 11:    return &a_move->form;
        case 12:    return &a_move->player_use;
        case 13:    return &a_move->other_use;
        case 14:    return &a_move->damage_icon;
        case 15:    return (int*)&a_move->flags;
        default:    return NULL;
    }
}

const char* get_form_name(int type) {
    switch(type) {
        case 0: return "self";
        case 1: return "bullet";
        case 2: return "melee";
        case 3: return "lobbed";
        case 4: return "fourway";
    }
    return "unknown";
}

const char* get_shape_name(int type) {
    switch(type) {
        case 0: return "square";
        case 1: return "circle";
        case 2: return "long";
        case 3: return "wide";
        case 4: return "cone";
    }
    return "unknown";
}

const char* get_type_name(int type) {
    switch(type) {
        case 0: return "physical";
        case 1: return "cold";
        case 2: return "fire";
        case 3: return "electric";
        case 4: return "toxic";
        case 5: return "divine";
        case 6: return "infernal";
        case 7: return "void";
    }
    return "unknown";
}

#define MAX_TILE_FIELD      15
void edit_tile(int tile_number) {
    int field = 0;
    struct move_info *a_move = &moves[tile_number];

    while (1) {
        attrset(A_NORMAL);
        clear();
        mvprintw(0, 75, "%d", tile_number);
        mvprintw( 0, 0, "      IDENT: %d%c",        a_move->ident,         field == 0  ? '_' : ' ');
        mvprintw( 1, 0, "       NAME: %d%c  %.40s", a_move->name_string,   field == 1  ? '_' : ' ', get_string(a_move->name_string));
        mvprintw( 2, 0, "   ACCURACY: %d%c",        a_move->accuracy,      field == 2  ? '_' : ' ');
        mvprintw( 3, 0, "      SPEED: %d%c",        a_move->speed,         field == 3  ? '_' : ' ');
        mvprintw( 4, 0, "       COST: %d%c",        a_move->cost,          field == 4  ? '_' : ' ');
        mvprintw( 5, 0, "       TYPE: %d%c  %s",    a_move->type,          field == 5  ? '_' : ' ', get_type_name(a_move->type));
        mvprintw( 6, 0, "  MIN RANGE: %d%c",        a_move->min_range,     field == 6  ? '_' : ' ');
        mvprintw( 7, 0, "  MAX RANGE: %d%c",        a_move->max_range,     field == 7  ? '_' : ' ');
        mvprintw( 8, 0, "     DAMAGE: %d%c",        a_move->damage,        field == 8  ? '_' : ' ');
        mvprintw( 9, 0, "       SIZE: %d%c",        a_move->size,          field == 9  ? '_' : ' ');
        mvprintw(10, 0, "      SHAPE: %d%c %s",     a_move->shape,         field == 10 ? '_' : ' ', get_shape_name(a_move->shape));
        mvprintw(11, 0, "       FORM: %d%c %s",     a_move->form,          field == 11 ? '_' : ' ', get_form_name(a_move->form));
        mvprintw(12, 0, " PLAYER USE: %d%c %.40s",  a_move->player_use,    field == 12 ? '_' : ' ', get_string(a_move->player_use));
        mvprintw(13, 0, "  OTHER USE: %d%c %.40s",  a_move->other_use,     field == 13 ? '_' : ' ', get_string(a_move->other_use));
        mvprintw(14, 0, "DAMAGE ICON: %d%c",        a_move->damage_icon,   field == 14 ? '_' : ' ');
        mvprintw(15, 0, "      FLAGS: 0x%X%c",      a_move->flags,         field == 15 ? '_' : ' ');
        mvprintw(field, 0, "->");

        refresh();

        int *t = field_by_index(a_move, field);
        int key = getch();
        switch(key) {
            case 'Q':
                return;
            case KEY_ENTER:
            case '\n':
            case '\r':
                // if (field == 1) {
                //     edit_text(1, 13, a_move->name);
                // } else if (field == 8) {
                //     a_move->flags ^= TF_SOLID;
                // } else if (field == 9) {
                //     a_move->flags ^= TF_OPAQUE;
                // } else if (field == 10) {
                //     a_move->flags ^= TF_ISDOOR;
                // }
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
            case KEY_BACKSPACE:
            case '\x8':
                if (t && *t != 0) {
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
                    a_move = &moves[tile_number];
                    field = 0;
                }
                break;
            case KEY_NPAGE:
                if (tile_number < MAX_MOVES - 1) {
                    ++tile_number;
                    a_move = &moves[tile_number];
                    field = 0;
                }
                break;
            // default:
            //     if (field == 1 && is_name_char(key)) {
            //         edit_text_key(a_move->name, key);
            //     }
        }
    }

}
