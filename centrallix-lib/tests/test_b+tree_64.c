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

	*tname = "b+tree_64 Testing memory leaks: add only";

    iter = 100;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        
        int j, amount;
        char* key;
	    int len = 10;

        amount = 1000;
        for (j = 0; j < amount; j++) 
        {
            key = nmSysMalloc(len + 1);
			int* data = nmMalloc(sizeof(int));
			*data = j;
			sprintf(key, "%010d", j);

            pBPTest stored = nmMalloc(sizeof(BPTest));
            stored->Key = key;
            stored->Ref = data;

			ret = bptAdd(tree, stored->Key, strlen(stored->Key), stored);
            if (ret != 0)
                {
                printf("Error in adding\n");
                }
        }

        assert (ret == 0);
        ret = bptFree(tree, free_func, NULL);
        assert(ret == 0);
        }

    return iter;
    }



