#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, j, iter, tmp;

	*tname = "b+tree-time00 baseline time of making tree size 10";
	iter = 250000;
		
    int as;

    int size = 10;

    int* info[size];
		
	for(j=0;j<iter;j++)
	 	{
		pBPTree tree = bptNew();

		for(i = 0; i < size; i++) {
			info[i] = nmMalloc(sizeof(int));
			*info[i] = i + 100;
			char k[12];
			sprintf(k, "%03d", i);
			as = bptAdd(tree, k, 11, info[i]);
			assert(as == 0);
		}	
		
		assert(bptSize(tree) == 10);
		}
	
    	return iter * size;
    }

