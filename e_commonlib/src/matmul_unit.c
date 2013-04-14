/*
  matmul_unit.c

*/

#include <stddef.h>

#include "matmul.h"

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

#define SER_TX(args...) printf(args)

#define INFO(args...)    do{          \
 if (1) {         \
    printf(args);                 \
 } } while (0)
#else  // no DEVICE_EMULATION
#define SER_TX(args...) ;

#define INFO(fmt, args...) ;

#endif // no DEVICE_EMULATION


#define DBG_u32_2(t,arg, arg2)  do {            \
 if (Mailbox.tp_debug_mask & 0x01) {         \
	 TP_u32_2(t,arg, arg2);                 \
 } } while (0)


//todo check volatile


//shared_buf_t _SHARED_DRAM_ SECTION("shared_dram");

void init();

static void ringbuffer_write(uint8_t type, uint8_t len, uint8_t* buf) {
	if (0==len) return;
	if (0==type)return;
	int size = (4+2+len);
//shared_buf_ptr_t *base = &((shared_buf_t*)Mailbox.pBase)->t;
	D_tp_record_t d;
	volatile int pos = Mailbox.tp_pos;

	//printf("dbg seq %x pos %d \n",Mailbox.tp_seq, Mailbox.tp_pos);
	if ((pos+size) >= TP_BUF_SIZE ) {
		pos = 0;
	}
	d.seq =  Mailbox.tp_seq;
	d.type = type;
	d.len = len;
	memcpy(Mailbox.pTP_buf+pos, &d, 6);
	memcpy(Mailbox.pTP_buf+pos+6,buf,len);
	Mailbox.tp_pos = (pos+size);
	Mailbox.tp_seq++;
	if(Mailbox.tp_seq == 0) Mailbox.tp_seq = 1;
}

static uint32_t get_host_seq() {
	uint32_t r = Mailbox.pCore->seq[me.corenum];
	return r;
}
static void set_own_seq(uint32_t seq) {
	//Mailbox.pCore->count[me.corenum] = seq;
}

void ivm_mem_put(uint16_t addr, uint16_t value, ivm_t* ivm) {
	if (RS232_TXD == addr) {
		SER_TX("%c", (uint8_t)value);
		DBG_u32_2( D_ser, value, 0);
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
	DBG_u32_2(D_dstack,ivm->dp,ivm->ds[ivm->dp]);
	DBG_u32_2(D_rstack, ivm->rp,ivm->rs[ivm->rp]);
	if(op & OP_LIT) DBG_u32_2( D_op, op, 0);
	uint16_t word = OP(op);
	uint16_t hex_arg_str = (ARG(op));
	if(OP_JZ == word)  {  }
	else if( OP_JMP == word) {  }
	else if( OP_CALL== word) {  }
	else if( OP_ALU == word) {
		uint16_t t, n, r, res;
		t = ivm->ds[ivm->dp];
		if (ivm->dp > 0) {
			n = ivm->ds[ivm->dp-1];
		}
		r = ivm->rs[ivm->rp];
		uint8_t alu_op = ALU_OP(op) >> 8;
		DBG_u32_2( D_op, op, 0 );
		DBG_u32_2( D_tn, t, n );

		switch (alu_op) {
			case ALU_OP_R: DBG_u32_2( D_r, (int16_t)r, 0); break;
		}
		if (ALU_OP_MEM == alu_op)
			DBG_u32_2(D_addr, *(uint16_t*)(ivm->DRAM+t),0);

		if (op & (ALU_F_R_PC)) DBG_u32_2( D_r, r, 0);
	}
}
static uint8_t do_something(char* input, ivm_t* ivm) {
	uint16_t op = rom_get(ivm->pc, ivm);
    uint16_t *p = (uint16_t*)(ivm->DRAM + ALARM);
    uint16_t t = *p;
    *p = t+1;
    //DBG_u32_2(D_op,op, ivm->pc);
    //dump_op(op, ivm);
	if (ivm_step(op, ivm)) {
	    //DBG("--- ABORT ---------------");
	    //dump_stack(ivm);
		dump_op(op,ivm);
	    // raise vm error
	    return 'E';
	}
	return 0;

}

int matmul_unit()
{
	volatile uint32_t nnnn = 1;
//	volatile int32_t* readp;
	volatile int host_seq = 0;
	volatile int own_seq = 1;
	volatile char os_state = 0;
	// Initialize data structures - mainly target pointers
	init();

//	readp = &Mailbox.pCore->seq[me.corenum];
	ivm_t vcpu;

	//INFO ("core %d %x\n", me.corenum, readp);
	do {
#ifdef DEVICE_EMULATION
		  pthread_yield();
#endif			
	} while ( get_host_seq() == 0);
	//INFO ("core %d step 0 %x\n", me.corenum, readp);

	while (1)
	{
		if (host_seq == get_host_seq()) {
			//DBG_u32_2(D_arg1,own_seq,0);
			if (os_state != 'g') continue;
			// no new instruction, carry on
			// do something
			uint8_t result = do_something("g", &vcpu);
			char tmps[sizeof(shared_msg_t)] = " : auto_run";
		    if (result) tmps[0] = result;
		    else tmps[0]='g';
			//memcpy(Mailbox.pCore->go_out[me.corenum].msg, tmps, 25);
			//set_own_seq(own_seq);
			//own_seq++;
		} else {
			host_seq = get_host_seq();
			char *op = Mailbox.pCore->go[me.corenum].msg;
			INFO("core %d cmd %s os_state %c\n", me.corenum, op, os_state);
			DBG_u32_2(D_arg2, op, os_state);
			if (*op == 'L' && os_state == 'r') {
				// loading the vm
				ivm_reset(&vcpu);
				vcpu.DRAM = Mailbox.pDRAM;
				for (int i=0;i<50;i++)INFO("%x \n",(uint16_t)*(vcpu.DRAM+2*i));
				os_state = 'g';
			} else if (*op == 'R') {
				os_state = 'r';
			} else if (*op == 's') {
				Mailbox.tp_debug_mask = *(op+1);
			}

			memcpy(Mailbox.pCore->go_out[me.corenum].msg, op, 25);
			set_own_seq(nnnn);
			//own_seq++;
		}
		nnnn++;
		//memcpy(&Mailbox.pCore->core_seq[me.corenum], &nnnn, 4);
		//set_own_seq(n);
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
	unsigned int row, col;
	e_coords_from_coreid(coreID, &row, &col);
	row     = row - E_FIRST_CORE_ROW;
	col     = col - E_FIRST_CORE_COL;
	me.corenum = row * E_COLS_IN_CHIP + col;

	// Initialize the mailbox shared buffer pointers
	Mailbox.pBase = (void *) SHARED_DRAM;
	//Mailbox.pBase = (void *) 0x8e000000;
	Mailbox.pCore = Mailbox.pBase + offsetof(shared_buf_t, core);
	Mailbox.pDRAM = Mailbox.pBase + offsetof(shared_buf_t, DRAM);

	Mailbox.tp_buf_size = TP_BUF_SIZE;
	Mailbox.pTP_buf = (uint32_t)Mailbox.pBase + offsetof(shared_buf_t, tp_buf);
	Mailbox.tp_seq = 1;
	Mailbox.tp_pos = 0;
	Mailbox.tp_debug_mask = 0xff;


	if (me.corenum == 0) {
		bzero(Mailbox.pTP_buf, TP_BUF_SIZE);
	}
	me.count = 0;

	return;
}



