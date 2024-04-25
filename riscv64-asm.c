/*************************************************************/
/*
 *  RISCV64 assembler for TCC
 *
 */

#ifdef TARGET_DEFS_ONLY

#define CONFIG_TCC_ASM
/* 32 general purpose + 32 floating point registers */
#define NB_ASM_REGS 64

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
// Registers go from 0 to 31. We use next bit to choose general/float
#define REG_FLOAT_MASK 0x20
#define REG_IS_FLOAT(register_index) ((register_index) & REG_FLOAT_MASK)
#define REG_VALUE(register_index)    ((register_index) & (REG_FLOAT_MASK-1))
#define C_ENCODE_RS1(register_index) (REG_VALUE(register_index) << 7)
#define C_ENCODE_RS2(register_index) (REG_VALUE(register_index) << 2)
#define ENCODE_RD(register_index)  (REG_VALUE(register_index) << 7)
#define ENCODE_RS1(register_index) (REG_VALUE(register_index) << 15)
#define ENCODE_RS2(register_index) (REG_VALUE(register_index) << 20)
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
static void asm_emit_a(int token, uint32_t opcode, const Operand *rs1, const Operand *rs2, const Operand *rd1, int aq, int rl);
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
static void asm_branch_opcode(TCCState *s1, int token, int argc);
ST_FUNC void gen_expr32(ExprValue *pe);
static void parse_operand(TCCState *s1, Operand *op);
static void parse_branch_offset_operand(TCCState *s1, Operand *op);
static void parse_operands(TCCState *s1, Operand *ops, int count);
static void parse_mem_access_operands(TCCState *s1, Operand* ops);
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

    /* Pseudoinstructions */
    case TOK_ASM_ret:
        /* jalr zero, x1, 0 */
        asm_emit_opcode( 0x67 | (0 << 12) | ENCODE_RS1(1) );
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

static void parse_branch_offset_operand(TCCState *s1, Operand *op){
    ExprValue e = {0};

    asm_expr(s1, &e);
    op->type = OP_IM32;
    op->e = e;
    /* compare against unsigned 12-bit maximum */
    if (!op->e.sym) {
        if ((int) op->e.v >= -0x1000 && (int) op->e.v < 0x1000)
            op->type = OP_IM12S;
    } else if (op->e.sym->type.t & (VT_EXTERN | VT_STATIC)) {
        greloca(cur_text_section, op->e.sym, ind, R_RISCV_BRANCH, 0);

        /* XXX: Implement far branches */

        op->type = OP_IM12S;
        op->e.v = 0;
    } else {
        expect("operand");
    }
}

static void parse_jump_offset_operand(TCCState *s1, Operand *op){
    ExprValue e = {0};

    asm_expr(s1, &e);
    op->type = OP_IM32;
    op->e = e;
    /* compare against unsigned 12-bit maximum */
    if (!op->e.sym) {
        if ((int) op->e.v >= -0x1000 && (int) op->e.v < 0x1000)
            op->type = OP_IM12S;
    } else if (op->e.sym->type.t & (VT_EXTERN | VT_STATIC)) {
        greloca(cur_text_section, op->e.sym, ind, R_RISCV_JAL, 0);
        op->type = OP_IM12S;
        op->e.v = 0;
    } else {
        expect("operand");
    }
}

static void parse_operands(TCCState *s1, Operand* ops, int count){
    int i;
    for (i = 0; i < count; i++) {
        if ( i != 0 ) {
            if ( tok == ',')
                next();
            else
                expect("','");
        }
        parse_operand(s1, &ops[i]);
    }
}

/* parse `X, imm(Y)` to {X, Y, imm} operands */
static void parse_mem_access_operands(TCCState *s1, Operand* ops){
    static const Operand zimm = {.type = OP_IM12S};

    Operand op;

    parse_operand(s1, &ops[0]);
    if ( tok == ',')
        next();
    else
        expect("','");

    if ( tok == '(') {
        /* `X, (Y)` case*/
        next();
        parse_operand(s1, &ops[1]);
        if ( tok == ')') next(); else expect("')'");
        ops[2] = zimm;
    } else {
        parse_operand(s1, &ops[2]);
        if ( tok == '('){
            /* `X, imm(Y)` case*/
            next();
            parse_operand(s1, &ops[1]);
            if ( tok == ')') next(); else expect("')'");
        } else {
            /* `X, Y` case*/
            /* we parsed Y thinking it was imm, swap and default imm to zero */
            op = ops[2];
            ops[1] = ops[2];
            ops[2] = op;
            ops[2] = zimm;
        }
    }
}

/* This is special: First operand is optional */
static void asm_jal_opcode(TCCState *s1, int token){
    static const Operand ra = {.type = OP_REG, .reg = 1};
    static const Operand zero = {.type = OP_REG};
    Operand ops[2];

    if (token == TOK_ASM_j ){
        ops[0] = zero; // j offset
    } else if (asm_parse_regvar(tok) == -1) {
        ops[0] = ra;   // jal offset
    } else {
        // jal reg, offset
        parse_operand(s1, &ops[0]);
        if ( tok == ',') next(); else expect("','");
    }
    parse_jump_offset_operand(s1, &ops[1]);
    asm_emit_j(token, 0x6f, &ops[0], &ops[1]);
}

/* This is special: It can be a pseudointruction or a instruction */
static void asm_jalr_opcode(TCCState *s1, int token){
    static const Operand zimm = {.type = OP_IM12S};
    static const Operand ra = {.type = OP_REG, .reg = 1};
    Operand ops[3];
    Operand op;

    parse_operand(s1, &ops[0]);
    if ( tok == ',')
        next();
    else {
        /* no more operands, it's the pseudoinstruction:
         *  jalr rs
         * Expand to:
         *  jalr ra, 0(rs)
         */
        asm_emit_i(token, 0x67 | (0 << 12), &ra, &ops[0], &zimm);
        return;
    }

    if ( tok == '(') {
        /* `X, (Y)` case*/
        next();
        parse_operand(s1, &ops[1]);
        if ( tok == ')') next(); else expect("')'");
        ops[2] = zimm;
    } else {
        parse_operand(s1, &ops[2]);
        if ( tok == '('){
            /* `X, imm(Y)` case*/
            next();
            parse_operand(s1, &ops[1]);
            if ( tok == ')') next(); else expect("')'");
        } else {
            /* `X, Y` case*/
            /* we parsed Y thinking it was imm, swap and default imm to zero */
            op = ops[2];
            ops[1] = ops[2];
            ops[2] = op;
            ops[2] = zimm;
        }
    }
    /* jalr(RD, RS1, IMM); I-format */
    asm_emit_i(token, 0x67 | (0 << 12), &ops[0], &ops[1], &ops[2]);
}


static void asm_unary_opcode(TCCState *s1, int token)
{
    uint32_t opcode = (0x1C << 2) | 3 | (2 << 12);
    Operand op;
    static const Operand zero = {.type = OP_REG};
    static const Operand zimm = {.type = OP_IM12S};

    parse_operands(s1, &op, 1);
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

    case TOK_ASM_jr:
        /* jalr zero, 0(rs)*/
        asm_emit_i(token, 0x67 | (0 << 12), &zero, &op, &zimm);
        return;
    case TOK_ASM_call:
        /* auipc ra, 0 */
        greloca(cur_text_section, op.e.sym, ind, R_RISCV_CALL, 0);
        asm_emit_opcode(3 | (5 << 2) | ENCODE_RD(1));
        /* jalr zero, 0(ra) */
        asm_emit_opcode(0x67 | (0 << 12) | ENCODE_RS1(1));
        return;
    case TOK_ASM_tail:
        /* auipc x6, 0 */
        greloca(cur_text_section, op.e.sym, ind, R_RISCV_CALL, 0);
        asm_emit_opcode(3 | (5 << 2) | ENCODE_RD(6));
        /* jalr zero, 0(x6) */
        asm_emit_opcode(0x67 | (0 << 12) | ENCODE_RS1(6));
        return;

    /* C extension */
    case TOK_ASM_c_j:
        asm_emit_cj(token, 1 | (5 << 13), &op);
        return;
    case TOK_ASM_c_jal: /* RV32C-only */
        asm_emit_cj(token, 1 | (1 << 13), &op);
        return;
    case TOK_ASM_c_jalr:
        asm_emit_cr(token, 2 | (9 << 12), &op, &zero);
        return;
    case TOK_ASM_c_jr:
        asm_emit_cr(token, 2 | (8 << 12), &op, &zero);
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

static int parse_fence_operand(){
    int t = tok;
    if ( tok == TOK_ASM_or ){
        // we are in a fence instruction, parse as output read
        t = TOK_ASM_or_fence;
    }
    next();
    return t - (TOK_ASM_w_fence - 1);
}

static void asm_fence_opcode(TCCState *s1, int token){
    // `fence` is both an instruction and a pseudoinstruction:
    // `fence` expands to `fence iorw, iorw`
    int succ = 0xF, pred = 0xF;
    if (tok != TOK_LINEFEED && tok != ';' && tok != CH_EOF){
        pred = parse_fence_operand();
        if ( pred > 0xF || pred < 0) {
            tcc_error("'%s': Expected first operand that is a valid predecessor operand", get_tok_str(token, NULL));
        }
        if ( tok == ',') next(); else expect("','");
        succ = parse_fence_operand();
        if ( succ > 0xF || succ < 0) {
            tcc_error("'%s': Expected second operand that is a valid successor operand", get_tok_str(token, NULL));
        }
    }
    asm_emit_opcode((0x3 << 2) | 3 | (0 << 12) | succ<<20 | pred<<24);
}

static void asm_binary_opcode(TCCState* s1, int token)
{
    static const Operand zero = {.type = OP_REG, .reg = 0};
    Operand imm = {.type = OP_IM12S, .e = {.v = 0}};
    Operand ops[2];
    int32_t lo;
    uint32_t hi;

    parse_operands(s1, &ops[0], 2);
    switch (token) {
    case TOK_ASM_lui:
        asm_emit_u(token, (0xD << 2) | 3, &ops[0], &ops[1]);
        return;
    case TOK_ASM_auipc:
        asm_emit_u(token, (0x05 << 2) | 3, &ops[0], &ops[1]);
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
    case TOK_ASM_li:
        if(ops[1].type != OP_IM32 && ops[1].type != OP_IM12S){
            tcc_error("'%s': Expected first source operand that is an immediate value between 0 and 0xFFFFFFFFFFFFFFFF", get_tok_str(token, NULL));
        }
        lo = ops[1].e.v;
        hi = (int64_t)ops[1].e.v >> 32;
        if(lo < 0){
            hi += 1;
        }
        imm.e.v = ((hi + 0x800) & 0xfffff000) >> 12;
        /* lui rd, HI_20(HI_32(imm)) */
        asm_emit_u(token, (0xD << 2) | 3, &ops[0], &imm);
        /* addi rd, rd, LO_12(HI_32(imm)) */
        imm.e.v = (int32_t)hi<<20>>20;
        asm_emit_i(token, 3 | (4 << 2), &ops[0], &ops[0], &imm);
        /* slli rd, rd, 12 */
        imm.e.v = 12;
        asm_emit_i(token, (4 << 2) | 3 | (1 << 12), &ops[0], &ops[0], &imm);
        /* addi rd, rd, HI_12(LO_32(imm)) */
        imm.e.v = (lo + (1<<19)) >> 20;
        asm_emit_i(token, 3 | (4 << 2), &ops[0], &ops[0], &imm);
        /* slli rd, rd, 12 */
        imm.e.v = 12;
        asm_emit_i(token, (4 << 2) | 3 | (1 << 12), &ops[0], &ops[0], &imm);
        /* addi rd, rd, HI_12(LO_20(LO_32imm)) */
        lo = lo << 12 >> 12;
        imm.e.v = lo >> 8;
        asm_emit_i(token, 3 | (4 << 2), &ops[0], &ops[0], &imm);
        /* slli rd, rd,  8 */
        imm.e.v = 8;
        asm_emit_i(token, (4 << 2) | 3 | (1 << 12), &ops[0], &ops[0], &imm);
        /* addi rd, rd, LO_8(LO_20(LO_32imm)) */
        lo &= 0xff;
        imm.e.v = lo << 20 >> 20;
        asm_emit_i(token, 3 | (4 << 2), &ops[0], &ops[0], &imm);
        return;
    case TOK_ASM_mv:
        /* addi rd, rs, 0 */
        asm_emit_i(token, 3 | (4 << 2), &ops[0], &ops[1], &imm);
        return;
    case TOK_ASM_not:
        /* xori rd, rs, -1 */
        imm.e.v = -1;
        asm_emit_i(token, (0x4 << 2) | 3 | (4 << 12), &ops[0], &ops[1], &imm);
        return;
    case TOK_ASM_neg:
        /* sub rd, x0, rs */
        imm.e.v = 1;
        asm_emit_i(token, (0x4 << 2) | 3 | (4 << 12), &ops[0], &zero, &imm);
        return;
    case TOK_ASM_negw:
        /* sub rd, x0, rs */
        imm.e.v = 1;
        asm_emit_i(token, (0x4 << 2) | 3 | (4 << 12), &ops[0], &zero, &imm);
        return;
    case TOK_ASM_jump:
        /* auipc x5, 0 */
        asm_emit_opcode(3 | (5 << 2) | ENCODE_RD(5));
        greloca(cur_text_section, ops->e.sym, ind, R_RISCV_CALL, 0);
        /* jalr zero, 0(x5) */
        asm_emit_opcode(0x67 | (0 << 12) | ENCODE_RS1(5));
        return;
    case TOK_ASM_seqz:
        /* sltiu rd, rs, 1 */
        imm.e.v = 1;
        asm_emit_i(token, (0x4 << 2) | 3 | (3 << 12), &ops[0], &ops[1], &imm);
        return;
    case TOK_ASM_snez:
        /* sltu rd, zero, rs */
        imm.e.v = 1;
        asm_emit_r(token, (0xC << 2) | 3 | (3 << 12), &ops[0], &zero, &ops[1]);
        return;
    case TOK_ASM_sltz:
        /* slt rd, rs, zero */
        asm_emit_r(token, (0xC << 2) | 3 | (2 << 12), &ops[0], &ops[1], &zero);
        return;
    case TOK_ASM_sgtz:
        /* slt rd, zero, rs */
        asm_emit_r(token, (0xC << 2) | 3 | (2 << 12), &ops[0], &zero, &ops[1]);
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
    if ((int)imm > (1 << 20) -1 || (int)imm <= -1 * ((1 << 20) -1)) {
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

static void asm_mem_access_opcode(TCCState *s1, int token)
{

    Operand ops[3];
    parse_mem_access_operands(s1, &ops[0]);

    /* Pseudoinstruction: inst reg, label
     * expand to:
     *   auipc reg, 0
     *   inst reg, 0(reg)
     * And with the proper relocation to label
     */
    if (ops[1].type == OP_IM32 && ops[1].e.sym && ops[1].e.sym->type.t & VT_STATIC){
        ops[1] = ops[0];
        /* set the offset to zero */
        ops[2].type = OP_IM12S;
        ops[2].e.v  = 0;
        /* auipc reg, 0 */
        asm_emit_u(token, (0x05 << 2) | 3, &ops[0], &ops[2]);
    }

    switch (token) {
    // l{b|h|w|d}[u] rd, imm(rs1); I-format
    case TOK_ASM_lb:
         asm_emit_i(token, (0x0 << 2) | 3, &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lh:
         asm_emit_i(token, (0x0 << 2) | 3 | (1 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lw:
         asm_emit_i(token, (0x0 << 2) | 3 | (2 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_ld:
         asm_emit_i(token, (0x0 << 2) | 3 | (3 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lbu:
         asm_emit_i(token, (0x0 << 2) | 3 | (4 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lhu:
         asm_emit_i(token, (0x0 << 2) | 3 | (5 << 12), &ops[0], &ops[1], &ops[2]);
         return;
    case TOK_ASM_lwu:
         asm_emit_i(token, (0x0 << 2) | 3 | (6 << 12), &ops[0], &ops[1], &ops[2]);
         return;

    // s{b|h|w|d} rs2, imm(rs1); S-format (with rsX swapped)
    case TOK_ASM_sb:
         asm_emit_s(token, (0x8 << 2) | 3 | (0 << 12), &ops[1], &ops[0], &ops[2]);
         return;
    case TOK_ASM_sh:
         asm_emit_s(token, (0x8 << 2) | 3 | (1 << 12), &ops[1], &ops[0], &ops[2]);
         return;
    case TOK_ASM_sw:
         asm_emit_s(token, (0x8 << 2) | 3 | (2 << 12), &ops[1], &ops[0], &ops[2]);
         return;
    case TOK_ASM_sd:
         asm_emit_s(token, (0x8 << 2) | 3 | (3 << 12), &ops[1], &ops[0], &ops[2]);
         return;
    }
}

static void asm_branch_opcode(TCCState *s1, int token, int argc){
    Operand ops[3];
    Operand zero = {.type = OP_REG};
    parse_operands(s1, &ops[0], argc-1);
    if ( tok == ',') next(); else { expect(","); }
    parse_branch_offset_operand(s1, &ops[argc-1]);

    switch(token){
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
    /* related pseudoinstructions */
    case TOK_ASM_bgt:
        asm_emit_b(token, 0x63 | (4 << 12), ops + 1, ops, ops + 2);
        return;
    case TOK_ASM_ble:
        asm_emit_b(token, 0x63 | (5 << 12), ops + 1, ops, ops + 2);
        return;
    case TOK_ASM_bgtu:
        asm_emit_b(token, 0x63 | (6 << 12), ops + 1, ops, ops + 2);
        return;
    case TOK_ASM_bleu:
        asm_emit_b(token, 0x63 | (7 << 12), ops + 1, ops, ops + 2);
        return;
    /* shorter pseudoinstructions */
    case TOK_ASM_bnez:
        /* bne rs, zero, offset */
        asm_emit_b(token, 0x63 | (1 << 12), &ops[0], &zero, &ops[1]);
        return;
    case TOK_ASM_beqz:
        /* bne rs, zero, offset */
        asm_emit_b(token, 0x63 | (0 << 12), &ops[0], &zero, &ops[1]);
        return;
    case TOK_ASM_blez:
        /* bge rs, zero, offset */
        asm_emit_b(token, 0x63 | (5 << 12), &ops[0], &zero, &ops[1]);
        return;
    case TOK_ASM_bgez:
        /* bge zero, rs, offset */
        asm_emit_b(token, 0x63 | (5 << 12), &zero, &ops[0], &ops[1]);
        return;
    case TOK_ASM_bltz:
        /* blt rs, zero, offset */
        asm_emit_b(token, 0x63 | (4 << 12), &ops[0], &zero, &ops[1]);
        return;
    case TOK_ASM_bgtz:
        /* blt zero, rs, offset */
        asm_emit_b(token, 0x63 | (4 << 12), &zero, &ops[0], &ops[1]);
        return;
    }
}

static void asm_ternary_opcode(TCCState *s1, int token)
{
    Operand ops[3];
    parse_operands(s1, &ops[0], 3);

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

static void asm_atomic_opcode(TCCState *s1, int token)
{
    static const Operand zero = {.type = OP_REG};
    Operand ops[3];

    parse_operand(s1, &ops[0]);
    if ( tok == ',') next(); else expect("','");

    if ( token <= TOK_ASM_lr_d_aqrl && token >= TOK_ASM_lr_w ) {
        ops[1] = zero;
    } else {
        parse_operand(s1, &ops[1]);
        if ( tok == ',') next(); else expect("','");
    }

    if ( tok == '(') next(); else expect("'('");
    parse_operand(s1, &ops[2]);
    if ( tok == ')') next(); else expect("')'");

    switch(token){
        case TOK_ASM_lr_w:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 0, 0);
            break;
        case TOK_ASM_lr_w_aq:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 1, 0);
            break;
        case TOK_ASM_lr_w_rl:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 0, 1);
            break;
        case TOK_ASM_lr_w_aqrl:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 1, 1);
            break;

        case TOK_ASM_lr_d:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 0, 0);
            break;
        case TOK_ASM_lr_d_aq:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 1, 0);
            break;
        case TOK_ASM_lr_d_rl:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 0, 1);
            break;
        case TOK_ASM_lr_d_aqrl:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x2<<27, &ops[0], &ops[1], &ops[2], 1, 1);
            break;

        case TOK_ASM_sc_w:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 0, 0);
            break;
        case TOK_ASM_sc_w_aq:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 1, 0);
            break;
        case TOK_ASM_sc_w_rl:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 0, 1);
            break;
        case TOK_ASM_sc_w_aqrl:
            asm_emit_a(token, 0x2F | 0x2<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 1, 1);
            break;

        case TOK_ASM_sc_d:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 0, 0);
            break;
        case TOK_ASM_sc_d_aq:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 1, 0);
            break;
        case TOK_ASM_sc_d_rl:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 0, 1);
            break;
        case TOK_ASM_sc_d_aqrl:
            asm_emit_a(token, 0x2F | 0x3<<12 | 0x3<<27, &ops[0], &ops[1], &ops[2], 1, 1);
            break;
    }
}

/* caller: Add funct3 and func5 to opcode */
static void asm_emit_a(int token, uint32_t opcode, const Operand *rd1, const Operand *rs2, const Operand *rs1, int aq, int rl)
{
    if (rd1->type != OP_REG)
        tcc_error("'%s': Expected first destination operand that is a register", get_tok_str(token, NULL));
    if (rs2->type != OP_REG)
        tcc_error("'%s': Expected second source operand that is a register", get_tok_str(token, NULL));
    if (rs1->type != OP_REG)
        tcc_error("'%s': Expected third source operand that is a register", get_tok_str(token, NULL));
        /* A-type instruction:
	        31...27 funct5
	        26      aq
	        25      rl
	        24...20 rs2
	        19...15 rs1
	        14...11 funct3
	        11...7  rd
	        6...0 opcode
        opcode always fixed pos. */
    gen_le32(opcode | ENCODE_RS1(rs1->reg) | ENCODE_RS2(rs2->reg) | ENCODE_RD(rd1->reg) | aq << 26 | rl << 25);
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
    case TOK_ASM_fence_i:
    case TOK_ASM_hrts:
    case TOK_ASM_mrth:
    case TOK_ASM_mrts:
    case TOK_ASM_wfi:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_fence:
        asm_fence_opcode(s1, token);
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
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_lb:
    case TOK_ASM_lh:
    case TOK_ASM_lw:
    case TOK_ASM_ld:
    case TOK_ASM_lbu:
    case TOK_ASM_lhu:
    case TOK_ASM_lwu:
    case TOK_ASM_sb:
    case TOK_ASM_sh:
    case TOK_ASM_sw:
    case TOK_ASM_sd:
        asm_mem_access_opcode(s1, token);
        break;

    case TOK_ASM_jalr:
        asm_jalr_opcode(s1, token); /* it can be a pseudo instruction too*/
        break;
    case TOK_ASM_j:
        asm_jal_opcode(s1, token); /* jal zero, offset*/
        return;
    case TOK_ASM_jal:
        asm_jal_opcode(s1, token); /* it can be a pseudo instruction too*/
        break;

    case TOK_ASM_add:
    case TOK_ASM_addi:
    case TOK_ASM_addiw:
    case TOK_ASM_addw:
    case TOK_ASM_and:
    case TOK_ASM_andi:
    case TOK_ASM_or:
    case TOK_ASM_ori:
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

    /* Branches */
    case TOK_ASM_beq:
    case TOK_ASM_bge:
    case TOK_ASM_bgeu:
    case TOK_ASM_blt:
    case TOK_ASM_bltu:
    case TOK_ASM_bne:
        asm_branch_opcode(s1, token, 3);
        break;

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
    case TOK_ASM_ret:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_jr:
    case TOK_ASM_call:
    case TOK_ASM_tail:
        asm_unary_opcode(s1, token);
        return;

    case TOK_ASM_la:
    case TOK_ASM_lla:
    case TOK_ASM_li:
    case TOK_ASM_jump:
    case TOK_ASM_seqz:
    case TOK_ASM_snez:
    case TOK_ASM_sltz:
    case TOK_ASM_sgtz:
    case TOK_ASM_mv:
    case TOK_ASM_not:
    case TOK_ASM_neg:
    case TOK_ASM_negw:
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_bnez:
    case TOK_ASM_beqz:
    case TOK_ASM_blez:
    case TOK_ASM_bgez:
    case TOK_ASM_bltz:
    case TOK_ASM_bgtz:
        asm_branch_opcode(s1, token, 2);
        return;

    case TOK_ASM_bgt:
    case TOK_ASM_bgtu:
    case TOK_ASM_ble:
    case TOK_ASM_bleu:
        asm_branch_opcode(s1, token, 3);
        return;

    /* Atomic operations */
    case TOK_ASM_lr_w:
    case TOK_ASM_lr_w_aq:
    case TOK_ASM_lr_w_rl:
    case TOK_ASM_lr_w_aqrl:
    case TOK_ASM_lr_d:
    case TOK_ASM_lr_d_aq:
    case TOK_ASM_lr_d_rl:
    case TOK_ASM_lr_d_aqrl:
    case TOK_ASM_sc_w:
    case TOK_ASM_sc_w_aq:
    case TOK_ASM_sc_w_rl:
    case TOK_ASM_sc_w_aqrl:
    case TOK_ASM_sc_d:
    case TOK_ASM_sc_d_aq:
    case TOK_ASM_sc_d_rl:
    case TOK_ASM_sc_d_aqrl:
        asm_atomic_opcode(s1, token);
        break;

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
    int r, reg, val;
    char buf[64];

    r = sv->r;
    if ((r & VT_VALMASK) == VT_CONST) {
        if (!(r & VT_LVAL) && modifier != 'c' && modifier != 'n' &&
            modifier != 'P') {
            //cstr_ccat(add_str, '#');
        }
        if (r & VT_SYM) {
            const char *name = get_tok_str(sv->sym->v, NULL);
            if (sv->sym->v >= SYM_FIRST_ANOM) {
                /* In case of anonymous symbols ("L.42", used
                   for static data labels) we can't find them
                   in the C symbol table when later looking up
                   this name.  So enter them now into the asm label
                   list when we still know the symbol.  */
                get_asm_sym(tok_alloc(name, strlen(name))->tok, sv->sym);
            }
            if (tcc_state->leading_underscore)
                cstr_ccat(add_str, '_');
            cstr_cat(add_str, name, -1);
            if ((uint32_t) sv->c.i == 0)
                goto no_offset;
            cstr_ccat(add_str, '+');
        }
        val = sv->c.i;
        if (modifier == 'n')
            val = -val;
        if (modifier == 'z' && sv->c.i == 0) {
            cstr_cat(add_str, "zero", -1);
        } else {
            snprintf(buf, sizeof(buf), "%d", (int) sv->c.i);
            cstr_cat(add_str, buf, -1);
        }
      no_offset:;
    } else if ((r & VT_VALMASK) == VT_LOCAL) {
        snprintf(buf, sizeof(buf), "%d", (int) sv->c.i);
        cstr_cat(add_str, buf, -1);
    } else if (r & VT_LVAL) {
        reg = r & VT_VALMASK;
        if (reg >= VT_CONST)
            tcc_internal_error("");
        if ((sv->type.t & VT_BTYPE) == VT_FLOAT ||
            (sv->type.t & VT_BTYPE) == VT_DOUBLE) {
            /* floating point register */
            reg = TOK_ASM_f0 + reg;
        } else {
            /* general purpose register */
            reg = TOK_ASM_x0 + reg;
        }
        snprintf(buf, sizeof(buf), "%s", get_tok_str(reg, NULL));
        cstr_cat(add_str, buf, -1);
    } else {
        /* register case */
        reg = r & VT_VALMASK;
        if (reg >= VT_CONST)
            tcc_internal_error("");
        if ((sv->type.t & VT_BTYPE) == VT_FLOAT ||
            (sv->type.t & VT_BTYPE) == VT_DOUBLE) {
            /* floating point register */
            reg = TOK_ASM_f0 + reg;
        } else {
            /* general purpose register */
            reg = TOK_ASM_x0 + reg;
        }
        snprintf(buf, sizeof(buf), "%s", get_tok_str(reg, NULL));
        cstr_cat(add_str, buf, -1);
    }
}

/* TCC does not use RISC-V register numbers internally, it uses 0-8 for
 * integers and 8-16 for floats instead */
static int tcc_ireg(int r){
    return REG_VALUE(r) - 10;
}
static int tcc_freg(int r){
    return REG_VALUE(r) - 10 + 8;
}

/* generate prolog and epilog code for asm statement */
ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands,
                         int nb_outputs, int is_output,
                         uint8_t *clobber_regs,
                         int out_reg)
{
    uint8_t regs_allocated[NB_ASM_REGS];
    ASMOperand *op;
    int i, reg;

    static const uint8_t reg_saved[] = {
        // General purpose regs
        8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        // Float regs
        40, 41, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
    };

    /* mark all used registers */
    memcpy(regs_allocated, clobber_regs, sizeof(regs_allocated));
    for(i = 0; i < nb_operands; i++) {
        op = &operands[i];
        if (op->reg >= 0) {
            regs_allocated[op->reg] = 1;
        }
    }

    if(!is_output) {
        /* generate reg save code */
        for(i = 0; i < sizeof(reg_saved)/sizeof(reg_saved[0]); i++) {
            reg = reg_saved[i];
            if (regs_allocated[reg]) {
                /* push */
                /* addi sp, sp, -offset */
                gen_le32((4 << 2) | 3 |
                        ENCODE_RD(2) | ENCODE_RS1(2) | -8 << 20);
                if (REG_IS_FLOAT(reg)){
                    /* fsd reg, offset(sp) */
                    gen_le32( 0x27 | (3 << 12) |
                            ENCODE_RS2(reg) | ENCODE_RS1(2) );
                } else {
                    /* sd reg, offset(sp) */
                    gen_le32((0x8 << 2) | 3 | (3 << 12) |
                            ENCODE_RS2(reg) | ENCODE_RS1(2) );
                }
            }
        }

        /* generate load code */
        for(i = 0; i < nb_operands; i++) {
            op = &operands[i];
            if (op->reg >= 0) {
                if ((op->vt->r & VT_VALMASK) == VT_LLOCAL &&
                    op->is_memory) {
                    /* memory reference case (for both input and
                       output cases) */
                    SValue sv;
                    sv = *op->vt;
                    sv.r = (sv.r & ~VT_VALMASK) | VT_LOCAL | VT_LVAL;
                    sv.type.t = VT_PTR;
                    load(tcc_ireg(op->reg), &sv);
                } else if (i >= nb_outputs || op->is_rw) {
                    /* load value in register */
                    if ((op->vt->type.t & VT_BTYPE) == VT_FLOAT ||
                        (op->vt->type.t & VT_BTYPE) == VT_DOUBLE) {
                        load(tcc_freg(op->reg), op->vt);
                    } else {
                        load(tcc_ireg(op->reg), op->vt);
                    }
                    if (op->is_llong) {
                        tcc_error("long long not implemented");
                    }
                }
            }
        }
    } else {
        /* generate save code */
        for(i = 0 ; i < nb_outputs; i++) {
            op = &operands[i];
            if (op->reg >= 0) {
                if ((op->vt->r & VT_VALMASK) == VT_LLOCAL) {
                    if (!op->is_memory) {
                        SValue sv;
                        sv = *op->vt;
                        sv.r = (sv.r & ~VT_VALMASK) | VT_LOCAL;
                        sv.type.t = VT_PTR;
                        load(tcc_ireg(out_reg), &sv);

                        sv = *op->vt;
                        sv.r = (sv.r & ~VT_VALMASK) | out_reg;
                        store(tcc_ireg(op->reg), &sv);
                    }
                } else {
                    if ((op->vt->type.t & VT_BTYPE) == VT_FLOAT ||
                        (op->vt->type.t & VT_BTYPE) == VT_DOUBLE) {
                        store(tcc_freg(op->reg), op->vt);
                    } else {
                        store(tcc_ireg(op->reg), op->vt);
                    }
                    if (op->is_llong) {
                        tcc_error("long long not implemented");
                    }
                }
            }
        }
        /* generate reg restore code for floating point registers */
        for(i = sizeof(reg_saved)/sizeof(reg_saved[0]) - 1; i >= 0; i--) {
            reg = reg_saved[i];
            if (regs_allocated[reg]) {
                /* pop */
                if (REG_IS_FLOAT(reg)){
                    /* fld reg, offset(sp) */
                    gen_le32(7 | (3 << 12) |
                            ENCODE_RD(reg) | ENCODE_RS1(2) | 0);
                } else {
                    /* ld reg, offset(sp) */
                    gen_le32(3 | (3 << 12) |
                            ENCODE_RD(reg) | ENCODE_RS1(2) | 0);
                }
                /* addi sp, sp, offset */
                gen_le32((4 << 2) | 3 |
                        ENCODE_RD(2) | ENCODE_RS1(2) | 8 << 20);
            }
        }
    }
}

/* return the constraint priority (we allocate first the lowest
   numbered constraints) */
static inline int constraint_priority(const char *str)
{
    // TODO: How is this chosen??
    int priority, c, pr;

    /* we take the lowest priority */
    priority = 0;
    for(;;) {
        c = *str;
        if (c == '\0')
            break;
        str++;
        switch(c) {
        case 'A': // address that is held in a general-purpose register.
        case 'S': // constraint that matches an absolute symbolic address.
        case 'f': // register [float]
        case 'r': // register [general]
        case 'p': // valid memory address for load,store [general]
            pr = 3;
            break;
        case 'I': // 12 bit signed immedate
        case 'i': // immediate integer operand, including symbolic constants [general]
        case 'm': // memory operand [general]
        case 'g': // general-purpose-register, memory, immediate integer [general]
            pr = 4;
            break;
        case 'v':
            tcc_error("unimp: vector constraints '%d'", c);
            pr = 0;
            break;
        default:
            tcc_error("unknown constraint '%d'", c);
            pr = 0;
        }
        if (pr > priority)
            priority = pr;
    }
    return priority;
}

static const char *skip_constraint_modifiers(const char *p)
{
    /* Constraint modifier:
        =   Operand is written to by this instruction
        +   Operand is both read and written to by this instruction
        %   Instruction is commutative for this operand and the following operand.

       Per-alternative constraint modifier:
        &   Operand is clobbered before the instruction is done using the input operands
    */
    while (*p == '=' || *p == '&' || *p == '+' || *p == '%')
        p++;
    return p;
}

#define REG_OUT_MASK 0x01
#define REG_IN_MASK  0x02

#define is_reg_allocated(reg) (regs_allocated[reg] & reg_mask)

ST_FUNC void asm_compute_constraints(ASMOperand *operands,
                                    int nb_operands, int nb_outputs,
                                    const uint8_t *clobber_regs,
                                    int *pout_reg)
{
    /* TODO: Simple constraints
        whitespace  ignored
        o  memory operand that is offsetable
        V  memory but not offsetable
        <  memory operand with autodecrement addressing is allowed.  Restrictions apply.
        >  memory operand with autoincrement addressing is allowed.  Restrictions apply.
        n  immediate integer operand with a known numeric value
        E  immediate floating operand (const_double) is allowed, but only if target=host
        F  immediate floating operand (const_double or const_vector) is allowed
        s  immediate integer operand whose value is not an explicit integer
        X  any operand whatsoever
        0...9 (postfix); (can also be more than 1 digit number);  an operand that matches the specified operand number is allowed
    */

    /* TODO: RISCV constraints
        J   The integer 0.
        K   A 5-bit unsigned immediate for CSR access instructions.
        A   An address that is held in a general-purpose register.
        S   A constraint that matches an absolute symbolic address.
        vr  A vector register (if available)..
        vd  A vector register, excluding v0 (if available).
        vm  A vector register, only v0 (if available).
    */
    ASMOperand *op;
    int sorted_op[MAX_ASM_OPERANDS];
    int i, j, k, p1, p2, tmp, reg, c, reg_mask;
    const char *str;
    uint8_t regs_allocated[NB_ASM_REGS];

    /* init fields */
    for (i = 0; i < nb_operands; i++) {
        op = &operands[i];
        op->input_index = -1;
        op->ref_index = -1;
        op->reg = -1;
        op->is_memory = 0;
        op->is_rw = 0;
    }
    /* compute constraint priority and evaluate references to output
       constraints if input constraints */
    for (i = 0; i < nb_operands; i++) {
        op = &operands[i];
        str = op->constraint;
        str = skip_constraint_modifiers(str);
        if (isnum(*str) || *str == '[') {
            /* this is a reference to another constraint */
            k = find_constraint(operands, nb_operands, str, NULL);
            if ((unsigned) k >= i || i < nb_outputs)
                tcc_error("invalid reference in constraint %d ('%s')",
                          i, str);
            op->ref_index = k;
            if (operands[k].input_index >= 0)
                tcc_error("cannot reference twice the same operand");
            operands[k].input_index = i;
            op->priority = 5;
        } else if ((op->vt->r & VT_VALMASK) == VT_LOCAL
                   && op->vt->sym
                   && (reg = op->vt->sym->r & VT_VALMASK) < VT_CONST) {
            op->priority = 1;
            op->reg = reg;
        } else {
            op->priority = constraint_priority(str);
        }
    }

    /* sort operands according to their priority */
    for (i = 0; i < nb_operands; i++)
        sorted_op[i] = i;
    for (i = 0; i < nb_operands - 1; i++) {
        for (j = i + 1; j < nb_operands; j++) {
            p1 = operands[sorted_op[i]].priority;
            p2 = operands[sorted_op[j]].priority;
            if (p2 < p1) {
                tmp = sorted_op[i];
                sorted_op[i] = sorted_op[j];
                sorted_op[j] = tmp;
            }
        }
    }

    for (i = 0; i < NB_ASM_REGS; i++) {
        if (clobber_regs[i])
            regs_allocated[i] = REG_IN_MASK | REG_OUT_MASK;
        else
            regs_allocated[i] = 0;
    }

    /* allocate registers and generate corresponding asm moves */
    for (i = 0; i < nb_operands; i++) {
        j = sorted_op[i];
        op = &operands[j];
        str = op->constraint;
        /* no need to allocate references */
        if (op->ref_index >= 0)
            continue;
        /* select if register is used for output, input or both */
        if (op->input_index >= 0) {
            reg_mask = REG_IN_MASK | REG_OUT_MASK;
        } else if (j < nb_outputs) {
            reg_mask = REG_OUT_MASK;
        } else {
            reg_mask = REG_IN_MASK;
        }
        if (op->reg >= 0) {
            if (is_reg_allocated(op->reg))
                tcc_error
                    ("asm regvar requests register that's taken already");
            reg = op->reg;
        }
      try_next:
        c = *str++;
        switch (c) {
        case '=': // Operand is written-to
            goto try_next;
        case '+': // Operand is both READ and written-to
            op->is_rw = 1;
            /* FALL THRU */
        case '&': // Operand is clobbered before the instruction is done using the input operands
            if (j >= nb_outputs)
                tcc_error("'%c' modifier can only be applied to outputs", c);
            reg_mask = REG_IN_MASK | REG_OUT_MASK;
            goto try_next;
        case 'r': // general-purpose register
        case 'p': // loadable/storable address
            /* any general register */
            /* From a0 to a7 */
            if ((reg = op->reg) >= 0)
                goto reg_found;
            else for (reg = 10; reg <= 18; reg++) {
                if (!is_reg_allocated(reg))
                    goto reg_found;
            }
            goto try_next;
          reg_found:
            /* now we can reload in the register */
            op->is_llong = 0;
            op->reg = reg;
            regs_allocated[reg] |= reg_mask;
            break;
        case 'f': // floating pont register
            /* floating point register */
            /* From fa0 to fa7 */
            if ((reg = op->reg) >= 0)
                goto reg_found;
            else for (reg = 42; reg <= 50; reg++) {
                if (!is_reg_allocated(reg))
                    goto reg_found;
            }
            goto try_next;
        case 'I': // I-Type 12 bit signed immediate
        case 'i': // immediate integer operand, including symbolic constants
            if (!((op->vt->r & (VT_VALMASK | VT_LVAL)) == VT_CONST))
                goto try_next;
            break;
        case 'm': // memory operand
        case 'g': // any register
            /* nothing special to do because the operand is already in
               memory, except if the pointer itself is stored in a
               memory variable (VT_LLOCAL case) */
            /* XXX: fix constant case */
            /* if it is a reference to a memory zone, it must lie
               in a register, so we reserve the register in the
               input registers and a load will be generated
               later */
            if (j < nb_outputs || c == 'm') {
                if ((op->vt->r & VT_VALMASK) == VT_LLOCAL) {
                    /* any general register: from a0 to a7 */
                    for (reg = 10; reg <= 18; reg++) {
                        if (!(regs_allocated[reg] & REG_IN_MASK))
                            goto reg_found1;
                    }
                    goto try_next;
                  reg_found1:
                    /* now we can reload in the register */
                    regs_allocated[reg] |= REG_IN_MASK;
                    op->reg = reg;
                    op->is_memory = 1;
                }
            }
            break;
        default:
            tcc_error("asm constraint %d ('%s') could not be satisfied",
                      j, op->constraint);
            break;
        }
        /* if a reference is present for that operand, we assign it too */
        if (op->input_index >= 0) {
            operands[op->input_index].reg = op->reg;
            operands[op->input_index].is_llong = op->is_llong;
        }
    }

    /* compute out_reg. It is used to store outputs registers to memory
       locations references by pointers (VT_LLOCAL case) */
    *pout_reg = -1;
    for (i = 0; i < nb_operands; i++) {
        op = &operands[i];
        if (op->reg >= 0 &&
            (op->vt->r & VT_VALMASK) == VT_LLOCAL && !op->is_memory) {
            if (REG_IS_FLOAT(op->reg)){
                /* From fa0 to fa7 */
                for (reg = 42; reg <= 50; reg++) {
                    if (!(regs_allocated[reg] & REG_OUT_MASK))
                        goto reg_found2;
                }
            } else {
                /* From a0 to a7 */
                for (reg = 10; reg <= 18; reg++) {
                    if (!(regs_allocated[reg] & REG_OUT_MASK))
                        goto reg_found2;
                }
            }
            tcc_error("could not find free output register for reloading");
          reg_found2:
            *pout_reg = reg;
            break;
        }
    }

    /* print sorted constraints */
#ifdef ASM_DEBUG
    for (i = 0; i < nb_operands; i++) {
        j = sorted_op[i];
        op = &operands[j];
        printf("%%%d [%s]: \"%s\" r=0x%04x reg=%d\n",
               j,
               op->id ? get_tok_str(op->id, NULL) : "",
               op->constraint, op->vt->r, op->reg);
    }
    if (*pout_reg >= 0)
        printf("out_reg=%d\n", *pout_reg);
#endif
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
        return t - TOK_ASM_f0 + 32; // Use higher 32 for floating point

    /* ABI mnemonic */
    if (t < TOK_ASM_ft0)
        return t - TOK_ASM_zero;

    return t - TOK_ASM_ft0 + 32; // Use higher 32 for floating point
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
