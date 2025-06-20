#include <signal.h>

long long
test(char** name)
    {
    *name = "00baseline_06 Native C ** SHOULD CRASH **";
    raise(SIGSEGV);
    return 0;
    }
