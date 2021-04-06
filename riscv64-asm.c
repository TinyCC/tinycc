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
