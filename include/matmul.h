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
	shared_msg_t    go[_Ncores]; 	// host send buffer
	int32_t      	ready;       	// Core is ready after reset
	int32_t      	clocks;      	// Cycle count
	int32_t 	 	count[_Ncores]; // core send count
	int32_t 	 	seq[_Ncores]; 	// host send count
} mbox_t;


typedef struct {
	mbox_t core;
	uint16_t DRAM[1<<16];
} shared_buf_t;


typedef struct {
	void   *pBase;       // ptr to base of shared buffers
	mbox_t *pCore;       // ptr to cores mailbox
	uint8_t* pDRAM;
} shared_buf_ptr_t;


#endif // __MATMUL_H__
