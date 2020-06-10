#include <stdio.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc_memfail.h" // enables mem fail

int fail_malloc;	// you must declare this if using newmalloc_memfail.h

long long
test(char** tname)
    {
    int i;
    int iter;
	void* ptr;

	*tname = "b+tree-39 DEMO making nmMalloc fail";

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

	iter = 800000;
	for(i=0;i<iter;i++)
	    {
		fail_malloc = 0;
		ptr = nmMalloc(5);
		assert (ptr != NULL);
		nmFree(ptr, 5);
		fail_malloc = 1;
		ptr = nmMalloc(5);
		assert (ptr == NULL);

		}

    return iter;
    }


