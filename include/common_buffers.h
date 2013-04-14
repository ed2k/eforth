/*
  common_buffers.h

*/


#ifndef __COMMON_BUFFERS_H__
#define __COMMON_BUFFERS_H__

#ifdef __HOST__
#define SECTION(x)
//shared_buf_t Mailbox;

#else // __HOST__
#include <e_common.h>
#endif // __HOST__

//volatile shared_buf_t Mailbox SECTION(".shared_dram");
#ifdef DEVICE_EMULATION
extern volatile void* _SHARED_DRAM_;
#define SHARED_DRAM   (_SHARED_DRAM_)
#else
extern const unsigned _SHARED_DRAM_;
#define SHARED_DRAM   ((void *)(&_SHARED_DRAM_))

#endif



#endif // __COMMON_BUFFERS_H__
