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

#define UNUSED(V) ((void) V)
#define RANDPTR_INITIAL_SIZE 8

int main(int argc, const char **argv) {
	redisContext *context;

	char buf[256], reply[256];

	write(context->fd, buf, strlen(buf));
	redisBufferRead(context);
	redisGetReply(context, &reply);
	printf("%s\n", reply);

	/*
	FILE *pf = fopen("/home/mazx/work/bigt.in", "r");
	char buf[100], tmp[100];
	fgets(buf, 100, pf);
	sscanf(buf, "%d", &opnum);
	//if (config.requests < opnum) opnum = config.requests;
	opid = 0;
	for (int i = 0; i < opnum; i++) {
		char ch;
		int t;
		fgets(buf, 99, pf);
		sscanf(buf+1, "%d", &t);
		strcpy(tmp, buf);
		for (int j = 0; j < 2 * t; j++) {
			fgets(buf, 99, pf);
			strcat(tmp, buf);

		}
		strcpy(optable[i], tmp);
	}
	*/

	return 0;
}
