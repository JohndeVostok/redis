#include "fmacros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>

#include <sds.h> /* Use hiredis sds. */
#include "ae.h"
#include "hiredis.h"
#include "adlist.h"
#include "zmalloc.h"

#ifdef ENABLE_MPI
#include <mpi.h>
int mpi_size,mpi_rank;
#endif

const int OPSIZE = 2000001;
const int BUFSIZE = 4500;
const int SERVERSIZE = 16;

int comp (const void * elem1, const void * elem2)
{
    double f = *((double*)elem1);
    double s = *((double*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}

static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

static long long mstime(void) {
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long long)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

unsigned short crc16(const char *buf, int len);

int getServer(char *buf, int len, int nserver) {
	int t = crc16(buf, len) & 0x3FFF;
	int p = t * nserver / 16384;
	if (p < 0 || p > 2) printf("!!!!! %d\n", p);
	return p;
}

int main(int argc, const char **argv) {
#ifdef ENABLE_MPI
	MPI_Init(NULL,NULL);
	MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
	printf("Using MPI, size: %d rank: %d\n",mpi_size,mpi_rank);
#endif
	redisContext* c[SERVERSIZE];

	char **optab;
	optab = malloc(OPSIZE * sizeof(char *));
	int *ops;
	ops = malloc(OPSIZE * sizeof(int));
	long long *oplat;
	oplat = malloc(OPSIZE * sizeof(long long));
	long long *optime;
	optime = malloc(OPSIZE * sizeof(long long));
	int opid = 0, opnum = 0;
	
///////////////////////
	FILE *pfc = fopen("config.txt", "r");
	char hostip[64];
	int hostport;
	int nserver;

	fscanf(pfc, "%d", &nserver);
	for (int i = 0; i < nserver; i++) {
		fscanf(pfc, "%s%d", hostip, &hostport);
		c[i] = redisConnectNonBlock(hostip, hostport);
	}
	char path[128];
	fscanf(pfc, "%s", path);
	printf("%s\n", path);
	fclose(pfc);
	long long op, ed;
	long long tin, tout;

	int len;
	
	FILE *pf = fopen(path, "r");
	char buf[BUFSIZE], tmp[BUFSIZE];
	int t;

	int cnt[3];
	memset(cnt, 0, sizeof(cnt));
	while (fgets(buf, BUFSIZE, pf)) {
		sscanf(buf+1, "%d", &t);
		strcpy(tmp, buf);
		for (int i = 0; i < 2 * t; i++) {
			fgets(buf, BUFSIZE, pf);
			strcat(tmp, buf);
			if (i == 3) {
				ops[opnum] = getServer(buf, strlen(buf) - 2, nserver);
			}
		}
		fgets(buf, BUFSIZE, pf);
		optab[opnum] = malloc(strlen(tmp) + 1);
		strcpy(optab[opnum], tmp);
		cnt[ops[opnum]]++;
		opnum++;
	}
	for (int i = 0; i < 3; i++) printf("%d ", cnt[i]); printf("\n");
	printf("load finished\n");
	char rep[BUFSIZE];
	int test_num=opnum/mpi_size;
#ifdef ENABLE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
	tin = ustime();
	for (int i = test_num*mpi_rank; i < test_num*(mpi_rank+1); i++) {
		op = ustime();
		len = send(c[ops[i]]->fd, optab[i], strlen(optab[i]), 0);
		while (1) {
			len = recv(c[ops[i]]->fd, rep, 256, 0);
			if (len > 0) break;
		}
		ed = ustime();
		oplat[opid] = ed - op;
		optime[opid] = ustime();
		opid++;
		if (!(i % 10000)) printf("%d finished!\n", i);
	}
	tout = ustime();
	long long *all_oplat;
	long long *all_optime;
	if (mpi_rank==0){
		all_oplat=malloc(test_num*mpi_size*sizeof(long long));
		all_optime=malloc(test_num*mpi_size*sizeof(long long));
		MPI_Gather(oplat,test_num,MPI_LONG_LONG,all_oplat,test_num,MPI_LONG_LONG,0,MPI_COMM_WORLD);
		MPI_Gather(optime,test_num,MPI_LONG_LONG,all_optime,test_num,MPI_LONG_LONG,0,MPI_COMM_WORLD);
	}else{
		MPI_Gather(oplat,test_num,MPI_LONG_LONG,all_oplat,test_num,MPI_LONG_LONG,0,MPI_COMM_WORLD);
		MPI_Gather(optime,test_num,MPI_LONG_LONG,all_optime,test_num,MPI_LONG_LONG,0,MPI_COMM_WORLD);
	}
	if (mpi_rank==0){
		int frank=0;
		long long ftime=all_optime[(frank+1)*test_num-1];
		for(int i=1;i<mpi_size;++i){
			if(ftime>all_optime[(i+1)*test_num-1]){
				frank=i;
				ftime=all_optime[(i+1)*test_num-1];
			}
		}
		long long *ok_oplat=malloc(test_num*mpi_size*sizeof(long long));
		long long fcnt=0;
		long long fall=0;
		for(int i=0;i<test_num*mpi_size;++i){
			if(all_optime[i]<=ftime){
				ok_oplat[fcnt]=all_oplat[i];
				fall+=ok_oplat[fcnt];
				fcnt++;
			}
		}
		qsort(ok_oplat,fcnt,sizeof(long long),comp);
		printf("MIN %lld\n", ok_oplat[0]);
		printf("MAX %lld\n", ok_oplat[fcnt-1]);
		printf("AVE %lf\n", (double)fall/(double)fcnt);
		for (int i=0;i<100;i+=5){
			printf("%d%% %lld\n",i,ok_oplat[(int)(i*0.01*fcnt)]);
		}
		printf("throughput: %lf\n",(double)fcnt/(double)(ftime-tin)*1000000.0);
		
	}
#ifdef ENABLE_MPI
	MPI_Finalize();
#endif
	return 0;
}
