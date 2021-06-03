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

	*tname = "b+tree3_59 Testing bptFront on a larger tree";
    //int status = 0;


    ret = automatedTree(tree, 1000);
    assert (ret == 0);

    char* str = "0";
    int len = strlen(str);

    pBPIter iter2 = bptFront(tree);

    printf("%s\n", iter2->Curr->Keys[iter2->Index].Value);

    printf("test1\n");
    pBPIter iterator = bptFromLookup(tree, 0, str, len);
    printf("test2\n");
    assert(iterator == NULL);


    //assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "00000000000"));
    //printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
    //assert(iterator->Ref == bptLookup(tree, str, strlen(str)));
    printf("test3\n");



    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }



