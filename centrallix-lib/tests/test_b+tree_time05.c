#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time05 testing time of searching tree size 1000";
	iter = 2000000;
	
	pBPNode this = bptBulkLoad("tests/bpt_bl_10e3.dat", 1000);
	pBPNode dummy = NULL;
	int idx = 0;

	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000090", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000110", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000275", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000290", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000500", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00000509", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000750", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00000900", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00001006", 8, &dummy, &idx);
                assert (tmp == -1);
		}
	
    	return iter;//do I need *4
    	}

