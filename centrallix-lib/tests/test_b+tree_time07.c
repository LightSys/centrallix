#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time07 testing time of searching tree size 10000";
	iter = 500000;
	
	pBPTree this = bptBulkLoad("tests/bpt_bl_10e4.dat", 10000);
	pBPTree dummy = NULL;
	int idx = 0;

	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000450", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000462", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00003500", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00005000", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00006783", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00007500", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00008642", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00009500", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00010001", 8, &dummy, &idx);
                assert (tmp == -1);
		}
	
    	return iter;//do I need *4
    	}

