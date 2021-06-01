
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
    return 0;
}

long long
test(char** tname)
   	{
    *tname = "b+tree3_36 Test bptFront on a big tree with multiple nodes";

    int i;

    int* status = 0;

   pBPTree tree = bptNew();

    

    int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

    int *info2 = nmMalloc(sizeof(int));
    *info2 = 20;

    int *info3 = nmMalloc(sizeof(int));
    *info3 = 30;

    int *info4 = nmMalloc(sizeof(int));
    *info4 = 40;

    int *info5 = nmMalloc(sizeof(int));
    *info5 = 50;

    int *info6 = nmMalloc(sizeof(int));
    *info6 = 60;

    int *info7 = nmMalloc(sizeof(int));
    *info6 = 70;

    int *info8 = nmMalloc(sizeof(int));
    *info6 = 80;

    int *info9 = nmMalloc(sizeof(int));
    *info6 = 90;

    int *info10 = nmMalloc(sizeof(int));
    *info6 = 100;

    bptAdd(tree, "0001", 4, info1);
    bptAdd(tree, "0002", 4, info2);
    bptAdd(tree, "0003", 4, info3);
    bptAdd(tree, "0004", 4, info4);
    bptAdd(tree, "0005", 4, info5);
    bptAdd(tree, "0006", 4, info6);
    bptAdd(tree, "0007", 4, info7);
    bptAdd(tree, "0008", 4, info8);
    bptAdd(tree, "0009", 4, info9);
    bptAdd(tree, "0010", 4, info10);
    
    pBPIter iterator = bptFront(tree);

    assert(iterator->Ref == info1);
    
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0001") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0002") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0003") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0004") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0005") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0006") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0007") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0008") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0009") == 0);
    printf("%d\n", iterator->Index);
    bptNext(iterator, status);
    assert(strcmp(iterator->Curr->Keys[iterator->Index].Value, "0010") == 0);
    printf("%d\n", iterator->Index);

    //This next part is just to avoid a floating point error
    
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }



    return 10;
   	}
