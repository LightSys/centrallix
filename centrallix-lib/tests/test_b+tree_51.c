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


	*tname = "b+tree_51 bptSize returns 0 after creation of tree";

		tree = bptNew();
		
		assert (bptSize(tree) == 0);

        
	iter = 10000000;
    int x = 1;
	for(i=0;i<iter;i++)
	    {
            x++;
        }
		

    return iter;
    }
