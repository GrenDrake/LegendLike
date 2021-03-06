#ifndef OPCODE_H
#define OPCODE_H

#include <iosfwd>

enum class Opcode {
    exit,

    stkdup,
    stkswap,

    pushw,
    readb,
    reads,
    readw,
    storeb,
    stores,
    storew,

    add,
    sub,
    mul,
    div,
    mod,
    inc,
    dec,

    gets,
    saynum,
    saychar,
    saystr,
    textbox,

    call,
    ret,

    jump,
    jumprel,
    jz,
    jnz,
    jlz,
    jgz,
    jle,
    jge,

    mf_fillrand,
    mf_fillbox,
    mf_drawbox,
    mf_settile,
    mf_horzline,
    mf_vertline,
    mf_addactor,
    mf_addactors,
    mf_additem,
    mf_clear,
    mf_addevent,
    mf_fromfile,
    mf_makemaze,
    mf_makefoes,

    p_stat,
    p_reset,
    p_damage,
    p_giveitem,
    p_claimed,
    p_giveitem_imm,
    p_hasitem,

    warpto,

    bad = -1
};

std::ostream& operator<<(std::ostream &out, Opcode code);

#endif
