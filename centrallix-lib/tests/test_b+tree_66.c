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

	*tname = "b+tree_66 Testing bptNext from bptFromLookup on a larger tree going backwards";
    int status = 0;


    ret = automatedTree(tree, 1000);
    assert (ret == 0);

    char* str = "0000000351";
    int len = strlen(str);

    pBPIter iterator = bptFromLookup(tree, 1, str, len);

    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0000000351") == 0);

    bptNext(iterator, &status);

    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0000000350") == 0);



    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }


