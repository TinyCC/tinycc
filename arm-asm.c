/*
 *  ARM specific functions for TCC assembler
 *
 *  Copyright (c) 2001, 2002 Fabrice Bellard
 *  Copyright (c) 2020 Danny Milosavljevic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef TARGET_DEFS_ONLY

#define CONFIG_TCC_ASM
#define NB_ASM_REGS 16

ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);

/*************************************************************/
#else
/*************************************************************/

#define USING_GLOBALS
#include "tcc.h"

enum {
    OPT_REG32,
    OPT_REGSET32,
    OPT_IM8,
    OPT_IM8N,
    OPT_IM32,
};
#define OP_REG32  (1 << OPT_REG32)
#define OP_REG    (OP_REG32)
#define OP_IM32   (1 << OPT_IM32)
#define OP_IM8   (1 << OPT_IM8)
#define OP_IM8N   (1 << OPT_IM8N)
#define OP_REGSET32  (1 << OPT_REGSET32)

typedef struct Operand {
    uint32_t type;
    union {
        uint8_t reg;
        uint16_t regset;
        ExprValue e;
    };
} Operand;

/* Parse a text containing operand and store the result in OP */
static void parse_operand(TCCState *s1, Operand *op)
{
    ExprValue e;
    int8_t reg;
    uint16_t regset = 0;

    op->type = 0;

    if (tok == '{') { // regset literal
        next(); // skip '{'
        while (tok != '}' && tok != TOK_EOF) {
            reg = asm_parse_regvar(tok);
            if (reg == -1) {
                expect("register");
                return;
            } else
                next(); // skip register name

            regset |= 1 << reg;
            if (tok != ',')
                break;
            next(); // skip ','
        }
        if (tok != '}')
            expect("'}'");
        next(); // skip '}'
        if (regset == 0) {
            // ARM instructions don't support empty regset.
            tcc_error("empty register list is not supported");
        } else {
            op->type = OP_REGSET32;
            op->regset = regset;
        }
    } else if (tok == '#' || tok == '$') {
        /* constant value */
        next(); // skip '#' or '$'
        asm_expr(s1, &e);
        op->type = OP_IM32;
        op->e = e;
        if (!op->e.sym) {
            if ((int) op->e.v < 0 && (int) op->e.v >= -255)
                op->type = OP_IM8N;
            else if (op->e.v == (uint8_t)op->e.v)
                op->type = OP_IM8;
        } else
            expect("constant");
    } else if ((reg = asm_parse_regvar(tok)) != -1) {
        next(); // skip register name
        op->type = OP_REG32;
        op->reg = (uint8_t) reg;
    } else
        expect("operand");
}

/* XXX: make it faster ? */
ST_FUNC void g(int c)
{
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind] = c;
    ind = ind1;
}

ST_FUNC void gen_le16 (int i)
{
    g(i);
    g(i>>8);
}

ST_FUNC void gen_le32 (int i)
{
    gen_le16(i);
    gen_le16(i>>16);
}

ST_FUNC void gen_expr32(ExprValue *pe)
{
    gen_le32(pe->v);
}

static uint32_t condition_code_of_token(int token) {
    if (token < TOK_ASM_nopeq) {
        expect("instruction");
        return 0;
    } else
        return (token - TOK_ASM_nopeq) & 15;
}

static void asm_emit_opcode(int token, uint32_t opcode) {
    gen_le32((condition_code_of_token(token) << 28) | opcode);
}

static void asm_nullary_opcode(int token)
{
    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_nopeq:
        asm_emit_opcode(token, 0xd << 21); // mov r0, r0
        break;
    case TOK_ASM_wfeeq:
        asm_emit_opcode(token, 0x320f002);
    case TOK_ASM_wfieq:
        asm_emit_opcode(token, 0x320f003);
        break;
    default:
        expect("nullary instruction");
    }
}

static void asm_block_data_transfer_opcode(TCCState *s1, int token)
{
    uint32_t opcode;
    int op0_exclam;
    Operand ops[2];
    int nb_ops = 1;
    parse_operand(s1, &ops[0]);
    if (tok == '!') {
        op0_exclam = 1;
        next(); // skip '!'
    }
    if (tok == ',') {
        next(); // skip comma
        parse_operand(s1, &ops[1]);
        ++nb_ops;
    }
    if (nb_ops < 1) {
        expect("at least one operand");
        return;
    } else if (ops[nb_ops - 1].type != OP_REGSET32) {
        expect("(last operand) register list");
        return;
    }

    // block data transfer: 1 0 0 P U S W L << 20 (general case):
    // operands:
    //   Rn: bits 19...16 base register
    //   Register List: bits 15...0

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_pusheq: // TODO: Optimize 1-register case to: str ?, [sp, #-4]!
        // Instruction: 1 I=0 P=1 U=0 S=0 W=1 L=0 << 20, op 1101
        //   operands:
        //      Rn: base register
        //      Register List: bits 15...0
        if (nb_ops != 1)
            expect("exactly one operand");
        else
            asm_emit_opcode(token, (0x92d << 16) | ops[0].regset); // TODO: base register ?
        break;
    case TOK_ASM_popeq: // TODO: Optimize 1-register case to: ldr ?, [sp], #4
        // Instruction: 1 I=0 P=0 U=1 S=0 W=0 L=1 << 20, op 1101
        //   operands:
        //      Rn: base register
        //      Register List: bits 15...0
        if (nb_ops != 1)
            expect("exactly one operand");
        else
            asm_emit_opcode(token, (0x8bd << 16) | ops[0].regset); // TODO: base register ?
        break;
    default:
        expect("block data transfer instruction");
    }
}

ST_FUNC void asm_opcode(TCCState *s1, int token)
{
    while (token == TOK_LINEFEED) {
        next();
        token = tok;
    }
    if (token == TOK_EOF)
        return;
    if (token < TOK_ASM_nopeq) {
        expect("instruction");
        return;
    }

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_pusheq:
    case TOK_ASM_popeq:
        return asm_block_data_transfer_opcode(s1, token);
    case TOK_ASM_nopeq:
    case TOK_ASM_wfeeq:
    case TOK_ASM_wfieq:
        return asm_nullary_opcode(token);
    default:
        expect("known instruction");
    }
}

ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier)
{
    tcc_error("internal error: subst_asm_operand not implemented");
}

/* generate prolog and epilog code for asm statement */
ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands,
                         int nb_outputs, int is_output,
                         uint8_t *clobber_regs,
                         int out_reg)
{
}

ST_FUNC void asm_compute_constraints(ASMOperand *operands,
                                    int nb_operands, int nb_outputs,
                                    const uint8_t *clobber_regs,
                                    int *pout_reg)
{
}

ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str)
{
    int reg;
    TokenSym *ts;

    if (!strcmp(str, "memory") ||
        !strcmp(str, "cc") ||
        !strcmp(str, "flags"))
        return;
    ts = tok_alloc(str, strlen(str));
    reg = asm_parse_regvar(ts->tok);
    if (reg == -1) {
        tcc_error("invalid clobber register '%s'", str);
    }
    clobber_regs[reg] = 1;
}

/* If T refers to a register then return the register number and type.
   Otherwise return -1.  */
ST_FUNC int asm_parse_regvar (int t)
{
    if (t >= TOK_ASM_r0 && t <= TOK_ASM_pc) { /* register name */
        switch (t) {
            case TOK_ASM_fp:
                return TOK_ASM_r11 - TOK_ASM_r0;
            case TOK_ASM_ip:
                return TOK_ASM_r12 - TOK_ASM_r0;
            case TOK_ASM_sp:
                return TOK_ASM_r13 - TOK_ASM_r0;
            case TOK_ASM_lr:
                return TOK_ASM_r14 - TOK_ASM_r0;
            case TOK_ASM_pc:
                return TOK_ASM_r15 - TOK_ASM_r0;
            default:
                return t - TOK_ASM_r0;
        }
    } else
        return -1;
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
