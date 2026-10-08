#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "smmalloc.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "test_utils.h"

/** Blocks handed from the parent to the child in each pass. **/
#define N_BLOCKS	2000

/** Region and message queue shared by every pass; created and destroyed by test(). **/
static pSmRegion region = NULL;
static int msg_id = -1;

static bool
doTest(void)
    {
    int i;
    char* ptr;
    int childpid;
    struct my_msgbuf { long mtype; char mtext[sizeof(char*)]; } buf;
    int status;

	childpid = fork();
	if (childpid < 0) return false;

	if (childpid == 0)
	    {
	    /** in child **/
	    for(i=0;i<N_BLOCKS;i++)
		{
		msgrcv(msg_id, (struct msgbuf*)&buf, sizeof(char*), 1, 0);
		memcpy(&ptr, buf.mtext, sizeof(char*));
		ptr = smToAbs(region, ptr);
		smFree(ptr);
		}
	    exit(0);
	    }

	/** in parent **/
	for(i=0;i<N_BLOCKS;i++)
	    {
	    ptr = smMalloc(region, 1024 + (rand()%1025));
	    if (!ptr)
		{
		/** memory full; wait 1ms and try again **/
		i--;
		usleep(1000);
		continue;
		}
	    ptr = smToRel(region, ptr);
	    memcpy(buf.mtext, &ptr, sizeof(char*));
	    buf.mtype = 1;
	    msgsnd(msg_id, (struct msgbuf*)&buf, sizeof(char*), 0);
	    }
	if (waitpid(childpid, &status, 0) != childpid) return false;

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-09 2 process, A:malloc -> B:free";

	smInitialize();
	srand(time(NULL));
	region = smCreate(1024*1024);
	if (!region) return -1;
	msg_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
	if (msg_id < 0)
	    {
	    smDestroy(region);
	    region = NULL;
	    return -1;
	    }

	rval = loopTest(doTest);

	if (rval > 0) rval *= N_BLOCKS;

	msgctl(msg_id, IPC_RMID, NULL);
	msg_id = -1;
	smDestroy(region);
	region = NULL;

    return rval;
    }
