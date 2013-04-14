/*
  matmul.h

*/


#ifndef __MATMUL_H__
#define __MATMUL_H__

#include <stdint.h>

#define _Nchips 4                  // # of chips in operand matrix side
#define _Nside  4                  // # of cores in chip side
#define _Ncores (_Nside * _Nside)  // Num of cores = 16
#define _Score  32                 // side size of per-core sub-submatrix (max 32)
#define _Schip  (_Score * _Nside)  // side size of per-chip submatrix
#define _Smtx   (_Schip * _Nchips) // side size of operand matrix

#define _Nbanks 4                  // Num of SRAM banks on core

#define _BankP  0
#define _BankA  1
#define _BankB  2
#define _BankC  3
#define _PING   0
#define _PONG   1


typedef struct {
	int    corenum;
	int    count;
} core_t;

typedef struct {
	char msg[256];
} shared_msg_t;

typedef struct {
	shared_msg_t    go_out[_Ncores];// core send buffer
	uint8_t barrier1[1024];
	shared_msg_t    go[_Ncores]; 	// host send buffer
	uint8_t barrier2[1024];
	uint32_t 	 	core_seq[_Ncores]; // core send count
	uint32_t 	 	seq[_Ncores]; 	// host send count
} mbox_t;

#define TP_BUF_SIZE (1<<10)


typedef struct {
	int tp_buf_size;
	uint32_t tp_seq;
	uint32_t tp_pos;
	uint8_t tp_debug_mask;

	void *pBase;       // ptr to base of shared buffers
	mbox_t *pCore;       // ptr to cores mailbox
	uint8_t* pDRAM;
	uint32_t pTP_buf;
} shared_buf_ptr_t;

typedef struct {
	mbox_t core;
	shared_buf_ptr_t t;
	uint8_t DRAM[1<<16];
	uint8_t tp_buf[TP_BUF_SIZE];
} shared_buf_t;

typedef struct {
	uint32_t seq;
	uint8_t type;
	uint8_t len;
	uint8_t buf[0];
} D_tp_record_t;


#define D_sys 0
#define D_str 's'
#define D_arg1 '1'
#define D_arg2 '2'
#define D_dstack 'D'
#define D_rstack 'R'
#define D_op   'o'
#define D_tn   't'
#define D_r    'r'
#define D_addr 'a'
#define D_ser  'c'

typedef struct {
	uint32_t v1;
	uint32_t v2;
} D_u32_2_t;

#define TP_u32_2(t,d,v) do { \
	D_u32_2_t b; \
	b.v1 = d; \
	b.v2 = v; \
	ringbuffer_write(t,8,&b); \
} while (0)

#endif // __MATMUL_H__
