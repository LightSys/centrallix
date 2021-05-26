#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time11 testing time of searching tree size 1000000";
	iter = 50000;
	
	pBPNode this = bptBulkLoad("tests/bpt_bl_10e6.dat", 1000000);
	pBPNode dummy = NULL;
	int idx = 0;

	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00010000", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00100000", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00200519", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00369412", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00369422", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00500000", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00750000", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00900008", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "01000001", 8, &dummy, &idx);
                assert (tmp == -1);
		}
	
    	return iter;//do I need *4
    	}

