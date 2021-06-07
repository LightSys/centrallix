#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
	return 0;
}

long long
test(char** tname)
    {
    int i, ret;
    int iter;
	pBPTree tree = bptNew();

	*tname = "b+tree_55 Automated mass add functionality";

	assert (bptIsEmpty(tree));
	iter = 2000;
    /*
	innerIter = 100;
    for(j = 0; j < innerIter; j++)
			{
			key = nmSysMalloc(len + 1);
			int* data = nmMalloc(sizeof(int));
			*data = j;
			sprintf(key, "%03d", j);
			ret = bptAdd(tree, key, len, data);
			nmSysFree(key);
			assert(ret == 0);
			}
            */

           ret = automatedTree(tree, 100);
           assert (ret == 0);

            assert(bptSize(tree) == 100);


    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return iter * 2000;
    }



