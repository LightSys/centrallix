#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

// edit Makefile.in
// ./configure
// export TONLY=b+tree
// make test 

long long
test(char** tname)
    {
    int i;
    int iter;
	int tmp;

	*tname = "b+tree-00 bptNew <test something> and printf";
	iter = 800000;
	for(i=0;i<iter;i++)
	    {
	    //pBPTree tree = NULL;
		//tree = bptNew();
		//assert (tree != NULL);
		tmp = bpt_i_Push(NULL);
		assert (tmp == 0);
	    }

	//printf("\nhello world\n\n");

    return iter*4;
    }

