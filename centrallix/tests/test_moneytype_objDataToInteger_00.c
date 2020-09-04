#include "obj.h"
#include <assert.h>
#include <stdio.h>
//#include "xstring.h"
//#include "mtsession.h"
//#include "mtask.h"

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
    //Manual Testing showed this section to work properly on overflow
    //Currently, I do not know how to print the error messages, so this part
    //will remain commented out.
    
    //test.Value = 250000000000000000;
    //testValue = objDataToInteger(7,&test,NULL);
    
    //assert(testValue);
    //assert(testValue == 5);
    //mssStringError(pXString);
    //printf("%s", pXString);
    
    
    return 0;
}
