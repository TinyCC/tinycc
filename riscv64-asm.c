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

enum {
    OPT_REG,
    OPT_IM12S,
    OPT_IM32,
};
#define C_ENCODE_RS1(register_index) ((register_index) << 7)
#define C_ENCODE_RS2(register_index) ((register_index) << 2)
#define ENCODE_RD(register_index) ((register_index) << 7)
#define ENCODE_RS1(register_index) ((register_index) << 15)
#define ENCODE_RS2(register_index) ((register_index) << 20)
#define NTH_BIT(b, n) ((b >> n) & 1)
#define OP_IM12S (1 << OPT_IM12S)
#define OP_IM32 (1 << OPT_IM32)
#define OP_REG (1 << OPT_REG)

typedef struct Operand {
    uint32_t type;
    union {
        uint8_t reg;
        uint16_t regset;
        ExprValue e;
    };
} Operand;

static void asm_binary_opcode(TCCState* s1, int token);
ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str);
ST_FUNC void asm_compute_constraints(ASMOperand *operands, int nb_operands, int nb_outputs, const uint8_t *clobber_regs, int *pout_reg);
static void asm_emit_b(int token, uint32_t opcode, const Operand *rs1, const Operand *rs2, const Operand *imm);
static void asm_emit_i(int token, uint32_t opcode, const Operand *rd, const Operand *rs1, const Operand *rs2);
static void asm_emit_j(int token, uint32_t opcode, const Operand *rd, const Operand *rs2);
static void asm_emit_opcode(uint32_t opcode);
static void asm_emit_r(int token, uint32_t opcode, const Operand *rd, const Operand *rs1, const Operand *rs2);
static void asm_emit_s(int token, uint32_t opcode, const Operand *rs1, const Operand *rs2, const Operand *imm);
static void asm_emit_u(int token, uint32_t opcode, const Operand *rd, const Operand *rs2);
ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands, int nb_outputs, int is_output, uint8_t *clobber_regs, int out_reg);
static void asm_nullary_opcode(TCCState *s1, int token);
ST_FUNC void asm_opcode(TCCState *s1, int token);
static int asm_parse_csrvar(int t);
ST_FUNC int asm_parse_regvar(int t);
static void asm_ternary_opcode(TCCState *s1, int token);
static void asm_unary_opcode(TCCState *s1, int token);
ST_FUNC void gen_expr32(ExprValue *pe);
static void parse_operand(TCCState *s1, Operand *op);
ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier);
/* C extension */
static void asm_emit_ca(int token, uint16_t opcode, const Operand *rd, const Operand *rs2);
static void asm_emit_cb(int token, uint16_t opcode, const Operand *rs1, const Operand *imm);
static void asm_emit_ci(int token, uint16_t opcode, const Operand *rd, const Operand *imm);
static void asm_emit_ciw(int token, uint16_t opcode, const Operand *rd, const Operand *imm);
static void asm_emit_cj(int token, uint16_t opcode, const Operand *imm);
static void asm_emit_cl(int token, uint16_t opcode, const Operand *rd, const Operand *rs1, const Operand *imm);
static void asm_emit_cr(int token, uint16_t opcode, const Operand *rd, const Operand *rs2);
static void asm_emit_cs(int token, uint16_t opcode, const Operand *rs2, const Operand *rs1, const Operand *imm);
static void asm_emit_css(int token, uint16_t opcode, const Operand *rs2, const Operand *imm);

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

static void asm_emit_opcode(uint32_t opcode) {
    gen_le32(opcode);
}

static void asm_nullary_opcode(TCCState *s1, int token)
{
    static const Operand nil = {.type = OP_REG};
    static const Operand zimm = {.type = OP_IM12S};

    switch (token) {
    // Sync instructions

    case TOK_ASM_fence: // I
        asm_emit_opcode((0x3 << 2) | 3 | (0 << 12));
        return;
    case TOK_ASM_fence_i: // I
        asm_emit_opcode((0x3 << 2) | 3| (1 << 12));
        return;

    // System calls

    case TOK_ASM_ecall: // I (pseudo)
        asm_emit_opcode((0x1C << 2) | 3 | (0 << 12));
        return;
    case TOK_ASM_ebreak: // I (pseudo)
        asm_emit_opcode((0x1C << 2) | 3 | (0 << 12) | (1 << 20));
        return;

    // Other

    case TOK_ASM_nop:
        asm_emit_i(token, (4 << 2) | 3, &nil, &nil, &zimm);
        return;

    case TOK_ASM_wfi:
        asm_emit_opcode((0x1C << 2) | 3 | (0x105 << 20));
        return;

    /* C extension */
    case TOK_ASM_c_ebreak:
        asm_emit_cr(token, 2 | (9 << 12), &nil, &nil);
        return;
    case TOK_ASM_c_nop:
        asm_emit_ci(token, 1, &nil, &zimm);
        return;

    default:
        expect("nullary instruction");
    }
}

/* Parse a text containing operand and store the result in OP */
static void parse_operand(TCCState *s1, Operand *op)
{
    ExprValue e = {0};
    Sym label = {0};
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
    } else if ((e.v = asm_parse_csrvar(tok)) != -1) {
        next();
    } else {
        asm_expr(s1, &e);
    }
    op->type = OP_IM32;
    op->e = e;
    /* compare against unsigned 12-bit maximum */
    if (!op->e.sym) {
        if ((int) op->e.v >= -0x1000 && (int) op->e.v < 0x1000)
            op->type = OP_IM12S;
    } else if (op->e.sym->type.t & (VT_EXTERN | VT_STATIC)) {
        label.type.t = VT_VOID | VT_STATIC;

        /* use the medium PIC model: GOT, auipc, lw */
        if (op->e.sym->type.t & VT_STATIC)
            greloca(cur_text_section, op->e.sym, ind, R_RISCV_PCREL_HI20, 0);
        else
            greloca(cur_text_section, op->e.sym, ind, R_RISCV_GOT_HI20, 0);
        put_extern_sym(&label, cur_text_section, ind, 0);
        greloca(cur_text_section, &label, ind+4, R_RISCV_PCREL_LO12_I, 0);

        op->type = OP_IM12S;
        op->e.v = 0;
    } else {
        expect("operand");
    }
}

static void asm_unary_opcode(TCCState *s1, int token)
{
    uint32_t opcode = (0x1C << 2) | 3 | (2 << 12);
    Operand op;
    static const Operand nil = {.type = OP_REG};

    parse_operand(s1, &op);
    /* Note: Those all map to CSR--so they are pseudo-instructions. */
    opcode |= ENCODE_RD(op.reg);

    switch (token) {
    /* pseudoinstructions */
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
    /* C extension */
    case TOK_ASM_c_j:
        asm_emit_cj(token, 1 | (5 << 13), &op);
        return;
    case TOK_ASM_c_jal: /* RV32C-only */
        asm_emit_cj(token, 1 | (1 << 13), &op);
        return;
    case TOK_ASM_c_jalr:
        asm_emit_cr(token, 2 | (9 << 12), &op, &nil);
        return;
    case TOK_ASM_c_jr:
        asm_emit_cr(token, 2 | (8 << 12), &op, &nil);
        return;
    default:
        expect("unary instruction");
    }
}

static void asm_emit_u(int token, uint32_t opcode, const Operand* rd, const Operand* rs2)
{
    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_IM12S && rs2->type != OP_IM32) {
        tcc_error("'%s': Expected second source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    } else if (rs2->e.v >= 0x100000) {
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 0xfffff", get_tok_str(token, NULL));
        return;
    }
    /* U-type instruction:
	      31...12 imm[31:12]
	      11...7 rd
	      6...0 opcode */
    gen_le32(opcode | ENCODE_RD(rd->reg) | (rs2->e.v << 12));
}

static void asm_binary_opcode(TCCState* s1, int token)
{
    Operand ops[2];
    parse_operand(s1, &ops[0]);
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);

    switch (token) {
    case TOK_ASM_lui:
        asm_emit_u(token, (0xD << 2) | 3, &ops[0], &ops[1]);
        return;
    case TOK_ASM_auipc:
        asm_emit_u(token, (0x05 << 2) | 3, &ops[0], &ops[1]);
        return;
    case TOK_ASM_jal:
        asm_emit_j(token, 0x6f, ops, ops + 1);
        return;

    /* C extension */
    case TOK_ASM_c_add:
        asm_emit_cr(token, 2 | (9 << 12), ops, ops + 1);
        return;
    case TOK_ASM_c_mv:
        asm_emit_cr(token, 2 | (8 << 12), ops, ops + 1);
        return;

    case TOK_ASM_c_addi16sp:
        asm_emit_ci(token, 1 | (3 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_addi:
        asm_emit_ci(token, 1, ops, ops + 1);
        return;
    case TOK_ASM_c_addiw:
        asm_emit_ci(token, 1 | (1 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_fldsp:
        asm_emit_ci(token, 2 | (1 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_flwsp: /* RV32FC-only */
        asm_emit_ci(token, 2 | (3 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_ldsp:
        asm_emit_ci(token, 2 | (3 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_li:
        asm_emit_ci(token, 1 | (2 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_lui:
        asm_emit_ci(token, 1 | (3 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_lwsp:
        asm_emit_ci(token, 2 | (2 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_slli:
        asm_emit_ci(token, 2, ops, ops + 1);
        return;

    case TOK_ASM_c_addi4spn:
        asm_emit_ciw(token, 0, ops, ops + 1);
        return;

#define CA (1 | (3 << 10) | (4 << 13))
    case TOK_ASM_c_addw:
        asm_emit_ca(token, CA | (1 << 5) | (1 << 12), ops, ops + 1);
        return;
    case TOK_ASM_c_and:
        asm_emit_ca(token, CA | (3 << 5), ops, ops + 1);
        return;
    case TOK_ASM_c_or:
        asm_emit_ca(token, CA | (2 << 5), ops, ops + 1);
        return;
    case TOK_ASM_c_sub:
        asm_emit_ca(token, CA, ops, ops + 1);
        return;
    case TOK_ASM_c_subw:
        asm_emit_ca(token, CA | (1 << 12), ops, ops + 1);
        return;
    case TOK_ASM_c_xor:
        asm_emit_ca(token, CA | (1 << 5), ops, ops + 1);
        return;
#undef CA

    case TOK_ASM_c_andi:
        asm_emit_cb(token, 1 | (2 << 10) | (4 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_beqz:
        asm_emit_cb(token, 1 | (6 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_bnez:
        asm_emit_cb(token, 1 | (7 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_srai:
        asm_emit_cb(token, 1 | (1 << 10) | (4 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_srli:
        asm_emit_cb(token, 1 | (4 << 13), ops, ops + 1);
        return;

    case TOK_ASM_c_sdsp:
        asm_emit_css(token, 2 | (7 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_swsp:
        asm_emit_css(token, 2 | (6 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_fswsp: /* RV32FC-only */
        asm_emit_css(token, 2 | (7 << 13), ops, ops + 1);
        return;
    case TOK_ASM_c_fsdsp:
        asm_emit_css(token, 2 | (5 << 13), ops, ops + 1);
        return;

    /* pseudoinstructions */
    /* rd, sym */
    case TOK_ASM_la:
        /* auipc rd, 0 */
        asm_emit_u(token, 3 | (5 << 2), ops, ops + 1);
        /* lw rd, rd, 0 */
        asm_emit_i(token, 3 | (2 << 12), ops, ops, ops + 1);
        return;
    case TOK_ASM_lla:
        /* auipc rd, 0 */
        asm_emit_u(token, 3 | (5 << 2), ops, ops + 1);
        /* addi rd, rd, 0 */
        asm_emit_i(token, 3 | (4 << 2), ops, ops, ops + 1);
        return;

    default:
        expect("binary instruction");
    }
}

/* caller: Add funct3, funct7 into opcode */
static void asm_emit_r(int token, uint32_t opcode, const Operand* rd, const Operand* rs1, const Operand* rs2)
{
    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected first source operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected second source operand that is a register or immediate", get_tok_str(token, NULL));
        return;
    }
    /* R-type instruction:
	     31...25 funct7
	     24...20 rs2
	     19...15 rs1
	     14...12 funct3
	     11...7 rd
	     6...0 opcode */
    gen_le32(opcode | ENCODE_RD(rd->reg) | ENCODE_RS1(rs1->reg) | ENCODE_RS2(rs2->reg));
}

/* caller: Add funct3 into opcode */
static void asm_emit_i(int token, uint32_t opcode, const Operand* rd, const Operand* rs1, const Operand* rs2)
{
    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected first source operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_IM12S) {
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 8191", get_tok_str(token, NULL));
        return;
    }
    /* I-type instruction:
	     31...20 imm[11:0]
	     19...15 rs1
	     14...12 funct3
	     11...7 rd
	     6...0 opcode */

    gen_le32(opcode | ENCODE_RD(rd->reg) | ENCODE_RS1(rs1->reg) | (rs2->e.v << 20));
}

static void asm_emit_j(int token, uint32_t opcode, const Operand* rd, const Operand* rs2)
{
    uint32_t imm;

    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_IM12S && rs2->type != OP_IM32) {
        tcc_error("'%s': Expected second source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    imm = rs2->e.v;

    /* even offsets in a +- 1 MiB range */
    if (imm > 0x1ffffe) {
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 0x1fffff", get_tok_str(token, NULL));
        return;
    }

    if (imm & 1) {
        tcc_error("'%s': Expected second source operand that is an even immediate value", get_tok_str(token, NULL));
        return;
    }
    /* J-type instruction:
    31      imm[20]
    30...21 imm[10:1]
    20      imm[11]
    19...12 imm[19:12]
    11...7  rd
    6...0   opcode */
    gen_le32(opcode | ENCODE_RD(rd->reg) | (((imm >> 20) & 1) << 31) | (((imm >> 1) & 0x3ff) << 21) | (((imm >> 11) & 1) << 20) | (((imm >> 12) & 0xff) << 12));
}

static void asm_ternary_opcode(TCCState *s1, int token)
{
    Operand ops[3];
    parse_operand(s1, &ops[0]);
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[2]);

    switch (token) {
    case TOK_ASM_sll:
        asm_emit_r(token, (0xC << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_slli:
        asm_emit_i(token, (4 << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_srl:
        asm_emit_r(token, (0xC << 2) | 3 | (4 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_srli:
        asm_emit_i(token, (0x4 << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_sra:
        asm_emit_r(token, (0xC << 2) | 3 | (5 << 12) | (32 << 25), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_srai:
        asm_emit_i(token, (0x4 << 2) | 3 | (5 << 12) | (16 << 26), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_sllw:
        asm_emit_r(token, (0xE << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_slliw:
        asm_emit_i(token, (6 << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_srlw:
        asm_emit_r(token, (0xE << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_srliw:
        asm_emit_i(token, (0x6 << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_sraw:
        asm_emit_r(token, (0xE << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
        return;
    case TOK_ASM_sraiw:
        asm_emit_i(token, (0x6 << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
        return;

    // Arithmetic (RD,RS1,(RS2|IMM)); R-format, I-format or U-format

    case TOK_ASM_add:
         asm_emit_r(token, (0xC << 2) | 3, &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_addi:
         asm_emit_i(token, (4 << 2) | 3, &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_sub:
         asm_emit_r(token, (0xC << 2) | 3 | (32 << 25), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_addw:
         asm_emit_r(token, (0xE << 2) | 3 | (0 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_addiw: // 64 bit
         asm_emit_i(token, (0x6 << 2) | 3 | (0 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_subw:
         asm_emit_r(token, (0xE << 2) | 3 | (0 << 12) | (32 << 25), &ops[0], &ops[1], &ops[2]);
         return;

    // Logical (RD,RS1,(RS2|IMM)); R-format or I-format

    case TOK_ASM_xor:
         asm_emit_r(token, (0xC << 2) | 3 | (4 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_xori:
         asm_emit_i(token, (0x4 << 2) | 3 | (4 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_or:
         asm_emit_r(token, (0xC << 2) | 3 | (6 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_ori:
         asm_emit_i(token, (0x4 << 2) | 3 | (6 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_and:
         asm_emit_r(token, (0xC << 2) | 3 | (7 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_andi:
         asm_emit_i(token, (0x4 << 2) | 3 | (7 << 12), &ops[0], &ops[1], &ops[2]);
         return;

    // Compare (RD,RS1,(RS2|IMM)); R-format or I-format

    case TOK_ASM_slt:
         asm_emit_r(token, (0xC << 2) | 3 | (2 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_slti:
         asm_emit_i(token, (0x4 << 2) | 3 | (2 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_sltu:
         asm_emit_r(token, (0xC << 2) | 3 | (3 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_sltiu:
         asm_emit_i(token, (0x4 << 2) | 3 | (3 << 12), &ops[0], &ops[1], &ops[2]);
         return;

    /* indirect jump (RD, RS1, IMM); I-format */
    case TOK_ASM_jalr:
        asm_emit_i(token, 0x67 | (0 << 12), ops, ops + 1, ops + 2);
        return;

    /* branch (RS1, RS2, IMM); B-format */
    case TOK_ASM_beq:
        asm_emit_b(token, 0x63 | (0 << 12), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_bne:
        asm_emit_b(token, 0x63 | (1 << 12), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_blt:
        asm_emit_b(token, 0x63 | (4 << 12), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_bge:
        asm_emit_b(token, 0x63 | (5 << 12), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_bltu:
        asm_emit_b(token, 0x63 | (6 << 12), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_bgeu:
        asm_emit_b(token, 0x63 | (7 << 12), ops, ops + 1, ops + 2);
        return;

    // Loads (RD,RS1,I); I-format

    case TOK_ASM_lb:
         asm_emit_i(token, (0x0 << 2) | 3, &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lh:
         asm_emit_i(token, (0x0 << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lw:
         asm_emit_i(token, (0x0 << 2) | 3 | (2 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lbu:
         asm_emit_i(token, (0x0 << 2) | 3 | (4 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lhu:
         asm_emit_i(token, (0x0 << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    // 64 bit
    case TOK_ASM_ld:
         asm_emit_i(token, (0x0 << 2) | 3 | (3 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lwu:
         asm_emit_i(token, (0x0 << 2) | 3 | (6 << 12), &ops[0], &ops[1], &ops[2]);
         return;

    // Stores (RS1,RS2,I); S-format

    case TOK_ASM_sb:
         asm_emit_s(token, (0x8 << 2) | 3 | (0 << 12), &ops[0], &ops[1], &ops[2]);
         return;
   case TOK_ASM_sh:
         asm_emit_s(token, (0x8 << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_sw:
         asm_emit_s(token, (0x8 << 2) | 3 | (2 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_sd:
         asm_emit_s(token, (0x8 << 2) | 3 | (3 << 12), &ops[0], &ops[1], &ops[2]);
         return;

    /* M extension */
    case TOK_ASM_div:
        asm_emit_r(token, 0x33 | (4 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_divu:
        asm_emit_r(token, 0x33 | (5 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_divuw:
        asm_emit_r(token, 0x3b | (5 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_divw:
        asm_emit_r(token, 0x3b | (4 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_mul:
        asm_emit_r(token, 0x33 | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_mulh:
        asm_emit_r(token, 0x33 | (1 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_mulhsu:
        asm_emit_r(token, 0x33 | (2 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_mulhu:
        asm_emit_r(token, 0x33 | (3 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_mulw:
        asm_emit_r(token, 0x3b | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_rem:
        asm_emit_r(token, 0x33 | (6 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_remu:
        asm_emit_r(token, 0x33 | (7 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_remuw:
        asm_emit_r(token, 0x3b | (7 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_remw:
        asm_emit_r(token, 0x3b | (6 << 12) | (1 << 25), ops, ops + 1, ops + 2);
        return;

    /* Zicsr extension; (rd, csr, rs/uimm) */
    case TOK_ASM_csrrc:
        asm_emit_i(token, 0x73 | (3 << 12), ops, ops + 2, ops + 1);
        return;
    case TOK_ASM_csrrci:
        /* using rs1 field for uimmm */
        ops[2].type = OP_REG;
        asm_emit_i(token, 0x73 | (7 << 12), ops, ops + 2, ops + 1);
        return;
    case TOK_ASM_csrrs:
        asm_emit_i(token, 0x73 | (2 << 12), ops, ops + 2, ops + 1);
        return;
    case TOK_ASM_csrrsi:
        ops[2].type = OP_REG;
        asm_emit_i(token, 0x73 | (6 << 12), ops, ops + 2, ops + 1);
        return;
    case TOK_ASM_csrrw:
        asm_emit_i(token, 0x73 | (1 << 12), ops, ops + 2, ops + 1);
        return;
    case TOK_ASM_csrrwi:
        ops[2].type = OP_REG;
        asm_emit_i(token, 0x73 | (5 << 12), ops, ops + 2, ops + 1);
        return;

    /* C extension */
    /* register-based loads and stores (RD, RS1, IMM); CL-format */
    case TOK_ASM_c_fld:
        asm_emit_cl(token, 1 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_flw: /* RV32FC-only */
        asm_emit_cl(token, 3 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_fsd:
        asm_emit_cs(token, 5 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_fsw: /* RV32FC-only */
        asm_emit_cs(token, 7 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_ld:
        asm_emit_cl(token, 3 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_lw:
        asm_emit_cl(token, 2 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_sd:
        asm_emit_cs(token, 7 << 13, ops, ops + 1, ops + 2);
        return;
    case TOK_ASM_c_sw:
        asm_emit_cs(token, 6 << 13, ops, ops + 1, ops + 2);
        return;

    default:
        expect("ternary instruction");
    }
}

/* caller: Add funct3 to opcode */
static void asm_emit_s(int token, uint32_t opcode, const Operand* rs1, const Operand* rs2, const Operand* imm)
{
    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected first source operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected second source operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (imm->type != OP_IM12S) {
        tcc_error("'%s': Expected third operand that is an immediate value between 0 and 8191", get_tok_str(token, NULL));
        return;
    }
    {
        uint16_t v = imm->e.v;
        /* S-type instruction:
	        31...25 imm[11:5]
	        24...20 rs2
	        19...15 rs1
	        14...12 funct3
	        11...7 imm[4:0]
	        6...0 opcode
        opcode always fixed pos. */
        gen_le32(opcode | ENCODE_RS1(rs1->reg) | ENCODE_RS2(rs2->reg) | ((v & 0x1F) << 7) | ((v >> 5) << 25));
    }
}

static void asm_emit_b(int token, uint32_t opcode, const Operand *rs1, const Operand *rs2, const Operand *imm)
{
    uint32_t offset;

    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected first source operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (imm->type != OP_IM12S) {
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 8191", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    /* B-type instruction:
    31      imm[12]
    30...25 imm[10:5]
    24...20 rs2
    19...15 rs1
    14...12 funct3
    8...11  imm[4:1]
    7       imm[11]
    6...0   opcode */
    asm_emit_opcode(opcode | ENCODE_RS1(rs1->reg) | ENCODE_RS2(rs2->reg) | (((offset >> 1) & 0xF) << 8) | (((offset >> 5) & 0x1f) << 25) | (((offset >> 11) & 1) << 7) | (((offset >> 12) & 1) << 31));
}

ST_FUNC void asm_opcode(TCCState *s1, int token)
{
    switch (token) {
    case TOK_ASM_ebreak:
    case TOK_ASM_ecall:
    case TOK_ASM_fence:
    case TOK_ASM_fence_i:
    case TOK_ASM_hrts:
    case TOK_ASM_mrth:
    case TOK_ASM_mrts:
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

    case TOK_ASM_lui:
    case TOK_ASM_auipc:
    case TOK_ASM_jal:
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_add:
    case TOK_ASM_addi:
    case TOK_ASM_addiw:
    case TOK_ASM_addw:
    case TOK_ASM_and:
    case TOK_ASM_andi:
    case TOK_ASM_beq:
    case TOK_ASM_bge:
    case TOK_ASM_bgeu:
    case TOK_ASM_blt:
    case TOK_ASM_bltu:
    case TOK_ASM_bne:
    case TOK_ASM_jalr:
    case TOK_ASM_lb:
    case TOK_ASM_lbu:
    case TOK_ASM_ld:
    case TOK_ASM_lh:
    case TOK_ASM_lhu:
    case TOK_ASM_lw:
    case TOK_ASM_lwu:
    case TOK_ASM_or:
    case TOK_ASM_ori:
    case TOK_ASM_sb:
    case TOK_ASM_sd:
    case TOK_ASM_sh:
    case TOK_ASM_sll:
    case TOK_ASM_slli:
    case TOK_ASM_slliw:
    case TOK_ASM_sllw:
    case TOK_ASM_slt:
    case TOK_ASM_slti:
    case TOK_ASM_sltiu:
    case TOK_ASM_sltu:
    case TOK_ASM_sra:
    case TOK_ASM_srai:
    case TOK_ASM_sraiw:
    case TOK_ASM_sraw:
    case TOK_ASM_srl:
    case TOK_ASM_srli:
    case TOK_ASM_srliw:
    case TOK_ASM_srlw:
    case TOK_ASM_sub:
    case TOK_ASM_subw:
    case TOK_ASM_sw:
    case TOK_ASM_xor:
    case TOK_ASM_xori:
    /* M extension */
    case TOK_ASM_div:
    case TOK_ASM_divu:
    case TOK_ASM_divuw:
    case TOK_ASM_divw:
    case TOK_ASM_mul:
    case TOK_ASM_mulh:
    case TOK_ASM_mulhsu:
    case TOK_ASM_mulhu:
    case TOK_ASM_mulw:
    case TOK_ASM_rem:
    case TOK_ASM_remu:
    case TOK_ASM_remuw:
    case TOK_ASM_remw:
    /* Zicsr extension */
    case TOK_ASM_csrrc:
    case TOK_ASM_csrrci:
    case TOK_ASM_csrrs:
    case TOK_ASM_csrrsi:
    case TOK_ASM_csrrw:
    case TOK_ASM_csrrwi:
        asm_ternary_opcode(s1, token);
        return;

    /* C extension */
    case TOK_ASM_c_ebreak:
    case TOK_ASM_c_nop:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_c_j:
    case TOK_ASM_c_jal:
    case TOK_ASM_c_jalr:
    case TOK_ASM_c_jr:
        asm_unary_opcode(s1, token);
        return;

    case TOK_ASM_c_add:
    case TOK_ASM_c_addi16sp:
    case TOK_ASM_c_addi4spn:
    case TOK_ASM_c_addi:
    case TOK_ASM_c_addiw:
    case TOK_ASM_c_addw:
    case TOK_ASM_c_and:
    case TOK_ASM_c_andi:
    case TOK_ASM_c_beqz:
    case TOK_ASM_c_bnez:
    case TOK_ASM_c_fldsp:
    case TOK_ASM_c_flwsp:
    case TOK_ASM_c_fsdsp:
    case TOK_ASM_c_fswsp:
    case TOK_ASM_c_ldsp:
    case TOK_ASM_c_li:
    case TOK_ASM_c_lui:
    case TOK_ASM_c_lwsp:
    case TOK_ASM_c_mv:
    case TOK_ASM_c_or:
    case TOK_ASM_c_sdsp:
    case TOK_ASM_c_slli:
    case TOK_ASM_c_srai:
    case TOK_ASM_c_srli:
    case TOK_ASM_c_sub:
    case TOK_ASM_c_subw:
    case TOK_ASM_c_swsp:
    case TOK_ASM_c_xor:
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_c_fld:
    case TOK_ASM_c_flw:
    case TOK_ASM_c_fsd:
    case TOK_ASM_c_fsw:
    case TOK_ASM_c_ld:
    case TOK_ASM_c_lw:
    case TOK_ASM_c_sd:
    case TOK_ASM_c_sw:
        asm_ternary_opcode(s1, token);
        return;

    /* pseudoinstructions */
    case TOK_ASM_nop:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_la:
    case TOK_ASM_lla:
        asm_binary_opcode(s1, token);
        return;

    default:
        expect("known instruction");
    }
}

static int asm_parse_csrvar(int t)
{
    switch (t) {
    case TOK_ASM_cycle:
        return 0xc00;
    case TOK_ASM_fcsr:
        return 3;
    case TOK_ASM_fflags:
        return 1;
    case TOK_ASM_frm:
        return 2;
    case TOK_ASM_instret:
        return 0xc02;
    case TOK_ASM_time:
        return 0xc01;
    case TOK_ASM_cycleh:
        return 0xc80;
    case TOK_ASM_instreth:
        return 0xc82;
    case TOK_ASM_timeh:
        return 0xc81;
    default:
        return -1;
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

ST_FUNC int asm_parse_regvar (int t)
{
    /* PC register not implemented */
    if (t >= TOK_ASM_pc || t < TOK_ASM_x0)
        return -1;

    if (t < TOK_ASM_f0)
        return t - TOK_ASM_x0;

    if (t < TOK_ASM_zero)
        return t - TOK_ASM_f0;

    /* ABI mnemonic */
    if (t < TOK_ASM_ft0)
        return t - TOK_ASM_zero;

    return t - TOK_ASM_ft0;
}

/*************************************************************/
/* C extension */

/* caller: Add funct6, funct2 into opcode */
static void asm_emit_ca(int token, uint16_t opcode, const Operand *rd, const Operand *rs2)
{
    uint8_t dst, src;

    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected source operand that is a register", get_tok_str(token, NULL));
        return;
    }

    /* subtract index of x8 */
    dst = rd->reg - 8;
    src = rs2->reg - 8;

    /* only registers {x,f}8 to {x,f}15 are valid (3-bit) */
    if (dst > 7) {
        tcc_error("'%s': Expected destination operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    if (src > 7) {
        tcc_error("'%s': Expected source operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    /* CA-type instruction:
    15...10 funct6
    9...7   rd'/rs1'
    6..5    funct2
    4...2   rs2'
    1...0   opcode */

    gen_le16(opcode | C_ENCODE_RS2(src) | C_ENCODE_RS1(dst));
}

static void asm_emit_cb(int token, uint16_t opcode, const Operand *rs1, const Operand *imm)
{
    uint32_t offset;
    uint8_t src;

    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected source operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    if (offset & 1) {
        tcc_error("'%s': Expected source operand that is an even immediate value", get_tok_str(token, NULL));
        return;
    }

    src = rs1->reg - 8;

    if (src > 7) {
        tcc_error("'%s': Expected source operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    /* CB-type instruction:
    15...13 funct3
    12...10 offset
    9..7    rs1'
    6...2   offset
    1...0   opcode */

    /* non-branch also using CB:
    15...13 funct3
    12      imm
    11..10  funct2
    9...7   rd'/rs1'
    6..2    imm
    1...0   opcode */

    switch (token) {
    case TOK_ASM_c_beqz:
    case TOK_ASM_c_bnez:
        gen_le16(opcode | C_ENCODE_RS1(src) | ((NTH_BIT(offset, 5) | (((offset >> 1) & 3) << 1) | (((offset >> 6) & 3) << 3)) << 2) | ((((offset >> 3) & 3) | NTH_BIT(offset, 8)) << 10));
        return;
    default:
        gen_le16(opcode | C_ENCODE_RS1(src) | ((offset & 0x1f) << 2) | (NTH_BIT(offset, 5) << 12));
        return;
    }
}

static void asm_emit_ci(int token, uint16_t opcode, const Operand *rd, const Operand *imm)
{
    uint32_t immediate;

    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    immediate = imm->e.v;

    /* CI-type instruction:
    15...13 funct3
    12      imm
    11...7  rd/rs1
    6...2   imm
    1...0   opcode */

    switch (token) {
    case TOK_ASM_c_addi:
    case TOK_ASM_c_addiw:
    case TOK_ASM_c_li:
    case TOK_ASM_c_slli:
        gen_le16(opcode | ((immediate & 0x1f) << 2) | ENCODE_RD(rd->reg) | (NTH_BIT(immediate, 5) << 12));
        return;
    case TOK_ASM_c_addi16sp:
        gen_le16(opcode | NTH_BIT(immediate, 5) << 2 | (((immediate >> 7) & 3) << 3) | NTH_BIT(immediate, 6) << 5 | NTH_BIT(immediate, 4) << 6 | ENCODE_RD(rd->reg) | (NTH_BIT(immediate, 9) << 12));
        return;
    case TOK_ASM_c_lui:
        gen_le16(opcode | (((immediate >> 12) & 0x1f) << 2) | ENCODE_RD(rd->reg) | (NTH_BIT(immediate, 17) << 12));
        return;
    case TOK_ASM_c_fldsp:
    case TOK_ASM_c_ldsp:
        gen_le16(opcode | (((immediate >> 6) & 7) << 2) | (((immediate >> 3) & 2) << 5) | ENCODE_RD(rd->reg) | (NTH_BIT(immediate, 5) << 12));
        return;
    case TOK_ASM_c_flwsp:
    case TOK_ASM_c_lwsp:
        gen_le16(opcode | (((immediate >> 6) & 3) << 2) | (((immediate >> 2) & 7) << 4) | ENCODE_RD(rd->reg) | (NTH_BIT(immediate, 5) << 12));
        return;
    case TOK_ASM_c_nop:
        gen_le16(opcode);
        return;
    default:
        expect("known instruction");
    }
}

/* caller: Add funct3 into opcode */
static void asm_emit_ciw(int token, uint16_t opcode, const Operand *rd, const Operand *imm)
{
    uint32_t nzuimm;
    uint8_t dst;

    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    dst = rd->reg - 8;

    if (dst > 7) {
        tcc_error("'%s': Expected destination operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    nzuimm = imm->e.v;

    if (nzuimm > 0x3fc) {
        tcc_error("'%s': Expected source operand that is an immediate value between 0 and 0x3ff", get_tok_str(token, NULL));
        return;
    }

    if (nzuimm & 3) {
        tcc_error("'%s': Expected source operand that is a non-zero immediate value divisible by 4", get_tok_str(token, NULL));
        return;
    }

    /* CIW-type instruction:
    15...13 funct3
    12...5  imm
    4...2   rd'
    1...0   opcode */

    gen_le16(opcode | ENCODE_RS2(rd->reg) | ((NTH_BIT(nzuimm, 3) | (NTH_BIT(nzuimm, 2) << 1) | (((nzuimm >> 6) & 0xf) << 2) | (((nzuimm >> 4) & 3) << 6)) << 5));
}

/* caller: Add funct3 into opcode */
static void asm_emit_cj(int token, uint16_t opcode, const Operand *imm)
{
    uint32_t offset;

    /* +-2 KiB range */
    if (imm->type != OP_IM12S) {
        tcc_error("'%s': Expected source operand that is a 12-bit immediate value", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    if (offset & 1) {
        tcc_error("'%s': Expected source operand that is an even immediate value", get_tok_str(token, NULL));
        return;
    }

    /* CJ-type instruction:
    15...13 funct3
    12...2  offset[11|4|9:8|10|6|7|3:1|5]
    1...0   opcode */

    gen_le16(opcode | (NTH_BIT(offset, 5) << 2) | (((offset >> 1) & 7) << 3) | (NTH_BIT(offset, 7) << 6) | (NTH_BIT(offset, 6) << 7) | (NTH_BIT(offset, 10) << 8) | (((offset >> 8) & 3) << 9) | (NTH_BIT(offset, 4) << 11) | (NTH_BIT(offset, 11) << 12));
}

/* caller: Add funct3 into opcode */
static void asm_emit_cl(int token, uint16_t opcode, const Operand *rd, const Operand *rs1, const Operand *imm)
{
    uint32_t offset;
    uint8_t dst, src;

    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected source operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    dst = rd->reg - 8;
    src = rs1->reg - 8;

    if (dst > 7) {
        tcc_error("'%s': Expected destination operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    if (src > 7) {
        tcc_error("'%s': Expected source operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    if (offset > 0xff) {
        tcc_error("'%s': Expected source operand that is an immediate value between 0 and 0xff", get_tok_str(token, NULL));
        return;
    }

    if (offset & 3) {
        tcc_error("'%s': Expected source operand that is an immediate value divisible by 4", get_tok_str(token, NULL));
        return;
    }

    /* CL-type instruction:
    15...13 funct3
    12...10 imm
    9...7   rs1'
    6...5   imm
    4...2   rd'
    1...0   opcode */

    switch (token) {
    /* imm variant 1 */
    case TOK_ASM_c_flw:
    case TOK_ASM_c_lw:
        gen_le16(opcode | C_ENCODE_RS2(dst) | C_ENCODE_RS1(src) | (NTH_BIT(offset, 6) << 5) | (NTH_BIT(offset, 2) << 6) | (((offset >> 3) & 7) << 10));
        return;
    /* imm variant 2 */
    case TOK_ASM_c_fld:
    case TOK_ASM_c_ld:
        gen_le16(opcode | C_ENCODE_RS2(dst) | C_ENCODE_RS1(src) | (((offset >> 6) & 3) << 5) | (((offset >> 3) & 7) << 10));
        return;
    default:
        expect("known instruction");
    }
}

/* caller: Add funct4 into opcode */
static void asm_emit_cr(int token, uint16_t opcode, const Operand *rd, const Operand *rs2)
{
    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected source operand that is a register", get_tok_str(token, NULL));
        return;
    }

    /* CR-type instruction:
    15...12 funct4
    11..7   rd/rs1
    6...2   rs2
    1...0   opcode */

    gen_le16(opcode | C_ENCODE_RS1(rd->reg) | C_ENCODE_RS2(rs2->reg));
}

/* caller: Add funct3 into opcode */
static void asm_emit_cs(int token, uint16_t opcode, const Operand *rs2, const Operand *rs1, const Operand *imm)
{
    uint32_t offset;
    uint8_t base, src;

    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (rs1->type != OP_REG) {
        tcc_error("'%s': Expected source operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    base = rs1->reg - 8;
    src = rs2->reg - 8;

    if (base > 7) {
        tcc_error("'%s': Expected destination operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    if (src > 7) {
        tcc_error("'%s': Expected source operand that is a valid C-extension register", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    if (offset > 0xff) {
        tcc_error("'%s': Expected source operand that is an immediate value between 0 and 0xff", get_tok_str(token, NULL));
        return;
    }

    if (offset & 3) {
        tcc_error("'%s': Expected source operand that is an immediate value divisible by 4", get_tok_str(token, NULL));
        return;
    }

    /* CS-type instruction:
    15...13 funct3
    12...10 imm
    9...7   rs1'
    6...5   imm
    4...2   rs2'
    1...0   opcode */
    switch (token) {
    /* imm variant 1 */
    case TOK_ASM_c_fsw:
    case TOK_ASM_c_sw:
        gen_le16(opcode | C_ENCODE_RS2(base) | C_ENCODE_RS1(src) | (NTH_BIT(offset, 6) << 5) | (NTH_BIT(offset, 2) << 6) | (((offset >> 3) & 7) << 10));
        return;
    /* imm variant 2 */
    case TOK_ASM_c_fsd:
    case TOK_ASM_c_sd:
        gen_le16(opcode | C_ENCODE_RS2(base) | C_ENCODE_RS1(src) | (((offset >> 6) & 3) << 5) | (((offset >> 3) & 7) << 10));
        return;
    default:
        expect("known instruction");
    }
}

/* caller: Add funct3 into opcode */
static void asm_emit_css(int token, uint16_t opcode, const Operand *rs2, const Operand *imm)
{
    uint32_t offset;

    if (rs2->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }

    if (imm->type != OP_IM12S && imm->type != OP_IM32) {
        tcc_error("'%s': Expected source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    }

    offset = imm->e.v;

    if (offset > 0xff) {
        tcc_error("'%s': Expected source operand that is an immediate value between 0 and 0xff", get_tok_str(token, NULL));
        return;
    }

    if (offset & 3) {
        tcc_error("'%s': Expected source operand that is an immediate value divisible by 4", get_tok_str(token, NULL));
        return;
    }

    /* CSS-type instruction:
    15...13 funct3
    12...7  imm
    6...2   rs2
    1...0   opcode */

    switch (token) {
    /* imm variant 1 */
    case TOK_ASM_c_fswsp:
    case TOK_ASM_c_swsp:
        gen_le16(opcode | ENCODE_RS2(rs2->reg) | (((offset >> 6) & 3) << 7) | (((offset >> 2) & 0xf) << 9));
        return;
    /* imm variant 2 */
    case TOK_ASM_c_fsdsp:
    case TOK_ASM_c_sdsp:
        gen_le16(opcode | ENCODE_RS2(rs2->reg) | (((offset >> 6) & 7) << 7) | (((offset >> 3) & 7) << 10));
        return;
    default:
        expect("known instruction");
    }
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
