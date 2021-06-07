#include <stdio.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc_memfail.h"

int fail_malloc;

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
    return 0;
}

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;


	*tname = "b+tree_50 SHOULD FAIL unless run special; bptNew returns NULL on nmMalloc fail (Remake of original test 42, using the method in original test 39)";
    
    // NOTE: To make this test work, you must recompile to enable mem fail.
	//
	// If you haven't run ./configure since this feature was implemented, run ./configure
	// 
	// To enable the fail_malloc variable (set to 1 to cause nmMalloc to act like it
	// ran out of memory and return null, 0 to cause nmMalloc to run as normal), run:
	// make clean
	// make memfail
	// then run make test as usual (you can run make test multiple times, for example
	// after making changes to your tests, without having to run make clean/memfail again)
	//
	// To return to normal nmMalloc (disbling the fail_malloc variable), run:
	// make clean
	// make

	iter = 8000000;
    
            fail_malloc = 0;
            tree = bptNew();
            assert (tree != NULL);
		    bptFree(tree, free_func, NULL);
		    fail_malloc = 1;
            tree = bptNew();
		    assert (tree == NULL);
		
    int x = 1;
        for(i=0;i<iter;i++)
            {
                x++;
            }
    return iter;
    }



