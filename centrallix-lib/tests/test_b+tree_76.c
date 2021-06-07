#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"


typedef struct _TEST_STRUCT BPTest, *pBPTest;


struct _TEST_STRUCT
    {
    char* Key;
    void* Ref;
    };

int free_func(void* args, void* ref)
    {
    pBPTest item = ref;
    nmSysFree(item->Key);
    nmFree(item->Ref, sizeof(int));
    nmFree(item, sizeof(BPTest));
	return 0;
    }

long long
test(char** tname)
    {
    int i, ret, iter;
    char* key;
	int len = 10;

	*tname = "b+tree_76 Testing memory leaks: bptIterFree with 1 key tree";

    iter = 100000;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        
        key = nmSysMalloc(len + 1);
        int* data = nmMalloc(sizeof(int));
        *data = 1;
        sprintf(key, "%010d", 1);

        pBPTest stored = nmMalloc(sizeof(BPTest));
        stored->Key = key;
        stored->Ref = data;

        ret = bptAdd(tree, stored->Key, strlen(stored->Key), stored);
        assert(ret == 0);

        pBPIter iterator = bptFront(tree);

        ret = bptIterFree(iterator);
        assert(ret == 0);
        ret = bptFree(tree, free_func, NULL);
        assert(ret == 0);
        }

    return iter;
    }