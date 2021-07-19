#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "moneytype_00 - objFormatMoneyTmp";
    
    /** 0 Case **/
    MoneyType test = {0};
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$0.00") == 0);

    /** Positive Case **/
    test.Value = 70000;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$7.00") == 0);

    /** Negative Case **/
    test.Value = -70000;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "-$7.00") == 0);

    /** Nonzero Cent Case **/
    test.Value = -75000;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "-$7.50") == 0);

    /** Fractional Cent Case **/
    test.Value = -70001;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "-$7.00") == 0);

    /** Whole Part > INT_MAX **/
    test.Value = 1836475854449306500;
    assert(strcmp(objFormatMoneyTmp(&test, NULL), "$183647585444930.65") == 0);
    
    return 0;
}
