#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "Old MoneyType - objDataToInteger 00";

    /** Zero Case **/
    MoneyType test = {0,0};
    assert(objDataToInteger(7,&test,NULL) == 0);

    /** Positive Case **/
    test.WholePart = 1;
    test.FractionPart = 0;
    assert(objDataToInteger(7,&test,NULL) == 1);

    /** Negative Case **/
    test.WholePart = -1;
    test.FractionPart = 0;
    assert(objDataToInteger(7,&test,NULL) == -1);

    /** Fractional Case **/
    test.WholePart = 1;
    test.FractionPart = 5000;
    assert(objDataToInteger(7,&test,NULL) == 1);

    return 0;
}
