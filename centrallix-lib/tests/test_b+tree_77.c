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
    int i, j, k, ret, ret2;
	pBPTree tree = bptNew();

	*tname = "b+tree_77 Testing inserts for cascading splits, 2 levels";
    int status = 0;
    
    //Note: this is empirically based on the current implementation of the tree, a branching factor of 8, AND very messy
    i = 0;
    //for(i = 0; i < 16; i++)
        {
        for(j = 0; j < 16; j++)
            {
            for(k = 0; k < 13; k++)
                {
                char* key = nmSysMalloc(11);
                int* data = nmMalloc(sizeof(int));
                *data = k*256 + j*16 + i*1;
                sprintf(key, "%010d", *data);
                ret = bptAdd(tree, key, 10, data);
                assert (ret == 0);
                }
            }
        }
    
    for(i = 0; i < 8; i++)
        {
        char* key = nmSysMalloc(11);
        int* data = nmMalloc(sizeof(int));
        *data = 369+i;
        sprintf(key, "%010d", *data);
        ret = bptAdd(tree, key, 10, data);
        assert (ret == 0);

        *data = 641+i;
        sprintf(key, "%010d", *data);
        ret = bptAdd(tree, key, 10, data);
        assert (ret == 0);
        }

    for(i = 0; i < 7; i++)
        {
        char* key = nmSysMalloc(11);
        int* data = nmMalloc(sizeof(int));
        *data = 513+i;
        sprintf(key, "%010d", *data);
        ret = bptAdd(tree, key, 10, data);
        assert (ret == 0);
        }

    for(i = 0; i < 6; i++)
        {
        char* key = nmSysMalloc(11);
        int* data = nmMalloc(sizeof(int));
        *data = 801+i;
        sprintf(key, "%010d", *data);
        ret = bptAdd(tree, key, 10, data);
        assert (ret == 0);
        }
    
    char* key = nmSysMalloc(11);
    int* data = nmMalloc(sizeof(int));
    *data = 1025;
    sprintf(key, "%010d", *data);
    ret = bptAdd(tree, key, 10, data);
    assert (ret == 0);

    key = nmSysMalloc(11);
    data = nmMalloc(sizeof(int));
    *data = 1026;
    sprintf(key, "%010d", *data);
    ret = bptAdd(tree, key, 10, data);
    assert (ret == 0);

    key = nmSysMalloc(11);
    data = nmMalloc(sizeof(int));
    *data = 1265;
    sprintf(key, "%010d", *data);
    ret = bptAdd(tree, key, 10, data);
    assert (ret == 0);

    key = nmSysMalloc(11);
    data = nmMalloc(sizeof(int));
    *data = 3313;
    sprintf(key, "%010d", *data);
    ret = bptAdd(tree, key, 10, data);
    assert (ret == 0);

    for(i = 0; i < 8; i++)
        {
        char* key = nmSysMalloc(11);
        int* data = nmMalloc(sizeof(int));
        *data = 3185+i;
        sprintf(key, "%010d", *data);
        bpt_i_InsertNonfull(tree->root->Children[14].Child, key, 10, data);
        }

    for(i = 0; i < 7; i++)
        {
        char* key = nmSysMalloc(11);
        int* data = nmMalloc(sizeof(int));
        *data = 3313+i;
        sprintf(key, "%010d", *data);
        bpt_i_InsertNonfull(tree->root->Children[15].Child, key, 10, data);
        }
    printf("Full 2-level Tree:\n");
    printTree(tree->root);
    printf("\n");

    key = nmSysMalloc(11);
    data = nmMalloc(sizeof(int));
    *data = 3325;
    sprintf(key, "%010d", *data);
    ret = bptAdd(tree, key, 10, data);
    assert (ret == 0);
    
    printf("Fresh 3-level tree:\n");
    printTree(tree->root);
    printf("\n");

    /*
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
    */
    int x = 1;
	for(i=0;i<100001100;i++)
	    {
        x++;
		
		}
    return 2000;
    }


