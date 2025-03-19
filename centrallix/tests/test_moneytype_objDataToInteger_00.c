#include <assert.h>
#include <stdio.h>
#include "obj.h"
#include "testhelpers/mssErrorHelper.h"

long long
test(char** name)
{
    *name = "moneytype_00 - objDataToInteger";

    /** Zero Case **/
    MoneyType test = {0};
    assert(objDataToInteger(7,&test,NULL) == 0);
    
    /** Positive Case **/
    test.Value = 10000;
    assert(objDataToInteger(7,&test,NULL) == 1);

    /** Negative Case **/
    test.Value = -10000;
    assert(objDataToInteger(7,&test,NULL) == -1);
    
    /** Fractional Case **/
    test.Value = 15000;
    assert(objDataToInteger(7,&test,NULL) == 1);

    /** Overflow Case **/
    mssErrorHelper_init();
    test.Value = 250000000000000000;
    objDataToInteger(7,&test,NULL);
    assert(mssErrorHelper_mostRecentErrorContains(
        "OBJ: Warning: 7 (MoneyType) overflow; cannot fit value of that size in int"));

    return 0;
}
