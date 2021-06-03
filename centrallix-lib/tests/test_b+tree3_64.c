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
<<<<<<< HEAD
    int i, ret, iter;

	*tname = "b+tree3_64 Testing memory leaks: add only";

    iter = 100;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        ret = automatedTree(tree, 1000);
        assert (ret == 0);
        ret = bptFree(tree, free_func, NULL);
        assert(ret == 0);
        }

    //pBPTree tree2 = bptNew();
    //ret = automatedTree(tree2, 1000);
    //assert(ret == 0);

    return iter;
    }

=======
    int i, ret;
	pBPTree tree = bptNew();

	*tname = "b+tree3_64 Testing bptFromLookup on a larger tree";
    //int status = 0;


    ret = automatedTree(tree, 1000);
    assert (ret == 0);

    char* str = "0000000111";
    int len = strlen(str);

    pBPIter iterator = bptFromLookup(tree, 0, str, len);


    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0000000111") == 0);


    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }
>>>>>>> 1b95137a68b4d1b16555f1b8edc492442c04b48e


