#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "Old MoneyType - objDataToDouble 00";

    /** Zero Case **/
    MoneyType test = {0,0};
    assert(objDataToDouble(7,&test) == 0);

    /** Positive Case **/
    test.WholePart = 1;
    test.FractionPart = 0;
    assert(objDataToDouble(7,&test) == 1);

    /** Negative Case **/
    test.WholePart = -1;
    test.FractionPart = 0;
    assert(objDataToDouble(7,&test) == -1);

    /** Fractional Case **/
    test.WholePart = 1;
    test.FractionPart = 5000;
    assert(objDataToDouble(7,&test) == 1.5);

    return 0;
}
