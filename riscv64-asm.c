/*************************************************************/
/*
 *  RISCV64 assembler for TCC
 *
 */

#ifdef TARGET_DEFS_ONLY

#define CONFIG_TCC_ASM
#define NB_ASM_REGS 32

ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);

/*************************************************************/
#else
/*************************************************************/
#define USING_GLOBALS
#include "tcc.h"

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

static void asm_emit_opcode(uint32_t opcode) {
    gen_le32(opcode);
}

static void asm_nullary_opcode(TCCState *s1, int token)
{
    switch (token) {
    // Sync instructions

    case TOK_ASM_fence: // I
        asm_emit_opcode((0x3 << 2) | 3 | (0 << 12));
        return;
    case TOK_ASM_fence_i: // I
        asm_emit_opcode((0x3 << 2) | 3| (1 << 12));
        return;

    // System calls

    case TOK_ASM_scall: // I (pseudo)
        asm_emit_opcode((0x1C << 2) | 3 | (0 << 12));
        return;
    case TOK_ASM_sbreak: // I (pseudo)
        asm_emit_opcode((0x1C << 2) | 3 | (0 << 12) | (1 << 20));
        return;

    // Privileged Instructions

    case TOK_ASM_ecall:
        asm_emit_opcode((0x1C << 2) | 3 | (0 << 20));
        return;
    case TOK_ASM_ebreak:
        asm_emit_opcode((0x1C << 2) | 3 | (1 << 20));
        return;

    // Other

    case TOK_ASM_wfi:
        asm_emit_opcode((0x1C << 2) | 3 | (0x105 << 20));
        return;

    default:
        expect("nullary instruction");
    }
}

enum {
    OPT_REG,
    OPT_IM12S,
    OPT_IM32,
};
#define OP_REG    (1 << OPT_REG)
#define OP_IM32   (1 << OPT_IM32)
#define OP_IM12S   (1 << OPT_IM12S)

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

    op->type = 0;

    if ((reg = asm_parse_regvar(tok)) != -1) {
        next(); // skip register name
        op->type = OP_REG;
        op->reg = (uint8_t) reg;
        return;
    } else if (tok == '$') {
        /* constant value */
        next(); // skip '#' or '$'
    }
    asm_expr(s1, &e);
    op->type = OP_IM32;
    op->e = e;
    if (!op->e.sym) {
        if ((int) op->e.v >= -2048 && (int) op->e.v < 2048)
            op->type = OP_IM12S;
    } else
        expect("operand");
}

#define ENCODE_RS1(register_index) ((register_index) << 15)
#define ENCODE_RS2(register_index) ((register_index) << 20)
#define ENCODE_RD(register_index) ((register_index) << 7)

// Note: Those all map to CSR--so they are pseudo-instructions.
static void asm_unary_opcode(TCCState *s1, int token)
{
    uint32_t opcode = (0x1C << 2) | 3 | (2 << 12);
    Operand op;
    parse_operand(s1, &op);
    if (op.type != OP_REG) {
        expect("register");
        return;
    }
    opcode |= ENCODE_RD(op.reg);

    switch (token) {
    case TOK_ASM_rdcycle:
        asm_emit_opcode(opcode | (0xC00 << 20));
        return;
    case TOK_ASM_rdcycleh:
        asm_emit_opcode(opcode | (0xC80 << 20));
        return;
    case TOK_ASM_rdtime:
        asm_emit_opcode(opcode | (0xC01 << 20) | ENCODE_RD(op.reg));
        return;
    case TOK_ASM_rdtimeh:
        asm_emit_opcode(opcode | (0xC81 << 20) | ENCODE_RD(op.reg));
        return;
    case TOK_ASM_rdinstret:
        asm_emit_opcode(opcode | (0xC02 << 20) | ENCODE_RD(op.reg));
        return;
    case TOK_ASM_rdinstreth:
        asm_emit_opcode(opcode | (0xC82 << 20) | ENCODE_RD(op.reg));
        return;
    default:
        expect("unary instruction");
    }
}

ST_FUNC void asm_opcode(TCCState *s1, int token)
{
    switch (token) {
    case TOK_ASM_fence:
    case TOK_ASM_fence_i:
    case TOK_ASM_scall:
    case TOK_ASM_sbreak:
    case TOK_ASM_ecall:
    case TOK_ASM_ebreak:
    case TOK_ASM_mrts:
    case TOK_ASM_mrth:
    case TOK_ASM_hrts:
    case TOK_ASM_wfi:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_rdcycle:
    case TOK_ASM_rdcycleh:
    case TOK_ASM_rdtime:
    case TOK_ASM_rdtimeh:
    case TOK_ASM_rdinstret:
    case TOK_ASM_rdinstreth:
        asm_unary_opcode(s1, token);
        return;

    default:
        expect("known instruction");
    }
}

ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier)
{
    tcc_error("RISCV64 asm not implemented.");
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
    tcc_error("RISCV64 asm not implemented.");
}

ST_FUNC int asm_parse_regvar (int t)
{
    tcc_error("RISCV64 asm not implemented.");
    return -1;
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
