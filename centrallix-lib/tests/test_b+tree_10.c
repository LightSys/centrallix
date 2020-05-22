#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;

	*tname = "b+tree-10 bptFree returns without crash if passed NULL";

	iter = 8000000;
	for(i=0;i<iter;i++)
	 	{
		bptFree(NULL);
		}
    return iter;
    }




