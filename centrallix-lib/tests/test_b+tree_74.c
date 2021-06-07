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
    int i, ret, ret2;
	pBPTree tree = bptNew();

	*tname = "b+tree_74 Testing bptRemove to try to get untested code";
    int status = 0;


    ret = automatedTree(tree, 500);
    assert (ret == 0);

    //printTree(tree->root);

    //printf("\n\n\n");

    ret2 = bptRemove(tree, "0000000048", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000058", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000076", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000016", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000092", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000035", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000005", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000019", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000049", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000041", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000089", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000070", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000128", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000192", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000256", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000320", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000008", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000120", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000184", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000248", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000015", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000024", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000056", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000112", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000176", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000240", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000312", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000384", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000014", 10, free_func, NULL);

    ret2 = bptRemove(tree, "0000000025", 10, free_func, NULL);

    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }


