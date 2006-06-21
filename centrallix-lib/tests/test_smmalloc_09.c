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

long long
test(char** tname)
    {
    int i;
    pSmRegion r;
    int iter;
    char* ptr;
    int pipefd[2];
    int childpid;
    int msg_id;
    struct my_msgbuf { long mtype; char mtext[sizeof(char*)]; } buf;
    int status;

	smInitialize();

	*tname = "smmalloc-09 2 process, A:malloc -> B:free";
	srand(time(NULL));
	iter = 20000;
	r = smCreate(1024*1024);

	pipe(pipefd);
	msg_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);

	childpid = fork();
	if (childpid < 0)
	    {
	    smDestroy(r);
	    return -1;
	    }

	if (childpid == 0)
	    {
	    /** in child **/
	    close(pipefd[1]);
	    for(i=0;i<iter;i++)
		{
		/*read(pipefd[0], &ptr, sizeof(ptr));*/
		msgrcv(msg_id, (struct msgbuf*)&buf, sizeof(char*), 1, 0);
		memcpy(&ptr, buf.mtext, sizeof(char*));
		ptr = smToAbs(r, ptr);
		smFree(ptr);
		}
	    exit(0);
	    }
	else
	    {
	    /** in parent **/
	    close(pipefd[0]);
	    for(i=0;i<iter;i++)
		{
		ptr = smMalloc(r, 1024 + (rand()%1025));
		if (!ptr)
		    {
		    /** memory full; wait 1ms and try again **/
		    i--;
		    usleep(1000);
		    /*write(0,".\b",2);*/
		    continue;
		    }
		ptr = smToRel(r,ptr);
		/*write(pipefd[1], &ptr, sizeof(ptr));*/
		memcpy(buf.mtext, &ptr, sizeof(char*));
		buf.mtype = 1;
		msgsnd(msg_id, (struct msgbuf*)&buf, sizeof(char*), 0);
		}
	    wait(&status);
	    }
	
	smDestroy(r);

	msgctl(msg_id, IPC_RMID, NULL);

    return iter;
    }

