/* <title of the code in this file>
   <http://www.gnu.org/licenses/>. */

/**
 * Emulation routines and data structures which allow compiling and running core code on host.
 * Activated only if DEVICE_EMULATION is defined.
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "e-host.h"
#include "ep_emulator.h"
#include "pthread.h"


void * get_SHARED_DRAM() {
  return (void*)_SHARED_DRAM_;
}

__thread unsigned int emu_coreid;

typedef struct {
   unsigned int   cidx;
} threadparm_t;


/**
 * global time counter
 */
int emulated_timer;

/**
 * Start timer.
 * @return start value of timer
 */
static unsigned int start_timer() {
    //emulated_timer = cvGetTickCount();
  printf("TODO implement %s\n", __func__);
    return ~0;
}

/**
 * Stop timer.
 * @return stop value of timer
 */
static unsigned int stop_timer() {
    //double const time = (cvGetTickCount() - emulated_timer) / cvGetTickFrequency();
    //return ~((unsigned int)0) - cvRound(time * CORE_FREQUENCY);
  printf("TODO implement %s\n", __func__);
    return ~0;
}

/**
 * Emulate DMA data transfer.
 * calls memcpy(dst, src, size)
 */
static void dma_transfer (
    void       volatile *const dst,
    void const volatile *const src,
    unsigned int         const size,
    int                  const wait
) {
    memcpy( (void *)dst, (void const *)src, size );
}

#define e_mutex_t pthread_mutex_t
#define e_mutex_lock pthread_mutex_lock
#define e_mutex_unlock pthread_mutex_unlock

static e_mutex_t *get_global_mutex_pointer(void) {
	static pthread_mutex_t emu_mutex;
    return (e_mutex_t *)(&emu_mutex);
}

/**
 * Increment shared variable
 * @param val pointer on variable for increment
 * @param max_val max value of variable
 * @return unmodified (*val) value
 */
static int atomic_increment(int volatile *const val, int const max_val) {
    e_mutex_t *const m_ptr = get_global_mutex_pointer();

    e_mutex_lock(m_ptr);

    int const cur_val = *val;
    if(cur_val < max_val)
        *val = cur_val + 1;
    e_mutex_unlock(m_ptr);

    return cur_val;
}

static int atomic_decrement(int volatile * const val, int const min_val) {
    e_mutex_t *const m_ptr = get_global_mutex_pointer();

    e_mutex_lock(m_ptr);
    int const cur_val = *val;
    if(cur_val > min_val)
        *val = cur_val - 1;
    e_mutex_unlock(m_ptr);

    return cur_val;
}



void *theThread(void *parm)
{
   threadparm_t     *gData;
   gData = (threadparm_t *)parm;
   // thread local storage
   emu_coreid = gData->cidx;
   matmul_unit();
   return NULL;
}

static 	pthread_t             thread[NUMTHREADS];
int     e_open(Epiphany_t *dev) {
  printf("TODO implement %s\n", __func__);
  return 0;
}

int     e_close(Epiphany_t *dev) {
  printf("TODO implement %s\n", __func__);
  return 0;
}


int e0_close() {

	printf("Wait for the threads to complete, and release their resources\n");
	for (int i=0; i < NUMTHREADS; i++) {
		int rc = pthread_join(thread[i], NULL);
		printf("pthread_join() %d\n", rc);
	}

	printf("Main completed\n");
    return 0;
}

ssize_t e_read(Epiphany_t *dev, int corenum, const off_t from_addr, void *buf, size_t count) {
  printf("TODO implement %s\n", __func__);
  return 0;
}
ssize_t e_write(Epiphany_t *dev, int corenum, off_t to_addr, const void *buf, size_t count) {
  printf("TODO implement %s\n", __func__);
  return 0;
}


ssize_t e_mread(DRAM_t *dram, const off_t from_addr, void *buf, size_t count) {
  char* from = (char*)(dram->base) + from_addr;
  memcpy(buf, (void*)from, count);
  return count;
}
ssize_t e_mwrite(DRAM_t *dram, off_t to_addr, const void *buf, size_t count) {
  char* to = (char*)(dram->base) + to_addr;
  memcpy((void*)to, buf, count);
  return count;
}
int     e_mread_word(DRAM_t *dram, const off_t from_addr) {
  int p;
  e_mread(dram, from_addr, &p, sizeof(int));
  return (p);
}

ssize_t e_mwrite_word(DRAM_t *dram, off_t to_addr, int data) {
  int p = (data); 
  e_mwrite(dram, to_addr, &p, sizeof(int));
  return sizeof(int);
}
ssize_t e_mread_buf(DRAM_t *dram, const off_t from_addr, void *buf, size_t count) {
  return e_mread(dram, from_addr, buf, count);
}
ssize_t e_mwrite_buf(DRAM_t *dram, off_t to_addr, const void *buf, size_t count) {
  return e_mwrite(dram, to_addr, buf, count);
}


// eDRAM access
int     e_alloc(DRAM_t *dram, off_t mbase, size_t msize){
  dram->base = malloc(msize);
  dram->map_size = msize;
  dram->phy_base = mbase;
  _SHARED_DRAM_ = ( void *)dram->base;
  return 0;
}
int     e_free(DRAM_t *dram){
  free(dram->base);
  return 0;
}

/////////////////////////
// Core control functions
int e_send_core_reset(Epiphany_t *pEpiphany, unsigned int coreid){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
int e_send_core_reset_ID(Epiphany_t *pEpiphany, unsigned int coreid){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
int e_send_reset(Epiphany_t *pEpiphany, e_resetid_t resetid){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
int e_send_ILAT(Epiphany_t *pEpiphany, unsigned int coreid){
  printf("TODO implement %s\n", __func__);
  return 0;  
}


////////////////////
// Utility functions

unsigned int e_get_coreid() {
    return emu_coreid;
}
unsigned int e_get_num_from_coords(int row, int col){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
unsigned int e_get_num_from_id(unsigned int coreid){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
unsigned int e_get_id_from_coords(int row, int col){
  printf("TODO implement %s\n", __func__);
  return 0;  
}
unsigned int e_get_id_from_num(int corenum){
  printf("TODO implement %s\n", __func__);
    return 0;
}
void  e_get_coords_from_id(unsigned int coreid, int *row, int *col){
  *row = coreid/4;
  *col = coreid % 4;
}
void  e_get_coords_from_num(int corenum, int *row, int *col){
  *row = corenum/4;
  *col = corenum % 4;  
}
void e_coords_from_coreid(e_coreid_t coreid, unsigned *row, unsigned *col){
  *row = coreid/4;
  *col = coreid % 4;
}
int e_is_oncore(const void *ptr){
  printf("TODO implement %s\n", __func__);
  return 0;
}
e_coreid_t e_coreid_origin(void) {
  printf("TODO implement %s\n", __func__);
  return emu_coreid;
}
int e_neighbor_id(e_coreid_t *coreid, e_coreid_wrap_t dir, e_coreid_wrap_t wrap){
  printf("TODO implement %s\n", __func__);
  return 0;
}

void e_set_host_verbosity(int verbose){}



// loader
int e_load(char *srecFile, bool reset_target, bool broadcast, bool run_target) {
  int                   i;
  static threadparm_t          gData[NUMTHREADS];
    e_mutex_t *const m_ptr = get_global_mutex_pointer();

    pthread_mutex_init(m_ptr, 0);
    //pthread_mutex_lock(m_ptr);

  printf("Create/start threads\n");
  for (i=0; i < NUMTHREADS; i++) {
    /* Create per-thread TLS data and pass it to the thread */
    gData[i].cidx = i;
    printf("Create/start thread %d\n", i);
    pthread_create(&thread[i], NULL, theThread, &gData[i]);
  }
    return 0;
}

int parseAndSendSrecFile(char *srecFile, Epiphany_t *p, bool broadcast, int verbose){
  printf("TODO implement %s\n", __func__);
  return 0;
}

void e_set_loader_verbosity(int verbose){
  printf("TODO implement %s\n", __func__);
}

