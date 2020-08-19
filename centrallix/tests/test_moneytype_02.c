#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "moneytype_02 - objDataToDouble";
    double testValue;

    /** Zero Case **/
    MoneyType test = {0};
    testValue = objDataToDouble(7,&test);
    assert(testValue == 0.0);

    /** Positive Case **/
    test.Value = 10000;
    testValue = objDataToDouble(7,&test);
    assert(testValue == 1.0);

    /** Negative Case **/
    test.Value = -10000;
    testValue = objDataToDouble(7,&test);
    assert(testValue == -1.0);

    /** Rational Case **/
    test.Value = -25000;
    testValue = objDataToDouble(7,&test);
    assert(testValue == -2.5);
    
    return 0;
}
