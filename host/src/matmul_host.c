/*
*/

//#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <arpa/inet.h>
#include <string.h>
#include "e-host.h"
#include "e-loader.h"
#include "matmul.h"
#include "common_buffers.h"
#include <pthread.h>

#include "ep_emulator.h"

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
//float Cref[_Smtx * _Smtx];
//float Cdiff[_Smtx * _Smtx];
//
//typedef struct timeval timeval_t;
//timeval_t timer[4];

int main(int argc, char *argv[])
{
	Epiphany_t   Epiphany, *pEpiphany;
	volatile DRAM_t       DRAM;
	unsigned int msize;
	float        seed;
	unsigned int addr; //, clocks;
	size_t       sz;
	double       tdiff[2];
	volatile int          result, rerval;
	
	pEpiphany = &Epiphany;
	volatile DRAM_t* pDRAM     = &DRAM;
	msize     = 0x00400000;

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
	for(int i=0;i<50;i++)printf("%x \n", (uint16_t)*(ROM+2*i));
	get_args(argc, argv);


	fo = stdout;
	fi = stdin;


	// Connect to device for communicating with the Epiphany system
	// Prepare device
	if (e_alloc(pDRAM, 0x00000000, msize))
	{
		fprintf(fo, "\nERROR: Can't allocate Epiphany DRAM!\n\n");
		exit(1);
	}
	//fprintf(fo, "host base %x \n", pDRAM->base); fflush(fo);
    // init all
	// Initialize Epiphany "Ready" state
	addr = offsetof(shared_buf_t, core.ready);
	//Mailbox.core.ready = 0;
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
	e_set_loader_verbosity(ar.verbose);
	result = e_load(ar.srecFile, ar.reset_target, ar.broadcast, ar.run_target);
	if (result == EPI_ERR) {
		printf("Error loading Epiphany program.\n");
		exit(1);
	}
	if (e_open(pEpiphany))
	{
		fprintf(fo, "\nERROR: Can't establish connection to Epiphany device!\n\n");
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

		char string[256] = "aaa bbb ccc     ";
		char s_out[256];

		int addr_to = offsetof(shared_buf_t, core.go[n]);
		int addr_out = offsetof(shared_buf_t, core.go_out[n]);
		int addr_seq = offsetof(shared_buf_t, core.seq[n]);
		int addr_count = offsetof(shared_buf_t, core.count[n]);
		// send command
		memcpy(string,buf+1,10);
		sz= e_mwrite_buf(pDRAM, addr_to, string, 256 );
		sz= e_mwrite_word(pDRAM, addr_seq, ep_seq[n] );
		ep_seq[n] ++;
		// read back seq
		//result = e_mread_word(pDRAM, addr_seq);
//		do {
//			result = e_mread_word(pDRAM, addr_count );
//		} while (ep_seq==result);
//		ep_seq = result;
//		sz = e_mread_buf(pDRAM, addr_out, s_out, 256);
//		fprintf(fo, "%d %s\n", ep_seq, s_out);
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


