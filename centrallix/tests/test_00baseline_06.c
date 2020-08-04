//NAME Native C ** SHOULD CRASH **

#include <signal.h>

long long
test(void)
    {
    raise(SIGSEGV);
    return 0;
    }
