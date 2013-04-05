/**
 * Header file for emulation routines and data structures which allow compiling
 * and running core code on host. Activated only if DEVICE_EMULATION is defined.
 */

#ifndef EP_EMULATOR_H
#define EP_EMULATOR_H

extern int matmul_unit();
#ifdef DEVICE_EMULATION
volatile void * _SHARED_DRAM_;
#endif

extern void * get_SHARED_DRAM();

#define E_ROWS_IN_CHIP   (4)
#define E_COLS_IN_CHIP   (4)
#define E_FIRST_CORE_ROW (0)
#define E_FIRST_CORE_COL (0)

#define                 NUMTHREADS   16

typedef enum
{
/* e_neighbor_id() wrap constants */
        E_CHIP_WRAP = 0,
        E_ROW_WRAP  = 1,
        E_COL_WRAP  = 2,
/* e_neighbor_id() dir constants */
        E_NEXT_CORE = 0,
        E_PREV_CORE = 1
} e_coreid_wrap_t;

typedef unsigned int e_coreid_t;


extern e_coreid_t e_get_coreid();

extern e_coreid_t e_coreid_from_address(const void *ptr);

extern e_coreid_t e_coreid_from_coords(unsigned row, unsigned col);

extern void *e_address_from_coreid(e_coreid_t coreid, void *ptr);

extern void e_coords_from_coreid(e_coreid_t coreid, unsigned *row, unsigned *col);
extern int e_is_oncore(const void *ptr);
extern e_coreid_t e_coreid_origin(void);
extern int e_neighbor_id(e_coreid_t *coreid, e_coreid_wrap_t dir, e_coreid_wrap_t wrap);

#endif//EP_EMULATOR_H


