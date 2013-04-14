/*
  static_buffers.c
*/

#include "static_buffers.h"

// Here's an example of explicit placement of static objects in memory. The three matrices
// are placed in the respective SRAM banks. However, if necessary, the linker may relocate
// the objects wherever within the bank. The core structure "me" is specifically located
// at an explicit address - 0x7000. To do that, a custom linker file (LDF) was defined,
// based on a standard LDF, in which a special data section was added with the required
// address to assign to the "me" object.
//volatile float  AA[2][_Score][_Score] SECTION(".data_bank1");  // local A submatrix
//volatile float  BB[2][_Score][_Score] SECTION(".data_bank2");  // local B submatrix
//volatile float  CC   [_Score][_Score] SECTION(".data_bank3");  // local C submatrix
//volatile e_tcb_t _tcb                 SECTION("section_core") ALIGN(8); // TCB structure for DMA

#ifdef DEVICE_EMULATION
__thread core_t me ;                  
__thread shared_buf_ptr_t Mailbox;
#else
volatile core_t me                    SECTION(".data_bank2"); // core data structure
volatile shared_buf_ptr_t Mailbox     SECTION(".data_bank3"); // Mailbox pointers;
#endif
