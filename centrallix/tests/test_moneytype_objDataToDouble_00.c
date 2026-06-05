#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "moneytype_00 - objDataToDouble";

    /** Zero Case **/
    MoneyType test = {0};
    assert(objDataToDouble(7,&test) == 0.0);

    /** Positive Case **/
    test.Value = 10000;
    assert(objDataToDouble(7,&test) == 1.0);

    /** Negative Case **/
    test.Value = -10000;
    assert(objDataToDouble(7,&test) == -1.0);

    /** Rational Case **/
    test.Value = -25000;
    assert(objDataToDouble(7,&test) == -2.5);
    
    return 0;
}
