#include <stdio.h>
#include <string.h>
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
	pBPTree tree = bptNew();

	*tname = "b+tree_62 Testing bptNext on a larger tree from the back";

    int status = 0;


    ret = automatedTree(tree, 1000);
    assert (ret == 0);

    pBPIter iterator = bptBack(tree);
    
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0000000999") == 0);
    bptNext(iterator, &status);
    assert(status ==0);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0000000998") == 0);

    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }



