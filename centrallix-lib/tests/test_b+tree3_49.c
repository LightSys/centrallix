#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int tmp;
	*tname = "b+tree3_49 Full test of bpt_i_Compare (Derived from original test 24)";

		tmp = bpt_i_Compare("", 0, "", 0);
		    assert (tmp == 0);
	    tmp = bpt_i_Compare("HELLO", 5, "HELLO", 5);
            assert (tmp == 0);
		tmp = bpt_i_Compare("Tommy O'Brien", 13, "Tommy O'Brien", 13);
            assert (tmp == 0);
	    tmp = bpt_i_Compare("2", 1, "2", 1);
            assert (tmp == 0);
	    tmp = bpt_i_Compare("!@#", 3, "!@#", 3);
            assert (tmp == 0);
	    tmp = bpt_i_Compare("peanut", 6, "peanut", 6);
            assert (tmp == 0);
	
		tmp = bpt_i_Compare("123", 3, "abc", 3);  
            assert (tmp < 0);
		tmp = bpt_i_Compare("", 0, "abc", 3);
            assert (tmp < 0);
		tmp = bpt_i_Compare("Mango", 5, "mango", 5);
            assert (tmp < 0);
		tmp = bpt_i_Compare("Will", 4, "William", 7);
            assert (tmp < 0); 
		tmp = bpt_i_Compare("credit", 6, "debit", 5);
            assert (tmp < 0); 
		tmp = bpt_i_Compare("hijklmmopqrs", 12, "hijklmnopqrs", 12);
            assert (tmp < 0); 

		tmp = bpt_i_Compare("789", 3, "788", 3);
            assert (tmp > 0);
		tmp = bpt_i_Compare("A", 1, "", 0);
            assert (tmp > 0); 
		tmp = bpt_i_Compare("William", 7, "Will", 4);
		    assert (tmp > 0);
		tmp = bpt_i_Compare("tommy", 5, "thomas", 6);
		    assert (tmp > 0);
		tmp = bpt_i_Compare("!!!!", 4, "!!", 2); 
		    assert (tmp > 0); 
		tmp = bpt_i_Compare("hijklmnopqrs", 12, "hijklmmopqrs", 12);
		    assert (tmp > 0); 

         //This next part is just to avoid a floating point error
        int i;
        int x;
        x = 1;
        for (i = 0; i < 10000000; i++) {
            x++;
        }


	
    	return 10;
    	}

