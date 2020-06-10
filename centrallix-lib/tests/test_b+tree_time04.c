#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time04 baseline time of making tree size 1000";
	iter = 2000000;
	
	pBPTree this = bptBulkLoad("tests/bpt_bl_10e3.dat", 1000);
	pBPTree dummy = NULL;
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

