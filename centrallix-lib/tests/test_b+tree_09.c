#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	int val;

	*tname = "b+tree-09 bptDeInit returns -1 if passed NULL";

	iter = 8000000;
	for(i=0;i<iter;i++)
	 	{
		val = bptDeInit(NULL);
		assert (val == -1);	
		}
    return iter;
    }


