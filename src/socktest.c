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

const int OPSIZE = 2000001;
const int BUFSIZE = 4500;
const int SERVERSIZE = 16;

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
	tin = mstime();
	for (int i = 0; i < opnum; i++) {
		op = ustime();
		len = send(c[ops[i]]->fd, optab[i], strlen(optab[i]), 0);
		while (1) {
			len = recv(c[ops[i]]->fd, rep, 256, 0);
			if (len > 0) break;
		}
		ed = ustime();
		oplat[opid] = ed - op;
		optime[opid] = mstime();
		opid++;
		if (!(i % 10000)) printf("%d finished!\n", i);
	}
	tout = mstime();
	int mn = 1e7, mx = 0;
	for (int i = 0; i < opnum; i++) {
		if (oplat[i] > mx) mx = oplat[i];
		if (oplat[i] < mn) mn = oplat[i];
	}
	printf("%d %d %lld\n", mx, mn, tout-tin);
	return 0;
}
