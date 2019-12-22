#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"
#include "opcode.h"

static void add_patch(struct parse_data *state, unsigned pos, int width, const char *name);
static int test_width(struct parse_data *state, unsigned value, int width);
static int data_at(struct parse_data *state);
static int data_push(struct parse_data *state, int include_count);
static int data_string(struct parse_data *state);
static int data_string_pad(struct parse_data *state);
static int data_zeroes(struct parse_data *state);
static int data_bytes(struct parse_data *state, int width);


/* ************************************************************************* *
 * OPCODE DEFINITIONS                                                        *
 * ************************************************************************* */
struct mnemonic mnemonics[] = {
    {   op_exit,        "exit",         0 },

    {   op_stkdup,      "stkdup",       0 },
    {   op_stackswap,   "stkswap",      0 },

    {   op_pushw,       "push",         4 },
    {   op_readb,       "readb",        0 },
    {   op_reads,       "reads",        0 },
    {   op_readw,       "readw",        0 },
    {   op_storeb,      "storeb",       0 },
    {   op_stores,      "stores",       0 },
    {   op_storew,      "storew",       0 },

    {   op_add,         "add",          0 },
    {   op_sub,         "sub",          0 },
    {   op_mul,         "mul",          0 },
    {   op_div,         "div",          0 },
    {   op_mod,         "mod",          0 },
    {   op_inc,         "inc",          0 },
    {   op_dec,         "dec",          0 },

    {   op_gets,        "gets",         0 },
    {   op_saynum,      "saynum",       0 },
    {   op_saychar,     "saychar",      0 },
    {   op_saystr,      "saystr",       0 },
    {   op_textbox,     "textbox",      0 },

    {   op_call,        "call",         0 },
    {   op_ret,         "ret",          0 },
    {   op_jump,        "jump",         0 },
    {   op_jumprel,     "jumprel",      0 },
    {   op_jz,          "jz",           0 },
    {   op_jnz,         "jnz",          0 },
    {   op_jlz,         "jlz",          0 },
    {   op_jgz,         "jgz",          0 },
    {   op_jle,         "jle",          0 },
    {   op_jge,         "jge",          0 },

    {   op_mf_fillrand, "mf_fillrand",  0 },
    {   op_mf_fillbox,  "mf_fillbox",   0 },
    {   op_mf_drawbox,  "mf_drawbox",   0 },
    {   op_mf_settile,  "mf_settile",   0 },
    {   op_mf_horzline, "mf_horzline",  0 },
    {   op_mf_vertline, "mf_vertline",  0 },
    {   op_mf_addactor, "mf_addactor",  0 },
    {   op_mf_clear,    "mf_clear",     0 },
    {   op_mf_addevent, "mf_addevent",  0 },
    {   op_mf_makemaze, "mf_makemaze",  0 },
    {   op_mf_makefoes, "mf_makefoes",  0 },

    {   op_p_add,       "p_add",        0 },
    {   op_p_size,      "p_size",       0 },
    {   op_p_name,      "p_name",       0 },
    {   op_p_type,      "p_type",       0 },
    {   op_p_stat,      "p_stat",       0 },
    {   op_p_reset,     "p_reset",      0 },
    {   op_p_damage,    "p_damage",     0 },

    {   op_combat,      "combat",       0 },
    {   op_warpto,      "warpto",       0 },

    {   op_bad,     NULL,       0 }
};


/* ************************************************************************* *
 * GENERAL UTILITY                                                           *
 * ************************************************************************* */
void add_patch(struct parse_data *state, unsigned pos, int width, const char *name) {
    struct backpatch *patch = malloc(sizeof(struct backpatch));
    if (!patch) return;
    patch->address = pos;
    patch->name = str_dup(name);
    patch->next = state->patches;
    patch->width = width;
    state->patches = patch;
}

static int test_width(struct parse_data *state, unsigned value, int width) {
    if (width == 1) {
        uint8_t w8 = value;
        if (value != w8) return 0;
        else return 1;
    }

    if (width == 2) {
        uint16_t w16 = value;
        if (value != w16) return 0;
        else return 1;
    }

    if (width == 4) return 1;

    parse_error(state, "(internal) invalid width.");
    return 0;
}

void write_byte(struct parse_data *state, uint8_t value) {
    fputc(value, state->out);
    ++state->code_pos;
}
void write_short(struct parse_data *state, uint16_t value) {
    fwrite(&value, 2, 1, state->out);
    state->code_pos += 2;
}
void write_long(struct parse_data *state, uint32_t value) {
    fwrite(&value, 4, 1, state->out);
    state->code_pos += 4;
}

void write_push(struct parse_data *state, uint32_t value) {
    write_byte(state, op_pushw);
    write_long(state, value);
}

/* ************************************************************************* *
 * DIRECTIVE PROCESSING                                                      *
 * ************************************************************************* */
int data_at(struct parse_data *state) {
    state->here = state->here->next;

    struct mnemonic *m = mnemonics;
    while (m->name && strcmp(m->name, state->here->text) != 0) {
        ++m;
    }
    if (m->name == NULL) {
        parse_error(state, "unknown mnemonic");
        return 0;
    }
    if (m->operand_size != 0) {
        parse_error(state, "cannot use opcode that takes operands with @");
        return 0;
    }

    if (!data_push(state, 0)) return 0;

    write_byte(state, m->opcode);

    return 1;
}

int data_bytes(struct parse_data *state, int width) {
    state->here = state->here->next;
#ifdef DEBUG
    printf("0x%08X data(%d)", *code_pos, width);
#endif

    while (state->here && state->here->type != tt_eol) {
        int value = 0xFFFFFFFF;
        struct label_def *label;

        switch (state->here->type) {
            case tt_integer:
#ifdef DEBUG
                printf(" %d", state->here->i);
#endif
                value = state->here->i;
                break;
            case tt_identifier:
#ifdef DEBUG
                printf(" LBL:%s", state->here->text);
#endif
                label = get_label(state, state->here->text);
                if (label) {
#ifdef DEBUG
                    printf("/%d", label->pos);
#endif
                    if (!test_width(state, label->pos, width)) {
                        parse_error(state, "Label value does not fit in data width.");
                    }
                    value = label->pos;
                } else {
                    value = -1;
                    add_patch(state, ftell(state->out), width, state->here->text);
                }
                break;
            default:
                parse_error(state, "bad operand type");
                continue;
        }

        fwrite(&value, width, 1, state->out);
        state->code_pos += width;
        state->here = state->here->next;
    }
#ifdef DEBUG
    printf("\n");
#endif
    return 1;
}

int data_define(struct parse_data *state) {
    state->here = state->here->next;
    if (state->here->type != tt_identifier) {
        parse_error(state, "expected identifier");
        return 0;
    }

    const char *name = state->here->text;
    if (get_label(state, name)) {
        parse_error(state, "name already in use");
        return 0;
    }

    state->here = state->here->next;
    if (state->here->type != tt_integer) {
        parse_error(state, "expected integer");
        return 0;
    }

    if (!add_label(state, name, state->here->i)) {
        parse_error(state, "error creating constant");
        return 0;
    }
    skip_line(&state->here);
    return 1;
}

int data_export(struct parse_data *state) {
    uint32_t count = 0;
    long countpos = ftell(state->out);
    write_long(state, 0);

    state->here = state->here->next;
    while (!matches_type(state, tt_eol)) {
        if (require_type(state, tt_identifier)) {
            char buffer[20] = "";

            if (strlen(state->here->text) > 16) {
                strncpy(buffer, state->here->text, 16);
                parse_warn(state, "Label longer than 16 characters; export name truncated.");
            } else {
                strcpy(buffer, state->here->text);
            }
            fwrite(buffer, 16, 1, state->out);
            state->code_pos += 16;
            add_patch(state, ftell(state->out), 4, state->here->text);
            write_long(state, 0);
            ++count;
        }
        state->here = state->here->next;
    }
    fseek(state->out, countpos, SEEK_SET);
    fwrite(&count, 4, 1, state->out);
    fseek(state->out, 0, SEEK_END);
    return 1;
}

int data_push(struct parse_data *state, int include_count) {
    state->here = state->here->next;
#ifdef DEBUG
    printf("0x%08X push");
#endif

    int count = 0;
    while (state->here && state->here->type != tt_eol) {
        ++count;
        int value = 0xFFFFFFFF;
        struct label_def *label;

        switch (state->here->type) {
            case tt_integer:
#ifdef DEBUG
                printf(" %d", state->here->i);
#endif
                value = state->here->i;
                break;
            case tt_identifier:
#ifdef DEBUG
                printf(" LBL:%s", state->here->text);
#endif
                label = get_label(state, state->here->text);
                if (label) {
#ifdef DEBUG
                    printf("/%d", label->pos);
#endif
                    value = label->pos;
                } else {
                    long start = ftell(state->out);
                    fputc(0, state->out);
                    add_patch(state, ftell(state->out), 4, state->here->text);
                    fseek(state->out, start, SEEK_SET);
                }
                break;
            default:
                parse_error(state, "bad operand type");
                continue;
        }

        write_push(state, value);
        state->here = state->here->next;

    }

    if (include_count) {
#ifdef DEBUG
        printf(" CNT:%d", count);
#endif
        write_push(state, count);
    }
#ifdef DEBUG
    printf("\n");
#endif
    return 1;
}

int data_string(struct parse_data *state) {
    state->here = state->here->next;

    if (!require_type(state, tt_string)) {
        return 0;
    }

#ifdef DEBUG
    printf("0x%08X string ~%s~\n", *code_pos, here->text);
#endif
    for (int pos = 0; state->here->text[pos] != 0; ++pos) {
        write_byte(state, state->here->text[pos]);
    }
    write_byte(state, 0);
    skip_line(&state->here);
    return 1;
}

int data_string_pad(struct parse_data *state) {
    state->here = state->here->next;

    if (!require_type(state, tt_integer)) {
        return 0;
    }
    int padTo = state->here->i;
    state->here = state->here->next;

    if (!require_type(state, tt_string)) {
        return 0;
    }

#ifdef DEBUG
    printf("0x%08X string ~%s~\n", *code_pos, here->text);
#endif
    for (int pos = 0; state->here->text[pos] != 0; ++pos) {
        write_byte(state, state->here->text[pos]);
    }
    write_byte(state, 0);

    unsigned long slength = strlen(state->here->text) + 1;
    if (padTo < slength) {
        parse_error(state, "String is longing than padding size.");
    }
    for (unsigned long i = slength; i < padTo; ++i) {
        write_byte(state, 0);
    }

    skip_line(&state->here);
    return 1;
}

int data_zeroes(struct parse_data *state) {
    state->here = state->here->next;
    if (!require_type(state, tt_integer)) {
        return 0;
    }

#ifdef DEBUG
    printf("0x%08X zeroes (%d)\n", *code_pos, here->i);
#endif
    for (int i = 0; i < state->here->i; ++i) {
        write_byte(state, 0);
    }
    skip_line(&state->here);
    return 1;
}



/* ************************************************************************* *
 * CORE PARSING ROUTINE                                                      *
 * ************************************************************************* */
int parse_tokens(struct token_list *list, const char *output_filename) {
    struct parse_data state = { NULL };
    int done_initial = 0;

    state.out = fopen(output_filename, "wb+");
    if (!state.out) {
        fprintf(stderr, "FATAL: could not open output file\n");
        return 1;
    }

    // write empty header
    for (int i = 0; i < HEADER_SIZE; ++i) {
        write_byte(&state, 0);
    }

    state.first_token = state.here = list->first;
    while (state.here) {
        if (state.here->type == tt_eol) {
            state.here = state.here->next;
            continue;
        }

        if (state.here->type == tt_at) {
            data_at(&state);
            continue;
        }

        if (state.here->type != tt_identifier) {
            parse_error(&state, "expected identifier");
            continue;
        }

        if (strcmp(state.here->text, ".export") == 0) {
            if (done_initial) {
                parse_error(&state, ".export must precede other statements");
                continue;
            }
            data_export(&state);
            continue;
        }

        done_initial = 1;

        if (state.here->next && state.here->next->type == tt_colon) {
            if (get_label(&state, state.here->text)) {
                parse_error(&state, "label already defined");
            } else if (!add_label(&state, state.here->text, state.code_pos)) {
                parse_error(&state, "could not create label");
            } else {
                state.here = state.here->next->next;
            }
            continue;
        }

        if (strcmp(state.here->text, ".string") == 0) {
            data_string(&state);
            continue;
        }

        if (strcmp(state.here->text, ".string_pad") == 0) {
            data_string_pad(&state);
            continue;
        }

        if (strcmp(state.here->text, ".byte") == 0) {
            data_bytes(&state, 1);
            continue;
        }
        if (strcmp(state.here->text, ".short") == 0) {
            data_bytes(&state, 2);
            continue;
        }
        if (strcmp(state.here->text, ".word") == 0) {
            data_bytes(&state, 4);
            continue;
        }
        if (strcmp(state.here->text, ".zero") == 0) {
            data_zeroes(&state);
            continue;
        }

        if (strcmp(state.here->text, ".define") == 0) {
            data_define(&state);
            continue;
        }

        if (strcmp(state.here->text, ".push") == 0) {
            data_push(&state, 0);
            continue;
        }

        if (strcmp(state.here->text, ".push_count") == 0) {
            data_push(&state, 1);
            continue;
        }

        if (strcmp(state.here->text, ".include") == 0) {
            state.here = state.here->next;
            if (state.here->type != tt_string) {
                parse_error(&state, "expected string");
                continue;
            }

            struct token_list *new_tokens = lex_file(state.here->text);
            if (new_tokens == NULL) {
                fprintf(stderr, "Failed to lex included file %s.\n", state.here->text);
                return 1 + state.error_count;
            }

            new_tokens->last->next = state.here->next;
            state.here->next = new_tokens->first;
            free(new_tokens);

            skip_line(&state.here);
            continue;
        }


        struct mnemonic *m = mnemonics;
        while (m->name && strcmp(m->name, state.here->text) != 0) {
            ++m;
        }
        if (m->name == NULL) {
            parse_error(&state, "unknown mnemonic");
            continue;
        }

#ifdef DEBUG
        printf("0x%08X %s/%d", code_pos, here->text, m->opcode);
#endif
        write_byte(&state, m->opcode);

        if (state.here->next && state.here->next->type != tt_eol) {
            if (m->operand_size == 0) {
                parse_error(&state, "expected operand");
                continue;
            }

            struct label_def *label;
            struct token *operand = state.here->next;
            int op_value = 0;
            switch (operand->type) {
                case tt_integer:
                    op_value = operand->i;
                    break;
                case tt_identifier:
                    label = get_label(&state, operand->text);
                    if (label) {
                        if (!test_width(&state, label->pos, m->operand_size)) {
                            parse_error(&state, "Label value does not fit in operand width.");
                        }
                        op_value = label->pos;
                    } else {
                        op_value = -1;
                        add_patch(&state, ftell(state.out), m->operand_size, operand->text);
                    }
                    break;
                default:
                    parse_error(&state, "bad operand type");
                    continue;
            }

            switch(m->operand_size) {
                case 1:
                    fputc(op_value, state.out);
                    break;
                case 2: {
                    short v = op_value;
                    fwrite(&v, 2, 1, state.out);
                    break; }
                case 4:
                    fwrite(&op_value, 4, 1, state.out);
                    break;
                default:
                    parse_error(&state, "(assembler) bad operand size");
            }
#ifdef DEBUG
            printf("  op/%d: %d", m->operand_size, op_value);
#endif
            state.code_pos += m->operand_size;
        } else if (m->operand_size > 0) {
            parse_error(&state, "unknown mnemonic");
            continue;
        }
#ifdef DEBUG
        printf("\n");
#endif

        skip_line(&state.here);
    }

    // write header info
    fseek(state.out, 0, SEEK_SET);
    fwrite("TVM", 4, 1, state.out);

    // update backpatches
    struct backpatch *patch = state.patches;
    while (patch) {
        struct label_def *label = get_label(&state, patch->name);
        if (!label) {
            fprintf(stderr, "Undefined symbol %s.\n", patch->name);
            ++state.error_count;
        } else {
            if (!test_width(&state, label->pos, patch->width)) {
                parse_error(&state, "Label value does not fit in patch width.");
            }
            fseek(state.out, patch->address, SEEK_SET);
            uint32_t v = label->pos;
            fwrite(&v, 4, 1, state.out);
        }
        struct backpatch *next = patch->next;
        free(patch->name);
        free(patch);
        patch = next;
    }

    // all done writing file
    fclose(state.out);
    dump_labels(&state);
    free_labels(&state);
    return state.error_count;
}
