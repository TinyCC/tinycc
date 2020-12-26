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

static void asm_unary_opcode(TCCState *s1, int token)
{
    Operand op;
    parse_operand(s1, &op);

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_swieq:
        if (op.type != OP_IM8)
            expect("immediate 8-bit unsigned integer");
        else {
            /* Note: Dummy operand (ignored by processor): ARM ref documented 0...255, ARM instruction set documented 24 bit */
            asm_emit_opcode(token, (0xf << 24) | op.e.v);
        }
        break;
    default:
        expect("unary instruction");
    }
}

static void asm_binary_opcode(TCCState *s1, int token)
{
    Operand ops[2];
    parse_operand(s1, &ops[0]);
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);
    if (ops[0].type != OP_REG32) {
        expect("(destination operand) register");
        return;
    }

    if (ops[1].type != OP_REG32)
        expect("(source operand) register");
    else switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_clzeq:
        asm_emit_opcode(token, 0x16f0f10 | (ops[0].reg << 12) | ops[1].reg);
        break;
    case TOK_ASM_sxtbeq:
        /* TODO: optional ROR (8|16|24) */
        asm_emit_opcode(token, 0x6af0070 | (ops[0].reg << 12) | ops[1].reg);
        break;
    case TOK_ASM_sxtheq:
        /* TODO: optional ROR (8|16|24) */
        asm_emit_opcode(token, 0x6bf0070 | (ops[0].reg << 12) | ops[1].reg);
        break;
    case TOK_ASM_uxtbeq:
        /* TODO: optional ROR (8|16|24) */
        asm_emit_opcode(token, 0x6ef0070 | (ops[0].reg << 12) | ops[1].reg);
        break;
    case TOK_ASM_uxtheq:
        /* TODO: optional ROR (8|16|24) */
        asm_emit_opcode(token, 0x6ff0070 | (ops[0].reg << 12) | ops[1].reg);
        break;
    default:
        expect("binary instruction");
    }
}

/* data processing and single data transfer instructions only */
#define ENCODE_RN(register_index) ((register_index) << 16)
#define ENCODE_RD(register_index) ((register_index) << 12)
#define ENCODE_SET_CONDITION_CODES (1 << 20)

/* Note: For data processing instructions, "1" means immediate.
   Note: For single data transfer instructions, "0" means immediate. */
#define ENCODE_IMMEDIATE_FLAG (1 << 25)

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
    case TOK_ASM_stmdaeq:
    case TOK_ASM_ldmdaeq:
    case TOK_ASM_stmeq:
    case TOK_ASM_ldmeq:
    case TOK_ASM_stmiaeq:
    case TOK_ASM_ldmiaeq:
    case TOK_ASM_stmdbeq:
    case TOK_ASM_ldmdbeq:
    case TOK_ASM_stmibeq:
    case TOK_ASM_ldmibeq:
        switch (ARM_INSTRUCTION_GROUP(token)) {
        case TOK_ASM_stmdaeq: // post-decrement store
            opcode = 0x82 << 20;
            break;
        case TOK_ASM_ldmdaeq: // post-decrement load
            opcode = 0x83 << 20;
            break;
        case TOK_ASM_stmeq: // post-increment store
        case TOK_ASM_stmiaeq: // post-increment store
            opcode = 0x8a << 20;
            break;
        case TOK_ASM_ldmeq: // post-increment load
        case TOK_ASM_ldmiaeq: // post-increment load
            opcode = 0x8b << 20;
            break;
        case TOK_ASM_stmdbeq: // pre-decrement store
            opcode = 0x92 << 20;
            break;
        case TOK_ASM_ldmdbeq: // pre-decrement load
            opcode = 0x93 << 20;
            break;
        case TOK_ASM_stmibeq: // pre-increment store
            opcode = 0x9a << 20;
            break;
        case TOK_ASM_ldmibeq: // pre-increment load
            opcode = 0x9b << 20;
            break;
        default:
            tcc_error("internal error: This place should not be reached (fallback in asm_block_data_transfer_opcode)");
        }
        // operands:
        //    Rn: first operand
        //    Register List: lower bits
        if (nb_ops != 2)
            expect("exactly two operands");
        else if (ops[0].type != OP_REG32)
            expect("(first operand) register");
        else if (!op0_exclam)
            tcc_error("first operand of '%s' should have an exclamation mark", get_tok_str(token, NULL));
        else
            asm_emit_opcode(token, opcode | ENCODE_RN(ops[0].reg) | ops[1].regset);
        break;
    default:
        expect("block data transfer instruction");
    }
}

static void asm_multiplication_opcode(TCCState *s1, int token)
{
    Operand ops[4];
    int nb_ops = 0;
    uint32_t opcode = 0x90;

    for (nb_ops = 0; nb_ops < sizeof(ops)/sizeof(ops[0]); ++nb_ops) {
        parse_operand(s1, &ops[nb_ops]);
        if (tok != ',') {
            ++nb_ops;
            break;
        }
        next(); // skip ','
    }
    if (nb_ops < 2)
        expect("at least two operands");
    else if (nb_ops == 2) {
        switch (ARM_INSTRUCTION_GROUP(token)) {
        case TOK_ASM_mulseq:
        case TOK_ASM_muleq:
            memcpy(&ops[2], &ops[0], sizeof(ops[1])); // ARM is actually like this!
            break;
        default:
            memcpy(&ops[2], &ops[1], sizeof(ops[1])); // move ops[2]
            memcpy(&ops[1], &ops[0], sizeof(ops[0])); // ops[1] was implicit
        }
        nb_ops = 3;
    }

    // multiply (special case):
    // operands:
    //   Rd: bits 19...16
    //   Rm: bits 3...0
    //   Rs: bits 11...8
    //   Rn: bits 15...12

    if (ops[0].type == OP_REG32)
        opcode |= ops[0].reg << 16;
    else
        expect("(destination operand) register");
    if (ops[1].type == OP_REG32)
        opcode |= ops[1].reg;
    else
        expect("(first source operand) register");
    if (ops[2].type == OP_REG32)
        opcode |= ops[2].reg << 8;
    else
        expect("(second source operand) register");
    if (nb_ops > 3) {
        if (ops[3].type == OP_REG32)
            opcode |= ops[3].reg << 12;
        else
            expect("(third source operand) register");
    }

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_mulseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_muleq:
        if (nb_ops != 3)
            expect("three operands");
        else {
            asm_emit_opcode(token, opcode);
        }
        break;
    case TOK_ASM_mlaseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_mlaeq:
        if (nb_ops != 4)
            expect("four operands");
        else {
            opcode |= 1 << 21; // Accumulate
            asm_emit_opcode(token, opcode);
        }
        break;
    default:
        expect("known multiplication instruction");
    }
}

static void asm_long_multiplication_opcode(TCCState *s1, int token)
{
    Operand ops[4];
    int nb_ops = 0;
    uint32_t opcode = 0x90 | (1 << 23);

    for (nb_ops = 0; nb_ops < sizeof(ops)/sizeof(ops[0]); ++nb_ops) {
        parse_operand(s1, &ops[nb_ops]);
        if (tok != ',') {
            ++nb_ops;
            break;
        }
        next(); // skip ','
    }
    if (nb_ops != 4) {
        expect("four operands");
        return;
    }

    // long multiply (special case):
    // operands:
    //   RdLo: bits 15...12
    //   RdHi: bits 19...16
    //   Rs: bits 11...8
    //   Rm: bits 3...0

    if (ops[0].type == OP_REG32)
        opcode |= ops[0].reg << 12;
    else
        expect("(destination lo accumulator) register");
    if (ops[1].type == OP_REG32)
        opcode |= ops[1].reg << 16;
    else
        expect("(destination hi accumulator) register");
    if (ops[2].type == OP_REG32)
        opcode |= ops[2].reg;
    else
        expect("(first source operand) register");
    if (ops[3].type == OP_REG32)
        opcode |= ops[3].reg << 8;
    else
        expect("(second source operand) register");

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_smullseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_smulleq:
        opcode |= 1 << 22; // signed
        asm_emit_opcode(token, opcode);
        break;
    case TOK_ASM_umullseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_umulleq:
        asm_emit_opcode(token, opcode);
        break;
    case TOK_ASM_smlalseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_smlaleq:
        opcode |= 1 << 22; // signed
        opcode |= 1 << 21; // Accumulate
        asm_emit_opcode(token, opcode);
        break;
    case TOK_ASM_umlalseq:
        opcode |= 1 << 20; // Status
        /* fallthrough */
    case TOK_ASM_umlaleq:
        opcode |= 1 << 21; // Accumulate
        asm_emit_opcode(token, opcode);
        break;
    default:
        expect("known long multiplication instruction");
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
    case TOK_ASM_stmdaeq:
    case TOK_ASM_ldmdaeq:
    case TOK_ASM_stmeq:
    case TOK_ASM_ldmeq:
    case TOK_ASM_stmiaeq:
    case TOK_ASM_ldmiaeq:
    case TOK_ASM_stmdbeq:
    case TOK_ASM_ldmdbeq:
    case TOK_ASM_stmibeq:
    case TOK_ASM_ldmibeq:
        return asm_block_data_transfer_opcode(s1, token);
    case TOK_ASM_nopeq:
    case TOK_ASM_wfeeq:
    case TOK_ASM_wfieq:
        return asm_nullary_opcode(token);
    case TOK_ASM_swieq:
        return asm_unary_opcode(s1, token);
    case TOK_ASM_clzeq:
    case TOK_ASM_sxtbeq:
    case TOK_ASM_sxtheq:
    case TOK_ASM_uxtbeq:
    case TOK_ASM_uxtheq:
        return asm_binary_opcode(s1, token);

    case TOK_ASM_muleq:
    case TOK_ASM_mulseq:
    case TOK_ASM_mlaeq:
    case TOK_ASM_mlaseq:
        return asm_multiplication_opcode(s1, token);

    case TOK_ASM_smulleq:
    case TOK_ASM_smullseq:
    case TOK_ASM_umulleq:
    case TOK_ASM_umullseq:
    case TOK_ASM_smlaleq:
    case TOK_ASM_smlalseq:
    case TOK_ASM_umlaleq:
    case TOK_ASM_umlalseq:
        return asm_long_multiplication_opcode(s1, token);
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
