#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time09 testing time of searching tree size 100000";
	iter = 50000;
	
	pBPTree this = bptBulkLoad("tests/bpt_bl_10e5.dat", 100000);
	pBPTree dummy = NULL;
	int idx = 0;

	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00001111", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00020000", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00034567", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00040288", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00052394", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00060000", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00075000", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00090002", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00100001", 8, &dummy, &idx);
                assert (tmp == -1);
		}
	
    	return iter;//do I need *4
    	}

