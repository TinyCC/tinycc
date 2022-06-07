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
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 4095", get_tok_str(token, NULL));
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

static void asm_shift_opcode(TCCState *s1, int token)
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
    default:
        expect("shift instruction");
    }
}

static void asm_data_processing_opcode(TCCState* s1, int token)
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
    default:
         expect("known data processing instruction");
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
        tcc_error("'%s': Expected third operand that is an immediate value between 0 and 0xfff", get_tok_str(token, NULL));
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

static void asm_data_transfer_opcode(TCCState* s1, int token)
{
    Operand ops[3];
    parse_operand(s1, &ops[0]);
    if (ops[0].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);
    if (ops[1].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[2]);

    switch (token) {
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

    default:
         expect("known data transfer instruction");
    }
}

static void asm_branch_opcode(TCCState* s1, int token)
{
    // Branch (RS1,RS2,IMM); SB-format
    uint32_t opcode = (0x18 << 2) | 3;
    uint32_t offset = 0;
    Operand ops[3];
    parse_operand(s1, &ops[0]);
    if (ops[0].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);
    if (ops[1].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[2]);

    if (ops[2].type != OP_IM12S) {
        tcc_error("'%s': Expected third operand that is an immediate value between 0 and 0xfff", get_tok_str(token, NULL));
        return;
    }
    offset = ops[2].e.v;
    if (offset & 1) {
        tcc_error("'%s': Expected third operand that is an even immediate value", get_tok_str(token, NULL));
        return;
    }

    switch (token) {
    case TOK_ASM_beq:
        opcode |= 0 << 12;
        break;
    case TOK_ASM_bne:
        opcode |= 1 << 12;
        break;
    case TOK_ASM_blt:
        opcode |= 4 << 12;
        break;
    case TOK_ASM_bge:
        opcode |= 5 << 12;
        break;
    case TOK_ASM_bltu:
        opcode |= 6 << 12;
        break;
    case TOK_ASM_bgeu:
        opcode |= 7 << 12;
        break;
    default:
        expect("known branch instruction");
    }
    asm_emit_opcode(opcode | ENCODE_RS1(ops[0].reg) | ENCODE_RS2(ops[1].reg) | (((offset >> 1) & 0xF) << 8) | (((offset >> 5) & 0x1f) << 25) | (((offset >> 11) & 1) << 7) | (((offset >> 12) & 1) << 31));
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

    case TOK_ASM_lui:
    case TOK_ASM_auipc:
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_sll:
    case TOK_ASM_slli:
    case TOK_ASM_srl:
    case TOK_ASM_srli:
    case TOK_ASM_sra:
    case TOK_ASM_srai:
    case TOK_ASM_sllw:
    case TOK_ASM_slld:
    case TOK_ASM_slliw:
    case TOK_ASM_sllid:
    case TOK_ASM_srlw:
    case TOK_ASM_srld:
    case TOK_ASM_srliw:
    case TOK_ASM_srlid:
    case TOK_ASM_sraw:
    case TOK_ASM_srad:
    case TOK_ASM_sraiw:
    case TOK_ASM_sraid:
        asm_shift_opcode(s1, token);
        return;

    case TOK_ASM_add:
    case TOK_ASM_addi:
    case TOK_ASM_sub:
    case TOK_ASM_addw:
    case TOK_ASM_addd:
    case TOK_ASM_addiw:
    case TOK_ASM_addid:
    case TOK_ASM_subw:
    case TOK_ASM_subd:
    case TOK_ASM_xor:
    case TOK_ASM_xori:
    case TOK_ASM_or:
    case TOK_ASM_ori:
    case TOK_ASM_and:
    case TOK_ASM_andi:
    case TOK_ASM_slt:
    case TOK_ASM_slti:
    case TOK_ASM_sltu:
    case TOK_ASM_sltiu:
        asm_data_processing_opcode(s1, token);
	return;

    case TOK_ASM_lb:
    case TOK_ASM_lh:
    case TOK_ASM_lw:
    case TOK_ASM_lbu:
    case TOK_ASM_lhu:
    case TOK_ASM_ld:
    case TOK_ASM_lwu:
    case TOK_ASM_sb:
    case TOK_ASM_sh:
    case TOK_ASM_sw:
    case TOK_ASM_sd:
        asm_data_transfer_opcode(s1, token);
        return;

    case TOK_ASM_beq:
    case TOK_ASM_bne:
    case TOK_ASM_blt:
    case TOK_ASM_bge:
    case TOK_ASM_bltu:
    case TOK_ASM_bgeu:
        asm_branch_opcode(s1, token);
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
    if (t >= TOK_ASM_x0 && t <= TOK_ASM_pc) { /* register name */
	if (t >= TOK_ASM_zero && t <= TOK_ASM_t6)
	    return t - TOK_ASM_zero;
        switch (t) {
            case TOK_ASM_s0:
                return 8;
            case TOK_ASM_pc:
	        tcc_error("PC register not implemented.");
            default:
                return t - TOK_ASM_x0;
        }
    } else
        return -1;
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
