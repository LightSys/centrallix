#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time00 baseline time of making tree size 10";
	iter = 4000000;
	
	pBPNode this = bptBulkLoad("tests/bpt_bl_10e1.dat", 10);
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

