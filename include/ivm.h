#ifndef __IVM_H__
#define __IVM_H__

#include <inttypes.h>

#define OP(x)  ((x) & (0xe000))
#define ARG(x) ((x) & (0x1fff))
#define ARG_LIT(x) ((x) & 0x7fff)

#define ALU_OP(x)   (x & (0xf << 8))
#define ALU_DS(x)		(x & (0x3 << 0))
#define ALU_RS(x)		(x & (0x3 << 2))

/* Operation types */
enum {
	OP_JMP  = 0x0000,
	OP_JZ   = 0x2000,
	OP_CALL = 0x4000,
	OP_ALU  = 0x6000,
	OP_LIT  = 0x8000
};
// ALU flags
enum {
	ALU_F_N_aT = 1<<5,
	ALU_F_T_R  = 1<<6,
	ALU_F_T_N  = 1<<7,
	ALU_F_R_PC = 1<<12
};

#define ALU_DS_PUSH (1)
#define ALU_DS_POP (3)
#define ALU_RS_PUSH (1<<2)
#define ALU_RS_POP (3<<2)
#define ALU_RS_PP (2<<2)


/* ALU operation types */
enum {
	ALU_OP_T      = 0,
	ALU_OP_N      = 1,
	ALU_OP_PLUS   = 2,
	ALU_OP_AND    = 3,
	ALU_OP_OR     = 4,
	ALU_OP_XOR    = 5,
	ALU_OP_NEG    = 6,
	ALU_OP_EQ     = 7,
	ALU_OP_LESS   = 8,
	ALU_OP_RSHIFT = 9,
	ALU_OP_DEC    = 10,
	ALU_OP_R      = 11,
	ALU_OP_MEM    = 12,
	ALU_OP_LSHIFT = 13,
	ALU_OP_DEPTH  = 14,
	ALU_OP_ULESS  = 15
};
enum {
	RC_DS_UNDER_FLOW = -1,
	RC_DS_OVER_FLOW =-2,
	RC_RS_OVER_FLOW = -3,
	RC_RS_UNDER_FLOW = -4,
	RC_PC_OVER_FLOW = -5,
	RC_OP_UNKNOWN = -6,
};

#define STACT_SIZE (32)
typedef struct {
 uint16_t ds[STACT_SIZE], rs[STACT_SIZE]; /* data stack and return stack */
 char dp, rp; /* top of data stack, next on data stack, top of ret stack */
 uint16_t pc;
 uint8_t* DRAM;
 uint16_t n,t,r,alu;
} ivm_t;

//static ivm_t ivm;
static void ivm_mem_put(uint16_t addr, uint16_t value, ivm_t*);
static uint16_t ivm_mem_get(uint16_t addr, ivm_t*);

static inline void ivm_reset(ivm_t* ivm) {
	ivm->pc = 0;
	ivm->dp = ivm->rp = -1;
}

static inline int ivm_step(uint16_t word, ivm_t* ivm) {
	if (word & 0x8000) {
		ivm->ds[++ivm->dp] = ARG_LIT(word);
		ivm->pc++;
		if (ivm->dp >= STACT_SIZE) return RC_DS_OVER_FLOW;
		if (ivm->pc > (1<<13)) return RC_PC_OVER_FLOW;
		return 0;
	}

	switch (OP(word)) {
		case OP_JZ:
			if (ivm->ds[ivm->dp--] == 0) {
				ivm->pc = ARG(word);
			} else {
				ivm->pc++;
			}
			break;
		case OP_JMP:
			ivm->pc = ARG(word);
			break;
		case OP_CALL:
			ivm->rs[++ivm->rp] = ivm->pc+1;
			ivm->pc = ARG(word);
			if(ivm->rp >= STACT_SIZE) return RC_RS_OVER_FLOW;
			break;
		case OP_ALU: {
// forums.parallella.org/viewtopic.php?f=31&t=173
			// step 1,
			ivm->t = ivm->ds[ivm->dp];
			if(ivm->dp > 0) ivm->n = ivm->ds[ivm->dp-1];
			ivm->r = ivm->rs[ivm->rp];
			uint16_t t,n,r, res;
			t = ivm->t;
			n = ivm->n;
			r = ivm->r;

			switch (ALU_OP(word) >> 8) {
				case ALU_OP_T: res = t; break;
				case ALU_OP_N: {
					res = n; 
					break;
				}
				case ALU_OP_PLUS: {
					res = t + n; 
					break;
				}
				case ALU_OP_AND: { 
					res = t & n; 
					break;
				}
				case ALU_OP_OR: { 
					res = t | n; 
					break;
				}
				case ALU_OP_XOR: {
					res = t ^ n; 
					break;
				}
				case ALU_OP_NEG: res = ~t; break; // -1, 0
				case ALU_OP_EQ: { 
					res = (t == n)?-1:0; 
					break; // -1, 0
				}
				case ALU_OP_LESS: {
					res = (n < t)?-1:0; 
					break; // -1, 0
				}
				case ALU_OP_RSHIFT: { 
					res = (n >> t); 
					break;
				}
				case ALU_OP_DEC: res = t-1; break;
				case ALU_OP_R: res = r; break;
				case ALU_OP_MEM: res = ivm_mem_get(t,ivm); break;
				case ALU_OP_LSHIFT: {
					res = (n << t); 
					break;
				}
				case ALU_OP_DEPTH: res = ivm->dp+1; break;
				case ALU_OP_ULESS: {
					res = (n < t)?-1:0; 
					break; // -1, 0
				}
				default:
				return RC_OP_UNKNOWN;
			}
			ivm->alu = res;
			if (word & ALU_F_N_aT) {
				ivm_mem_put(t, n, ivm);
			}
			// step 2
			switch (ALU_DS(word)) {
				case (ALU_DS_PUSH): ivm->dp++; break;
				case (ALU_DS_POP): ivm->dp--; break;
			}
			// step 3
			if (word & (ALU_F_T_N)) {
				ivm->ds[ivm->dp-1] = t;
			}
			if (word & (ALU_F_R_PC)) {
				/* TODO: set 0x7fff return address on reset? */
				if (ivm->rp < -1) { /* exit condition */
					return RC_RS_UNDER_FLOW;
				} else {
					ivm->pc = ivm->rs[ivm->rp];
				}
			} else {
				ivm->pc++;
			}
			// step 4
			switch (ALU_RS(word)) {
				case (ALU_RS_PUSH): ivm->rp++; break;
				case (ALU_RS_POP): ivm->rp--; break;
			}
			// step 5
			if (word & (ALU_F_T_R)) {
				ivm->rs[ivm->rp] = t;
			}
			// step 6
			if (ivm->dp >= 0) {
				ivm->ds[ivm->dp] = res;
			} 

			break;			
		}
	}
	if(ivm->dp < -1) return RC_DS_UNDER_FLOW;
	if(ivm->rp < -1) return RC_DS_UNDER_FLOW;

	return 0;
}

#endif /* __IVM_H__ */
