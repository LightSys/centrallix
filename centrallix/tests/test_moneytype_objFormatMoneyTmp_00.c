#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    /** objFormatMoneyTmp is a wrapper that calls obj_internal_FormatMoney
     ** Functionality was changed in obj_internal_FormatMoney, so the point of
     ** these tests is to test obj_internal_FormatMoney, even with a different function call
     **/
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
    
    return 0;
}
