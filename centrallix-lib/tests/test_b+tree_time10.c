#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time10 baseline time of making tree size 1000000";
	iter = 50000;
	
	pBPNode this = bptBulkLoad("tests/bpt_bl_10e6.dat", 1000000);
	pBPNode dummy = NULL;
	tmp = 0;
	
		
	for(i=0;i<iter;i++)
	 	{
		assert (tmp == 0);
		assert (tmp == 0);
		assert (tmp == 0);
                assert (tmp == 0);   
		assert (tmp == 0); 
		assert (tmp == 0);
		assert (tmp == 0);     
		assert (tmp == 0); 
		assert (tmp == 0); 
		assert (tmp == 0);
		}
	
    	return iter;//do I need *4
    	}

