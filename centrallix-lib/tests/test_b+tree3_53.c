#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"


long long
test(char** tname)
    {
    int i;
	int t;
    int iter;
	pBPTree tree = bptNew();
	char key[] = "-1\0";


	*tname = "b+tree3_53 bptIsEmpty updates when inserting (Based on original test 55)";

	assert(bptIsEmpty(tree));
    
        sprintf(key, "%d", i);
		t = bptAdd(tree, key, 5, &i);
		assert(t==0);
		assert(!bptIsEmpty(tree));

    int x = 1;
	iter = 10000000;
	for(i=0;i<iter;i++)
	    {	
            x++;
		}

    return iter;
    }
