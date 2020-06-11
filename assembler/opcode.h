#ifndef OPCODE_H
#define OPCODE_H

enum opcode {
    op_exit,

    op_stkdup,
    op_stackswap,

    op_pushw,
    op_readb,
    op_reads,
    op_readw,
    op_storeb,
    op_stores,
    op_storew,

    op_add,
    op_sub,
    op_mul,
    op_div,
    op_mod,
    op_inc,
    op_dec,

    op_gets,
    op_saynum,
    op_saychar,
    op_saystr,
    op_textbox,

    op_call,
    op_ret,

    op_jump,
    op_jumprel,
    op_jz,
    op_jnz,
    op_jlz,
    op_jgz,
    op_jle,
    op_jge,

    op_mf_fillrand,
    op_mf_fillbox,
    op_mf_drawbox,
    op_mf_settile,
    op_mf_horzline,
    op_mf_vertline,
    op_mf_addactor,
    op_mf_clear,
    op_mf_addevent,
    op_mf_fromfile,
    op_mf_makemaze,
    op_mf_makefoes,

    op_p_stat,
    op_p_reset,
    op_p_damage,
    op_p_learn,
    op_p_forget,
    op_p_knows,
    op_p_giveitem,

    op_warpto,

    op_bad = -1
};

#endif
