/*
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <arpa/inet.h>
#include <string.h>

#include "e-hal.h"
#include "matmul.h"
#include "common_buffers.h"
#include "pthread.h"

#ifdef DEVICE_EMULATION
#include "ep_emulator.h"
#endif

#define __DO_STRASSEN__
#define __WIPE_OUT_RESULT_MATRIX__
#define __DUMP_MATRICES__1

#define _MAX_MEMBER_ 64
#define eMHz 600
#define aMHz 800

#define FALSE 0
#define TRUE  1

int   main(int argc, char *argv[]);
void  matrix_init(int seed);

typedef struct {
	int  reset_target;
	int  broadcast;
	int  run_target;
	int  verbose;
	char srecFile[4096];
} args_t;

args_t ar = {TRUE, FALSE, TRUE, 0, ""};
void get_args(int argc, char *argv[]);

FILE *fo, *fi;

ssize_t e_mread_buf(e_mem_t *dram, const off_t from_addr, void *buf, size_t count) {
	ssize_t s = e_read(dram, 0, 0, from_addr, buf, count);
	if (s==E_ERR) {
		printf("e read error");
	}
	return s;
}
ssize_t e_mwrite_buf(e_mem_t *dram, off_t to_addr, const void *buf, size_t count) {
	ssize_t s = e_write(dram, 0, 0, to_addr, buf, count);
	if (s==E_ERR) {
		printf("e write error");
	}
	return s;
}

static 	pthread_t trace_reader_a ;
void * trace_reader(void *parm)
{
	int addr =  offsetof(shared_buf_t, tp_buf);
	uint32_t expect_seq = 1;
	uint8_t buf[TP_BUF_SIZE];

	while(1) {
		int pos = 0;
		e_mread_buf(parm, addr, buf, TP_BUF_SIZE);
		for(int i=0;i<100;i++)printf("%x ", buf[i]);
		printf("\n");
		uint32_t seq = *(uint32_t*)(buf);
		if (seq>expect_seq) {
			// this is writer faster than reader
			// todo consider display the wrapup case 789456
			expect_seq = seq;
		}

		while ((pos+6) < (TP_BUF_SIZE) ) {
			uint32_t seq = *(uint32_t*)(buf+pos);
			if (0==seq)break;
			uint8_t t = buf[pos+4];
			if (0==t)break;
			uint8_t len = buf[pos+5];
			if(0==len)break;
			// parse buffer
			if (seq < expect_seq) {
				// this is writer slower
				pos += (6+len);
				continue;
			}
			if (seq > expect_seq)break;
			expect_seq = (seq+1);
			if (pos+6+len>=(TP_BUF_SIZE)) break;
			printf("seq %x pos %d cmd %c len %d \n", seq, pos, t,len);
			if (t=='s')printf("%s\n",buf+pos+6);
			else if(t=='D'||t=='R'||t=='1'||t=='2'||t=='o') {
				printf("%x %x\n",*(uint32_t*)(buf+pos+6),*(uint32_t*)(buf+pos+10));
			} else if (t=='c') printf("%x %c\n",*(uint32_t*)(buf+pos+6),*(uint8_t*)(buf+pos+6));
			pos += (6+len);
		}

		sleep(3);
	}
	return NULL;
}



int main(int argc, char *argv[])
{
	e_platform_t platform;
	e_epiphany_t Epiphany, *pEpiphany;
	e_mem_t      DRAM,     *pDRAM;
	unsigned int msize;
	float        seed;
	unsigned int addr; //, clocks;
	size_t       sz;
	int    result, rerval;
	
	pEpiphany 	= &Epiphany;
	pDRAM  		= &DRAM;
	msize     	= 0x00400000;

	// load j1.bin into shared mem
	uint8_t ROM[1<<16];
	printf("load %s \n", argv[1]);	
	FILE* f = fopen("j1.bin","r");
	int i = 0;
	uint16_t op;
	while(fread(&op, 2, 1, f)) {
		*(ROM+i*2+1) = op&0xff;
		*(ROM+i*2) = op>>8;
		i++;
	}
	fclose(f);
	printf ("read %d words\n",i);
	for(int i=0;i<50;i++)printf("%x ", (uint16_t)*(ROM+2*i));
	printf("\n");
	get_args(argc, argv);

	fo = stdout;
	fi = stdin;

	// Connect to device for communicating with the Epiphany system
	// Prepare device
	e_set_host_verbosity(H_D0);
	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);

	if (e_alloc(pDRAM, 0x00000000, msize))
	{
		fprintf(fo, "\nERROR: Can't allocate Epiphany DRAM!\n\n");
		exit(1);
	}
	if (e_open(pEpiphany, 0, 0, 1, 1))
	{
		fprintf(fo, "\nERROR: Can't establish connection to Epiphany device!\n\n");
		exit(1);
	}
	e_reset_core(pEpiphany, 0, 0);

	//fprintf(fo, "host base %x \n", pDRAM->base); fflush(fo);
    // init all

    for(int i=0;i<16;i++){
		int n = 0;
		addr = offsetof(shared_buf_t, core.seq);
        e_mwrite_buf(pDRAM, addr, &n, sizeof(int));
        addr = offsetof(shared_buf_t, core.go_out);
        e_mwrite_buf(pDRAM, addr, &n, sizeof(int));
	} 
	addr = offsetof(shared_buf_t, DRAM);
    e_mwrite_buf(pDRAM, addr, ROM, sizeof(ROM));

	printf("Loading program on Epiphany chip...\n");
	//e_set_loader_verbosity(ar.verbose);
	//result = e_load_group(ar.srecFile, pEpiphany, 0, 0, pEpiphany->rows, pEpiphany->cols, ar.run_target);
	result = e_load(ar.srecFile, pEpiphany, 0, 0, ar.run_target);
	if (result == E_ERR) {
		printf("Error loading Epiphany program.\n");
		exit(1);
	}

	// Generate operand matrices based on a provided seed
	matrix_init(seed);
	sleep(2);

    struct sockaddr_in si_me, si_other;
    int s, slen, n=0;
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        printf("socket open error\n");
          return -1;
      }
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(27777);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &si_me, sizeof(si_me))==-1) {
        printf("ERROR, bind failed\n");
        return -1;
    }
	int ep_seq[16];
	bzero(ep_seq, 16*sizeof(int) );

    while (1) {
        // read command from udp client
   	    char buf[1024];
		ssize_t len =
		    recvfrom(s, buf, 255, 0, (struct sockaddr *)&si_other, (socklen_t*)&slen);
		if (len == -1) {
		    printf("socket error\n");
		    break;
		}
        printf("Received packet from %s:%d\nlen %d Data: %s\n",
                inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), len, buf);

        // R then L load vm, G run, R reset
        n = buf[0];
        printf("seq %d core %d cmd %c\n", ep_seq[n], n, buf[1]);
        if('s' == buf[1])printf("set debug mask %x \n",  buf[2]);
        else if('L' == buf[1]) {
            pthread_create(&trace_reader_a, NULL, trace_reader, pDRAM);
        }

		char string[256] = "aaa bbb ccc     ";
		char s_out[256];

		int addr_to = offsetof(shared_buf_t, core.go[n]);
		int addr_out = offsetof(shared_buf_t, core.go_out[n]);
		int addr_seq = offsetof(shared_buf_t, core.seq[n]);
		int addr_core_seq = offsetof(shared_buf_t, core.core_seq[n]);
		// send command
		memcpy(string,buf+1,10);
		sz= e_mwrite_buf(pDRAM, addr_to, string, 25 );
		sz= e_mwrite_buf(pDRAM, addr_seq, &ep_seq[n], sizeof(uint32_t) );
		ep_seq[n] ++;
		sleep(1);
		// read from core
		uint32_t core_out;
		sz = e_mread_buf(pDRAM, addr_out, s_out, 25);
		result = e_mread_buf(pDRAM, addr_core_seq, &core_out,sizeof(uint32_t));
		printf("check seq %x and output %s\n", core_out, s_out);
	}


	// Close connection to device
	if (e_close(pEpiphany))
	{
		fprintf(fo, "\nERROR: Can't close connection to Epiphany device!\n\n");
		exit(1);
	}
	if (e_free(pDRAM))
	{
		fprintf(fo, "\nERROR: Can't release Epiphany DRAM!\n\n");
		exit(1);
	}

	return rerval;
}




// Initialize operand matrices
void matrix_init(int seed)
{
	return;
}


// Process command line args
void get_args(int argc, char *argv[])
{
	int n;

	strcpy(ar.srecFile, "");

	for (n=1; n<argc; n++)
	{
		if (!strcmp(argv[n], "-no-reset"))
		{
			ar.reset_target = FALSE;
			continue;
		}

		if (!strcmp(argv[n], "-broadcast"))
		{
			ar.broadcast = TRUE;
			continue;
		}

		if (!strcmp(argv[n], "-no-run"))
		{
			ar.run_target = FALSE;
			continue;
		}

		if (!strcmp(argv[n], "-verbose"))
		{
			n++;
			if (n < argc)
			{
				ar.verbose = atoi(argv[n]);
				if (ar.verbose < 0)
						ar.verbose = 0;
			}
			continue;
		}

		if (!strcmp(argv[n], "-h") || !strcmp(argv[n], "--help"))
		{
			printf("Usage: matmul-16_host.elf [-no-reset] [-broadcast] [-no-run] [-verbose N] [-h | --help] [SREC_file]\n");
			printf("       N: available levels of diagnostics\n");
			exit(0);
		}

		strcpy(ar.srecFile, argv[n]);
	}

	if (!strcmp(ar.srecFile, ""))
		strcpy(ar.srecFile, "matmul-16.srec");

	return;
}


