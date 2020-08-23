#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

// dummy free function
int bpt_dummy_freeFn(void* arg, void* ptr) {
    free(ptr);
    return 0;
}

long long
test(char** tname)
   	{
	*tname = "b+tree2-01 TEST NEW VERSION";
	// TODO: This is the entire random insert/remove test that was being run while developing. Probably not appropriate as a unit test, so needs refactoring to test individual components and use asserts rather than print.
	
	//
	int seed = time(0);
    srand(seed);
    printf("\nSeed: %d\n", seed);
    BPTree* t = bptNew();
    //bpt_PrintTree(t->root, 0);
    int val;
    char* k;
    char* d;
    int i;
    
    // Change this to any power of 10 (>= 10; 100000+ may take a long time)
    int KEY_MAX = 1000;

    int NUM_TESTS = rand() % (int)(KEY_MAX * 0.7);//20;
    printf("%d tests, numbers 0 to %d\n", NUM_TESTS, KEY_MAX-1);
    char* FMT = malloc(10);
    char* VAL_FMT = malloc(10);
    int NUM_DIGITS = 0;
    i=KEY_MAX;
    while (i >= 10) 
        {
        NUM_DIGITS++;
        i /= 10;
        }
    sprintf(FMT, "%%0%dd", NUM_DIGITS);
    sprintf(VAL_FMT, "Val%%0%dd", NUM_DIGITS);

    for (i=0; i<NUM_TESTS; i++) 
        {
        val = rand() % KEY_MAX;
        k = malloc(sizeof(char)*(NUM_DIGITS+1));
        sprintf(k, FMT, val);
        d = malloc(sizeof(char)*(NUM_DIGITS+4));
        sprintf(d, VAL_FMT, val);
        if (bptInsert(t, k, strlen(k)+1, d) < 0) free (d);  // free data if insert failed because that key already existed
        
        if (testTree(t) < 0)
            {
            printf("Inserting %s\t", k);
            printf("Failed tree:\n");
            bpt_PrintTree(t->root, 0);
            break;
            }
        //else printf("Passed\n");

        free(k);
        }
    printf("Passed inserts\n");
    //bpt_PrintTree(t->root, 0);
    
    while (t->root->nKeys > 0) //for (i=NUM_TESTS; i>0 && t->root->nKeys > 0; i--) 
        {
        val = rand() % KEY_MAX;
        k = malloc(sizeof(char)*(NUM_DIGITS+1));
        sprintf(k, FMT, val);
        bptRemove(t, k, strlen(k)+1, bpt_dummy_freeFn, NULL);
        
        if (testTree(t) < 0)
            {
            printf("Removing %s\t", k);
            printf("Failed tree:\n");
            bpt_PrintTree(t->root, 0);
            printf("\nSeed: %d\n", seed);
            break;
            }
        else 
            {
            //printf("Passed\n");
            //bpt_PrintTree(t->root, 0);
            }
        free(k);
        }
    printf("Passed remove\n");

    bpt_PrintTree(t->root, 0);  // tree should be empty

    nmFree(t->root, sizeof(BPNode));
    free(t);
    free(FMT);
    free(VAL_FMT);

	
    return 10000000000;
   	}



