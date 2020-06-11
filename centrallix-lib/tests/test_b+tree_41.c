#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int iter, i, tmp;
	
	*tname = "b+tree-40 Analyze depth of tree";
	iter = 8000000;

	pBPTree this1 = bptBulkLoad("tests/bpt_bl_10e1.dat", 10);
	pBPTree this2 = bptBulkLoad("tests/bpt_bl_10e2.dat", 100);
	pBPTree this3 = bptBulkLoad("tests/bpt_bl_10e3.dat", 1000);
	pBPTree this4 = bptBulkLoad("tests/bpt_bl_10e4.dat", 10000);
	pBPTree this5 = bptBulkLoad("tests/bpt_bl_10e5.dat", 100000);
	pBPTree this6 = bptBulkLoad("tests/bpt_bl_10e6.dat", 1000000);
	
	
	
	int idx;
	pBPTree locate;
	for(i=0;i<iter;i++)
	 	{
		
		}

	printf("\n");
	
    	return iter*4;
    	}

