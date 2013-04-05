/*
  matmul_unit.c

*/

#include <stddef.h>
#include "matmul.h"
//#include "matmul_edefs.h"

#include "static_buffers.h"
#include "common_buffers.h"

#define DRAM_SIZE (1<<14) // 0-0x3fff memory, 0x4000-0x7fff I/O 
#define ROM_SIZE (1<<13)
// note some I/O such as alarm and eth are indeed memory

#define RS232_TXD 0x5000
#define c_pkt_buffer 0x6200 // 0x600 1536 bytes
#define dma_addr_a 0x4120
#define dma_xf_n  0x4121
#define dma_pkt_addr 0x4122
#define ALARM 0x6000
#define ether_irq 0x5109


#include "ivm.h"

#ifdef DEVICE_EMULATION
//# 0-15
char ALU_OP_str[][16] = {"T", "N", "PLUS", "AND",
"OR", "XOR", "NEG", "EQ",
"LESS", "RSHIFT", "DEC", "R",
"MEM", "LSHIFT", "DEPTH", "ULESS"};

#define SER_TX(args...) printf(args)
#define DBG(args...)              \
 if (debug_mask & 0x01) {         \
    printf(args);                 \
 }

#define INFO(args...)              \
 if (1) {         \
    printf(args);                 \
 }
#else
#define DBG(args...)

#endif

//#define _gptr(coreID, ptr) ( (void *) ( (((unsigned) coreID) << 20) | ((unsigned) ((void *) ptr)) ) )

#define _MAX_MEMBER_ 64

#ifdef DEVICE_EMULATION
//# 0-15
#endif

static uint8_t debug_mask = 0;
void init();

static int get_host_seq() {
	int r = Mailbox.pCore->seq[me.corenum];
	return r;
}
static void set_own_seq(int seq) {
	Mailbox.pCore->count[me.corenum] = seq; 
}

void ivm_mem_put(uint16_t addr, uint16_t value, ivm_t* ivm) {
	if (RS232_TXD == addr) {
		SER_TX("%c", (uint8_t)value);
	} else { 
		uint8_t u2 = value & 0xff;
		uint8_t u1 = value >> 8;
		*(ivm->DRAM+addr+1) = u1;	
		*(ivm->DRAM+addr) = u2;	
	}
}
uint16_t ivm_mem_get(uint16_t addr, ivm_t* ivm) {
	//DBG( "get: addr %x \n", addr);
	if (dma_pkt_addr == addr) {
		return 0xffff;
	} else if (ether_irq == addr) {
		// put pakcet into packet buffer
		for (int i=0;i<0x600;i++) {
			*(ivm->DRAM+c_pkt_buffer+i) = (uint8_t)i;
		}
		return 0;
	} 
	if(addr & 0x1) {
		addr = addr - 1;
	}
		uint8_t u1 = *(ivm->DRAM+addr+1);
		uint8_t u2 = *(ivm->DRAM+addr);
		return (u1<<8|u2);

}

static uint16_t rom_get(uint16_t pc, ivm_t* ivm) {
        // here we can virtualize rom, put cache in between
        uint8_t u1 = *(ivm->DRAM+pc*2+1);
        uint8_t u2 = *(ivm->DRAM+pc*2);
        return (u1<<8|u2);
}

void dump_op(uint16_t op, ivm_t* ivm) {
	DBG("D%02d:%04x ", ivm->dp,ivm->ds[ivm->dp]);
	for(int i=0;i<ivm->rp;i++) {
		DBG(" ");
	}
	DBG("R%02d:%04x ", ivm->rp,ivm->rs[ivm->rp]);
	if(op & OP_LIT) DBG( "OP_LIT ds push 0x%x ",(op&0x7fff));
	uint16_t word = OP(op);
	uint16_t hex_arg_str = (ARG(op));
	if(OP_JZ == word)  { DBG( "OP_JZ %x ", ARG(op)); }
	else if( OP_JMP == word) { DBG( "OP_JMP %x ", hex_arg_str); }
	else if( OP_CALL== word) { DBG( "OP_CALL %x ", hex_arg_str); }
	else if( OP_ALU == word) {
		uint16_t t, n, r, res;
		t = ivm->ds[ivm->dp];
		if (ivm->dp > 0) {
			n = ivm->ds[ivm->dp-1];
		}
		r = ivm->rs[ivm->rp];
		DBG( "ALU ");
		uint8_t alu_op = ALU_OP(op) >> 8;
		DBG( "%s ", ALU_OP_str[alu_op] );
		switch (alu_op) {
			case ALU_OP_T: DBG( "%d ", (int16_t)t); break;
			case ALU_OP_N: {
				DBG( "%d ", (int16_t)n);
				break;
			}
			case ALU_OP_PLUS: {
				DBG( "%d ", (int16_t)t+n);
				break;
			}
			case ALU_OP_AND: {
				DBG( "%d ", (int16_t)t&n);
				break;
			}
			case ALU_OP_OR: {
				DBG( "%d ", (int16_t)t|n);
				break;
			}
			case ALU_OP_XOR: {
				DBG( "%d ", (int16_t)t ^ n);
				break;
			}
			case ALU_OP_NEG: DBG( "%x ", ~t); break; // -1, 0
			case ALU_OP_EQ: {
				DBG( "%d ", (int16_t)(t == n));
				break; // -1, 0
			}
			case ALU_OP_LESS: {
				DBG( "%d ", (int16_t)n<t);
				break; // -1, 0
			}
			case ALU_OP_RSHIFT: {
				DBG( "%d ", (int16_t)n>>t);
				break;
			}
			case ALU_OP_DEC: DBG( "%d ", (int16_t)t-1); break;
			case ALU_OP_R: DBG( "%d ", (int16_t)r); break;
			case ALU_OP_MEM: break;
			case ALU_OP_LSHIFT: {
				DBG( "%d ", (int16_t)n<<t);
				break;
			}
			case ALU_OP_DEPTH:  break;
			case ALU_OP_ULESS: {
				DBG( "%d ", (int16_t)n<t);
				break; // -1, 0
			}
			default: break;
		}

		if (ALU_OP_MEM == alu_op)
			DBG("get *0x%x=%x ",t, *(uint16_t*)(ivm->DRAM+t));
		if (ALU_DS(op) == ALU_DS_PUSH) DBG( "d+ ");
		if (ALU_DS(op) == ALU_DS_POP) DBG( "d- ");
		if (ALU_RS(op) == ALU_RS_PUSH) DBG( "r+ ");
		if (ALU_RS(op) == ALU_RS_POP) DBG( "r- ");
		if (ALU_RS(op) == ALU_RS_PP) DBG( "r-2 ");

		if (op & (ALU_F_N_aT)) DBG( "mem_put *0x%x=0x%x ",t,n);
		if (op & (ALU_F_T_R))  DBG( "r=0x%x ",t);
		if (op & (ALU_F_T_N))  DBG( "dN=0x%x ",t);
		if (op & (ALU_F_R_PC)) DBG( "pc=0x%x ", r);
	}
	DBG("\n");
}
static uint8_t do_something(char* input, ivm_t* ivm) {
	uint16_t op = rom_get(ivm->pc, ivm);
    uint16_t *p = (uint16_t*)(ivm->DRAM + ALARM);
    uint16_t t = *p;
    *p = t+1;
	DBG("pc %04x: %04x ",ivm->pc, op);
    dump_op(op, ivm);
	if (ivm_step(op, ivm)) {
	    //DBG("--- ABORT ---------------");
	    //dump_stack(ivm);
	    // raise vm error
	    return 'E';
	}
	return 0;

}

int matmul_unit()
{
	unsigned time_s, time_e;
	int n = 0;
	volatile int32_t* readp;
	volatile int host_seq = 0;
	char* op;
	int own_seq = 1;
	char os_state = 0;
	// Initialize data structures - mainly target pointers
	init();
	readp = &Mailbox.pCore->seq[me.corenum];
	ivm_t vcpu;

#ifdef DEVICE_EMULATION
	DBG ("core %d %x\n", me.corenum, readp);
#endif
	do {
#ifdef DEVICE_EMULATION
		  pthread_yield();
#endif			
	} while ( get_host_seq() == 0);
#ifdef DEVICE_EMULATION
	DBG ("core %d step 0 %x\n", me.corenum, readp);
#endif

	while (1)
	{
		if (host_seq == get_host_seq()) {
			if (os_state != 'g') continue;
			// no new instruction, carry on
			// do something
			uint8_t result = do_something("g", &vcpu);
			char tmps[sizeof(shared_msg_t)] = " : auto_run";
		    if (result) tmps[0] = result;
		    else tmps[0]='g';
			memcpy(Mailbox.pCore->go_out[me.corenum].msg, tmps, sizeof(shared_msg_t));
			set_own_seq(own_seq);
			own_seq++;
		} else {
			host_seq = get_host_seq();
			char *op = Mailbox.pCore->go[me.corenum].msg;
			INFO("core %d cmd %s os_state %c\n", me.corenum, op, os_state);
			if (*op == 'L' && os_state == 'r') {
				// loading the vm
				ivm_reset(&vcpu);
				vcpu.DRAM = Mailbox.pDRAM;
				for (int i=0;i<50;i++)DBG("%x \n",(uint16_t)*(vcpu.DRAM+2*i));
				os_state = 'g';
			} else if (*op == 'R') {
				os_state = 'r';
			} else if (*op == 's') {
				debug_mask = *(op+1);
			}

		    *op='g';
			memcpy(Mailbox.pCore->go_out[me.corenum].msg, op, sizeof(shared_msg_t));
			set_own_seq(own_seq);
			own_seq++;
		}
		n++;
#ifdef DEVICE_EMULATION
		  pthread_yield();
#endif
	}

	return 0;
}


void init()
{
        // Init core enumerations
        int coreID  = e_get_coreid();
        int row, col;
        e_coords_from_coreid(coreID, &row, &col);
        row     = row - E_FIRST_CORE_ROW;
        col     = col - E_FIRST_CORE_COL;
        me.corenum = row * E_COLS_IN_CHIP + col;

	// Initialize the mailbox shared buffer pointers
	Mailbox.pBase = (void *) SHARED_DRAM;
	Mailbox.pCore = Mailbox.pBase + offsetof(shared_buf_t, core);
	Mailbox.pDRAM = Mailbox.pBase + offsetof(shared_buf_t, DRAM);

#ifdef DEVICE_EMULATION
	//DBG ("core %d pbase %x\n", me.corenum, Mailbox.pBase);
	// Initialize per-core parameters - core data structure
#endif	
	// Initialize pointers to the operand matrices ping-pong arrays


	// Initialize the pointer addresses of the arrays in the horizontal and vertical target
	// cores, where the submatrices data will be swapped, and the inter-core sync signals.


	// Clear the inter-core sync signals

	// Init the host-accelerator sync signals
	//Mailbox.pCore->go[me.corenum] = 0;
#ifdef DEVICE_EMULATION
	//DBG ("core %d\n", me.corenum);
#endif
	if (me.corenum == 0)
		Mailbox.pCore->ready = 1;
	
	me.count = 0;

	return;
}



