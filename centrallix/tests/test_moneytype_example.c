#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "MoneyType example";
    
    MoneyType test = {10000};
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$1.00") == 0);
    
    return 0;
}
