//NAME Native C ** SHOULD CRASH **

#include <signal.h>

long long
test(char** name)
    {
    *name = "Native C ** SHOULD CRASH **";
    raise(SIGSEGV);
    return 0;
    }
