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
    int i, j, ret;
    int iter;
	pBPTree tree = bptNew();
	char* key;
	int len = 10;
	int status = 0;

	*tname = "b+tree-58 add and remove many items from the tree";

	assert (bptIsEmpty(tree));
	iter = 2000;
	for(i=0;i<iter;i++)
	    {	
		for(j = 0; j < 100; j++)
			{
			key = nmSysMalloc(len + 1);
			int* data = nmMalloc(sizeof(int));
			*data = j;
			sprintf(key, "%03d", j);
			ret = bptAdd(tree, key, len, data);
			nmSysFree(key);
			assert(ret == 0);
			}
	
	pBPIter iterator = bptFront(tree);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);
	bptNext(iterator, &status);
	printf("%s\n", iterator->Curr->Keys[iterator->Index].Value);


		for(j = 0; j < 100; j++)
			{
			printf("Iteration %d\n", j);
			key = nmSysMalloc(len + 1);
			if (j == 86) {
				printf("test1\n");
			}
			sprintf(key, "%03d", j);
			if (j == 86) {
				printf("test2\n");
			}
			ret = bptRemove(tree, key, len, free_func, NULL);
			if (j == 86) {
				printf("test3\n");
			}
			nmSysFree(key);
			assert(ret == 0);
			}
		}
    return iter * 2000;
    }



