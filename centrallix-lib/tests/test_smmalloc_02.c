#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"

long long
test(char** tname)
    {
    int i;
    pSmRegion r;
    void* ptr;
    int iter;

	smInitialize();

	*tname = "smmalloc-02 malloc/free 1024 bytes";
	iter = 100000;
	r = smCreate(1024*1024);
	for(i=0;i<iter;i++)
	    {
	    ptr = smMalloc(r, 1024);
	    if (!ptr) 
		{
		smDestroy(r);
		return -1;
		}
	    smFree(ptr);
	    }
	smDestroy(r);

    return iter;
    }
