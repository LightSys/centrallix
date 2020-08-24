#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "Old MoneyType - objFormatMoneyTmp 00";
    
    /** Zero Case **/
    MoneyType test = {0, 0};
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$0.00") == 0);
    
    /** Positive Case **/
    test.WholePart = 1;
    test.FractionPart = 0;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$1.00") == 0);

    /** Negative Case **/
    test.WholePart = -1;
    test.FractionPart = 0;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "-$1.00") == 0);

    /** Nonzero Cent Case **/
    test.WholePart = 1;
    test.FractionPart = 5000;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$1.50") == 0);

    /** Fractional Cent Case **/
    test.WholePart = 1;
    test.FractionPart = 20;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$1.00") == 0);
    
    return 0;
}
