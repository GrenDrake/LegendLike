/* ************************************************************************* *
 * OPCODE DEFINITIONS                                                        *
 * ************************************************************************* */

#include <string>
#include <vector>
#include "assemble.h"

Mnemonic BAD_OPCODE{ Opcode::bad, "", 0 };

std::vector<Mnemonic> mnemonics{
    {   Opcode::exit,        "exit",         0 },

    {   Opcode::stkdup,      "stkdup",       0 },
    {   Opcode::stackswap,   "stkswap",      0 },

    {   Opcode::pushw,       "push",         4 },
    {   Opcode::readb,       "readb",        0 },
    {   Opcode::reads,       "reads",        0 },
    {   Opcode::readw,       "readw",        0 },
    {   Opcode::storeb,      "storeb",       0 },
    {   Opcode::stores,      "stores",       0 },
    {   Opcode::storew,      "storew",       0 },

    {   Opcode::add,         "add",          0 },
    {   Opcode::sub,         "sub",          0 },
    {   Opcode::mul,         "mul",          0 },
    {   Opcode::div,         "div",          0 },
    {   Opcode::mod,         "mod",          0 },
    {   Opcode::inc,         "inc",          0 },
    {   Opcode::dec,         "dec",          0 },

    {   Opcode::gets,        "gets",         0 },
    {   Opcode::saynum,      "saynum",       0 },
    {   Opcode::saychar,     "saychar",      0 },
    {   Opcode::saystr,      "saystr",       0 },
    {   Opcode::textbox,     "textbox",      0 },

    {   Opcode::call,        "call",         0 },
    {   Opcode::ret,         "ret",          0 },
    {   Opcode::jump,        "jump",         0 },
    {   Opcode::jumprel,     "jumprel",      0 },
    {   Opcode::jz,          "jz",           0 },
    {   Opcode::jnz,         "jnz",          0 },
    {   Opcode::jlz,         "jlz",          0 },
    {   Opcode::jgz,         "jgz",          0 },
    {   Opcode::jle,         "jle",          0 },
    {   Opcode::jge,         "jge",          0 },

    {   Opcode::mf_fillrand, "mf_fillrand",  0 },
    {   Opcode::mf_fillbox,  "mf_fillbox",   0 },
    {   Opcode::mf_drawbox,  "mf_drawbox",   0 },
    {   Opcode::mf_settile,  "mf_settile",   0 },
    {   Opcode::mf_horzline, "mf_horzline",  0 },
    {   Opcode::mf_vertline, "mf_vertline",  0 },
    {   Opcode::mf_addactor, "mf_addactor",  0 },
    {   Opcode::mf_clear,    "mf_clear",     0 },
    {   Opcode::mf_addevent, "mf_addevent",  0 },
    {   Opcode::mf_makemaze, "mf_makemaze",  0 },
    {   Opcode::mf_makefoes, "mf_makefoes",  0 },
    {   Opcode::mf_fromfile,    "mf_fromfile",     0 },

    {   Opcode::p_stat,      "p_stat",       0 },
    {   Opcode::p_reset,     "p_reset",      0 },
    {   Opcode::p_damage,    "p_damage",     0 },
    {   Opcode::p_giveitem,  "p_giveitem",     0 },
    {   Opcode::p_learn,     "p_learn",     0 },
    {   Opcode::p_forget,    "p_forget",     0 },

    {   Opcode::warpto,      "warpto",       0 },
};

const Mnemonic& getMnemonic(const std::string &name) {
    for (const Mnemonic &m : mnemonics) {
        if (m.name == name) return m;
    }
    return BAD_OPCODE;
}
