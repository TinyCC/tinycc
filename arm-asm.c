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
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 4;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind++] = i & 0xFF;
    cur_text_section->data[ind++] = (i >> 8) & 0xFF;
    cur_text_section->data[ind++] = (i >> 16) & 0xFF;
    cur_text_section->data[ind++] = (i >> 24) & 0xFF;
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

#define ENCODE_BARREL_SHIFTER_SHIFT_BY_REGISTER (1 << 4)
#define ENCODE_BARREL_SHIFTER_MODE_LSL (0 << 5)
#define ENCODE_BARREL_SHIFTER_MODE_LSR (1 << 5)
#define ENCODE_BARREL_SHIFTER_MODE_ASR (2 << 5)
#define ENCODE_BARREL_SHIFTER_MODE_ROR (3 << 5)
#define ENCODE_BARREL_SHIFTER_REGISTER(register_index) ((register_index) << 8)
#define ENCODE_BARREL_SHIFTER_IMMEDIATE(value) ((value) << 7)

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

static uint32_t asm_encode_rotation(Operand* rotation)
{
    uint64_t amount;
    switch (rotation->type) {
    case OP_REG32:
        tcc_error("cannot rotate immediate value by register");
        return 0;
    case OP_IM8:
        amount = rotation->e.v;
        if (amount >= 0 && amount < 32 && (amount & 1) == 0)
            return (amount >> 1) << 8;
        else
            tcc_error("rotating is only possible by a multiple of 2");
        break;
    default:
        tcc_error("unknown rotation amount");
        return 0;
    }
}

static uint32_t asm_encode_shift(Operand* shift)
{
    uint64_t amount;
    uint32_t operands = 0;
    switch (shift->type) {
    case OP_REG32:
        if (shift->reg == 15)
            tcc_error("r15 cannot be used as a shift count");
        else {
            operands = ENCODE_BARREL_SHIFTER_SHIFT_BY_REGISTER;
            operands |= ENCODE_BARREL_SHIFTER_REGISTER(shift->reg);
        }
        break;
    case OP_IM8:
        amount = shift->e.v;
        if (amount > 0 && amount < 32)
            operands = ENCODE_BARREL_SHIFTER_IMMEDIATE(amount);
        else
            tcc_error("shift count out of range");
        break;
    default:
        tcc_error("unknown shift amount");
    }
    return operands;
}

static void asm_data_processing_opcode(TCCState *s1, int token)
{
    Operand ops[3];
    int nb_ops;

    /* 16 entries per instruction for the different condition codes */
    uint32_t opcode_idx = (ARM_INSTRUCTION_GROUP(token) - TOK_ASM_andeq) >> 4;

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
        memcpy(&ops[2], &ops[1], sizeof(ops[1])); // move ops[2]
        memcpy(&ops[1], &ops[0], sizeof(ops[0])); // ops[1] was implicit
        nb_ops = 3;
    }
    if (nb_ops != 3) {
        expect("two or three operands");
        return;
    } else {
        uint32_t opcode = 0;
        uint32_t operands = 0;

        // data processing (general case):
        // operands:
        //   Rn: bits 19...16 (first operand)
        //   Rd: bits 15...12 (destination)
        //   Operand2: bits 11...0 (second operand);  depending on I that's either a register or an immediate
        // operator:
        //   bits 24...21: "OpCode"--see below

        /* operations in the token list are ordered by opcode */
        opcode = (opcode_idx >> 1) << 21; // drop "s"
        if (ops[0].type != OP_REG32)
            expect("(destination operand) register");
        else if (opcode == 0xa << 21 || opcode == 0xb << 21 || opcode == 0x8 << 21 || opcode == 0x9 << 21 ) // cmp, cmn, tst, teq
            operands |= ENCODE_SET_CONDITION_CODES; // force S set, otherwise it's a completely different instruction.
        else
            operands |= ENCODE_RD(ops[0].reg);
        if (ops[1].type != OP_REG32)
            expect("(first source operand) register");
        else if (!(opcode == 0xd << 21 || opcode == 0xf << 21)) // not: mov, mvn (those have only one source operand)
            operands |= ENCODE_RN(ops[1].reg);
        switch (ops[2].type) {
        case OP_REG32:
            // TODO: Parse and encode shift.
            operands |= ops[2].reg;
            break;
        case OP_IM8:
            // TODO: Parse and encode rotation.
            operands |= ENCODE_IMMEDIATE_FLAG;
            operands |= ops[2].e.v;
            break;
        case OP_IM8N: // immediate negative value
            // TODO: Parse and encode rotation.
            operands |= ENCODE_IMMEDIATE_FLAG;
            /* Instruction swapping:
               0001 = EOR - Rd:= Op1 EOR Op2     -> difficult
               0011 = RSB - Rd:= Op2 - Op1       -> difficult
               0111 = RSC - Rd:= Op2 - Op1 + C   -> difficult
               1000 = TST - CC on: Op1 AND Op2   -> difficult
               1001 = TEQ - CC on: Op1 EOR Op2   -> difficult
               1100 = ORR - Rd:= Op1 OR Op2      -> difficult
            */
            switch (opcode_idx >> 1) { // "OpCode" in ARM docs
#if 0
            case 0x0: // AND - Rd:= Op1 AND Op2
                opcode = 0xe << 21; // BIC
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
            case 0x2: // SUB - Rd:= Op1 - Op2
                opcode = 0x4 << 21; // ADD
                operands |= (-ops[2].e.v) & 0xFF;
                break;
            case 0x4: // ADD - Rd:= Op1 + Op2
                opcode = 0x2 << 21; // SUB
                operands |= (-ops[2].e.v) & 0xFF;
                break;
            case 0x5: // ADC - Rd:= Op1 + Op2 + C
                opcode = 0x6 << 21; // SBC
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
            case 0x6: // SBC - Rd:= Op1 - Op2 + C
                opcode = 0x5 << 21; // ADC
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
#endif
            case 0xa: // CMP - CC on: Op1 - Op2
                opcode = 0xb << 21; // CMN
                operands |= (-ops[2].e.v) & 0xFF;
                break;
            case 0xb: // CMN - CC on: Op1 + Op2
                opcode = 0xa << 21; // CMP
                operands |= (-ops[2].e.v) & 0xFF;
                break;
            // moveq r1, r3: 0x01a01003; mov Rd, Op2
            case 0xd: // MOV - Rd:= Op2
                opcode = 0xf << 21; // MVN
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
#if 0
            case 0xe: // BIC - Rd:= Op1 AND NOT Op2
                opcode = 0x0 << 21; // AND
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
#endif
            case 0xf: // MVN - Rd:= NOT Op2
                opcode = 0xd << 21; // MOV
                operands |= (ops[2].e.v ^ 0xFF) & 0xFF;
                break;
            default:
                tcc_error("cannot use '%s' with a negative immediate value", get_tok_str(token, NULL));
            }
            break;
        default:
            expect("(second source operand) register or immediate value");
        }

        /* S=0 and S=1 entries alternate one after another, in that order */
        opcode |= (opcode_idx & 1) ? ENCODE_SET_CONDITION_CODES : 0;
        asm_emit_opcode(token, opcode | operands);
    }
}

static void asm_shift_opcode(TCCState *s1, int token)
{
    Operand ops[3];
    int nb_ops;
    uint32_t opcode = 0xd << 21; // MOV
    uint32_t operands = 0;

    for (nb_ops = 0; nb_ops < sizeof(ops)/sizeof(ops[0]); ++nb_ops) {
        parse_operand(s1, &ops[nb_ops]);
        if (tok != ',') {
            ++nb_ops;
            break;
        }
        next(); // skip ','
    }
    if (nb_ops < 2) {
        expect("at least two operands");
        return;
    }

    if (ops[0].type != OP_REG32)
        expect("(destination operand) register");
    else
        operands |= ENCODE_RD(ops[0].reg);

    if (nb_ops == 2) {
        switch (ARM_INSTRUCTION_GROUP(token)) {
        case TOK_ASM_rrxseq:
            opcode |= ENCODE_SET_CONDITION_CODES;
            /* fallthrough */
        case TOK_ASM_rrxeq:
            if (ops[1].type == OP_REG32) {
                operands |= ops[1].reg;
                operands |= ENCODE_BARREL_SHIFTER_MODE_ROR;
                asm_emit_opcode(token, opcode | operands);
            } else
                tcc_error("(first source operand) register");
            return;
        default:
            memcpy(&ops[2], &ops[1], sizeof(ops[1])); // move ops[2]
            memcpy(&ops[1], &ops[0], sizeof(ops[0])); // ops[1] was implicit
            nb_ops = 3;
        }
    }
    if (nb_ops != 3) {
        expect("two or three operands");
        return;
    }

    switch (ops[1].type) {
    case OP_REG32:
        operands |= ops[1].reg;
        break;
    case OP_IM8:
        operands |= ENCODE_IMMEDIATE_FLAG;
        operands |= ops[1].e.v;
        break;
    }

    if (operands & ENCODE_IMMEDIATE_FLAG)
        operands |= asm_encode_rotation(&ops[2]);
    else
        operands |= asm_encode_shift(&ops[2]);

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_lslseq:
        opcode |= ENCODE_SET_CONDITION_CODES;
        /* fallthrough */
    case TOK_ASM_lsleq:
        operands |= ENCODE_BARREL_SHIFTER_MODE_LSL;
        break;
    case TOK_ASM_lsrseq:
        opcode |= ENCODE_SET_CONDITION_CODES;
        /* fallthrough */
    case TOK_ASM_lsreq:
        operands |= ENCODE_BARREL_SHIFTER_MODE_LSR;
        break;
    case TOK_ASM_asrseq:
        opcode |= ENCODE_SET_CONDITION_CODES;
        /* fallthrough */
    case TOK_ASM_asreq:
        operands |= ENCODE_BARREL_SHIFTER_MODE_ASR;
        break;
    case TOK_ASM_rorseq:
        opcode |= ENCODE_SET_CONDITION_CODES;
        /* fallthrough */
    case TOK_ASM_roreq:
        operands |= ENCODE_BARREL_SHIFTER_MODE_ROR;
        break;
    default:
        expect("shift instruction");
        return;
    }
    asm_emit_opcode(token, opcode | operands);
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

static void asm_single_data_transfer_opcode(TCCState *s1, int token)
{
    Operand ops[3];
    int exclam = 0;
    int closed_bracket = 0;
    int op2_minus = 0;
    uint32_t opcode = 1 << 26;
    // Note: ldr r0, [r4, #4]  ; simple offset: r0 = *(int*)(r4+4); r4 unchanged
    // Note: ldr r0, [r4, #4]! ; pre-indexed:   r0 = *(int*)(r4+4); r4 = r4+4
    // Note: ldr r0, [r4], #4  ; post-indexed:  r0 = *(int*)(r4+0); r4 = r4+4

    parse_operand(s1, &ops[0]);
    if (ops[0].type == OP_REG32)
        opcode |= ENCODE_RD(ops[0].reg);
    else {
        expect("(destination operand) register");
        return;
    }
    if (tok != ',')
        expect("two arguments");
    else
        next(); // skip ','
    if (tok != '[')
        expect("'['");
    else
        next(); // skip '['

    parse_operand(s1, &ops[1]);
    if (ops[1].type == OP_REG32)
        opcode |= ENCODE_RN(ops[1].reg);
    else {
        expect("(first source operand) register");
        return;
    }
    if (tok == ']') {
        next();
        closed_bracket = 1;
        // exclam = 1; // implicit in hardware; don't do it in software
    }
    if (tok != ',')
        expect("','");
    else
        next(); // skip ','
    if (tok == '-') {
        op2_minus = 1;
        next();
    }
    parse_operand(s1, &ops[2]);
    if (!closed_bracket) {
        if (tok != ']')
            expect("']'");
        else
            next(); // skip ']'
        opcode |= 1 << 24; // add offset before transfer
        if (tok == '!') {
            exclam = 1;
            next(); // skip '!'
        }
    }

    // single data transfer: 0 1 I P U B W L << 20 (general case):
    // operands:
    //    Rd: destination operand [ok]
    //    Rn: first source operand [ok]
    //    Operand2: bits 11...0 [ok]
    // I: immediate operand? [ok]
    // P: Pre/post indexing is PRE: Add offset before transfer [ok]
    // U: Up/down is up? (*adds* offset to base) [ok]
    // B: Byte/word is byte?  TODO
    // W: Write address back into base? [ok]
    // L: Load/store is load? [ok]
    if (exclam)
        opcode |= 1 << 21; // write offset back into register

    if (ops[2].type == OP_IM32 || ops[2].type == OP_IM8 || ops[2].type == OP_IM8N) {
        int v = ops[2].e.v;
        if (op2_minus)
            tcc_error("minus before '#' not supported for immediate values");
        if (v >= 0) {
            opcode |= 1 << 23; // up
            if (v >= 0x1000)
                tcc_error("offset out of range for '%s'", get_tok_str(token, NULL));
            else
                opcode |= v;
        } else { // down
            if (v <= -0x1000)
                tcc_error("offset out of range for '%s'", get_tok_str(token, NULL));
            else
                opcode |= -v;
        }
    } else if (ops[2].type == OP_REG32) {
        if (!op2_minus)
            opcode |= 1 << 23; // up
        opcode |= ENCODE_IMMEDIATE_FLAG; /* if set, it means it's NOT immediate */
        opcode |= ops[2].reg;
    } else
        expect("register");

    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_strbeq:
        opcode |= 1 << 22; // B
        /* fallthrough */
    case TOK_ASM_streq:
        asm_emit_opcode(token, opcode);
        break;
    case TOK_ASM_ldrbeq:
        opcode |= 1 << 22; // B
        /* fallthrough */
    case TOK_ASM_ldreq:
        opcode |= 1 << 20; // L
        asm_emit_opcode(token, opcode);
        break;
    default:
        expect("data transfer instruction");
    }
}

/* Note: almost dupe of encbranch in arm-gen.c */
static uint32_t encbranchoffset(int pos, int addr, int fail)
{
  addr-=pos+8;
  addr/=4;
  if(addr>=0x1000000 || addr<-0x1000000) { // FIXME: Is that correct?
    if(fail)
      tcc_error("function bigger than 32MB");
    return 0;
  }
  return /*not 0x0A000000|*/(addr&0xffffff);
}

static void asm_branch_opcode(TCCState *s1, int token)
{
    int jmp_disp = 0;
    Operand op;
    parse_operand(s1, &op);
    if (op.type == OP_IM32 || op.type == OP_IM8 || op.type == OP_IM8N) {
        jmp_disp = encbranchoffset(ind, op.e.v, 0);
        if (jmp_disp < -0x800000 || jmp_disp > 0x7fffff) {
            tcc_error("branch is too far");
            return;
        }
    }
    switch (ARM_INSTRUCTION_GROUP(token)) {
    case TOK_ASM_beq:
        if (op.type == OP_IM32 || op.type == OP_IM8 || op.type == OP_IM8N)
            asm_emit_opcode(token, (0xa << 24) | (jmp_disp & 0xffffff));
        else
            expect("branch target");
        break;
    case TOK_ASM_bleq:
        if (op.type == OP_IM32 || op.type == OP_IM8 || op.type == OP_IM8N)
            asm_emit_opcode(token, (0xb << 24) | (jmp_disp & 0xffffff));
        else
            expect("branch target");
        break;
    case TOK_ASM_bxeq:
        if (op.type != OP_REG32)
            expect("register");
        else
            asm_emit_opcode(token, (0x12fff1 << 4) | op.reg);
        break;
    case TOK_ASM_blxeq:
        if (op.type != OP_REG32)
            expect("register");
        else
            asm_emit_opcode(token, (0x12fff3 << 4) | op.reg);
        break;
    default:
        expect("branch instruction");
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
    case TOK_ASM_beq:
    case TOK_ASM_bleq:
    case TOK_ASM_bxeq:
    case TOK_ASM_blxeq:
        return asm_branch_opcode(s1, token);
    case TOK_ASM_clzeq:
    case TOK_ASM_sxtbeq:
    case TOK_ASM_sxtheq:
    case TOK_ASM_uxtbeq:
    case TOK_ASM_uxtheq:
        return asm_binary_opcode(s1, token);

    case TOK_ASM_ldreq:
    case TOK_ASM_ldrbeq:
    case TOK_ASM_streq:
    case TOK_ASM_strbeq:
        return asm_single_data_transfer_opcode(s1, token);

    case TOK_ASM_andeq:
    case TOK_ASM_eoreq:
    case TOK_ASM_subeq:
    case TOK_ASM_rsbeq:
    case TOK_ASM_addeq:
    case TOK_ASM_adceq:
    case TOK_ASM_sbceq:
    case TOK_ASM_rsceq:
    case TOK_ASM_tsteq:
    case TOK_ASM_teqeq:
    case TOK_ASM_cmpeq:
    case TOK_ASM_cmneq:
    case TOK_ASM_orreq:
    case TOK_ASM_moveq:
    case TOK_ASM_biceq:
    case TOK_ASM_mvneq:
    case TOK_ASM_andseq:
    case TOK_ASM_eorseq:
    case TOK_ASM_subseq:
    case TOK_ASM_rsbseq:
    case TOK_ASM_addseq:
    case TOK_ASM_adcseq:
    case TOK_ASM_sbcseq:
    case TOK_ASM_rscseq:
//  case TOK_ASM_tstseq:
//  case TOK_ASM_teqseq:
//  case TOK_ASM_cmpseq:
//  case TOK_ASM_cmnseq:
    case TOK_ASM_orrseq:
    case TOK_ASM_movseq:
    case TOK_ASM_bicseq:
    case TOK_ASM_mvnseq:
        return asm_data_processing_opcode(s1, token);

    case TOK_ASM_lsleq:
    case TOK_ASM_lslseq:
    case TOK_ASM_lsreq:
    case TOK_ASM_lsrseq:
    case TOK_ASM_asreq:
    case TOK_ASM_asrseq:
    case TOK_ASM_roreq:
    case TOK_ASM_rorseq:
    case TOK_ASM_rrxseq:
    case TOK_ASM_rrxeq:
        return asm_shift_opcode(s1, token);

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
