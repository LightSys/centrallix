#include <stdio.h>
#include <assert.h>

long long
test(char** name)
    {
    *name = "00baseline_08 Native C Assert ** SHOULD FAIL **";
    assert(0);
    return 0;
    }
