/*************************************************************/
/*
 *  ARM64 dummy assembler for TCC
 *
 */

#ifdef TARGET_DEFS_ONLY

#define CONFIG_TCC_ASM
#define NB_ASM_REGS 16

ST_FUNC void g(TCCState *S, int c);
ST_FUNC void gen_le16(TCCState *S, int c);
ST_FUNC void gen_le32(TCCState *S, int c);

/*************************************************************/
#else
/*************************************************************/
#define USING_GLOBALS
#include "tcc.h"

static void asm_error(TCCState *S)
{
    tcc_error(S, "ARM asm not implemented.");
}

/* XXX: make it faster ? */
ST_FUNC void g(TCCState *S, int c)
{
    int ind1;
    if (S->nocode_wanted)
        return;
    ind1 = S->ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(S, cur_text_section, ind1);
    cur_text_section->data[S->ind] = c;
    S->ind = ind1;
}

ST_FUNC void gen_le16 (TCCState *S, int i)
{
    g(S, i);
    g(S, i>>8);
}

ST_FUNC void gen_le32 (TCCState *S, int i)
{
    gen_le16(S, i);
    gen_le16(S, i>>16);
}

ST_FUNC void gen_expr32(TCCState *S, ExprValue *pe)
{
    gen_le32(S, pe->v);
}

ST_FUNC void asm_opcode(TCCState *S, int opcode)
{
    asm_error(S);
}

ST_FUNC void subst_asm_operand(TCCState *S, CString *add_str, SValue *sv, int modifier)
{
    asm_error(S);
}

/* generate prolog and epilog code for asm statement */
ST_FUNC void asm_gen_code(TCCState *S, ASMOperand *operands, int nb_operands,
                         int nb_outputs, int is_output,
                         uint8_t *clobber_regs,
                         int out_reg)
{
}

ST_FUNC void asm_compute_constraints(TCCState *S, ASMOperand *operands,
                                    int nb_operands, int nb_outputs,
                                    const uint8_t *clobber_regs,
                                    int *pout_reg)
{
}

ST_FUNC void asm_clobber(TCCState *S, uint8_t *clobber_regs, const char *str)
{
    asm_error(S);
}

ST_FUNC int asm_parse_regvar (TCCState *S, int t)
{
    asm_error(S);
    return -1;
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
