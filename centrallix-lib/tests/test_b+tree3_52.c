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
    int iter;
	pBPTree tree;


	*tname = "b+tree3_52 bptIsEmpty returns true after creation of tree (Based on original test 54)";

        tree = bptNew();
		
		assert (bptIsEmpty(tree));

        
    int x = 1;
	iter = 10000000;
	for(i=0;i<iter;i++)
	    {
            x++;
		}

    return iter;
    }

