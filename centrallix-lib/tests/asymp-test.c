#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
	return 0;
}

int main()
{
    /*
    int i;
    time_t seconds1, seconds2;
    //char* name;
    int times;
    times = 10000000;
    //x = 100;
    pBPTree tree = bptNew();
    automatedTree(tree, 10000);
    int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

    seconds1 = time(NULL);
    //clock_t CPU_time_1 = clock();
    //printf("Time 1: %d \n", CPU_time_1);

    for(i = 0; i < times; i++) {
        bptRemove(tree, "0000000000", 10, free_func, NULL);
    }

    //clock_t CPU_time_2 = clock();
    seconds2 = time(NULL);

    //printf("Time 2: %d\n", CPU_time_2);

    //printf("Final time: %ld\n", (CPU_time_2 - CPU_time_1));
    printf("Final time: %ld\n", (seconds2 - seconds1));
*/

    time_t seconds1, seconds2;
    int i, j;
    int iter;
	pBPTree tree = bptNew();
    automatedTree(tree, 10000000);
    
	iter = 1000;

    //seconds1 = time(NULL);
    clock_t CPU_time_1 = clock();
	for(i=0;i<iter;i++) {
        for (j = 0; j < iter; j++) {
            bptLookup(tree, "1111111111", 10);
        }
		
    }
    //seconds2 = time(NULL);
    clock_t CPU_time_2 = clock();


    //printf("Elapsed time: %ld\n", seconds2 - seconds1);
    
    printf("Elapsed time: %ld\n", CPU_time_2 - CPU_time_1);
    return 0;
}
