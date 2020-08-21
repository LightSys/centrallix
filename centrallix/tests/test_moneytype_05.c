#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "moneytype_05 - objDataToWords";
    MoneyType test = {70500};

    /** Positive Case **/
    char* data_ptr = "Seven And 05/100 ";
    char* returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);
    
    /** Negative Case **/
    test.Value = -70500;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    /**objDataToWords Truncates fractional cent values past 100ths**/
    /** Fractional Case **/
    test.Value = -70525;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    /** Fractional Case **/
    test.Value = -70575;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);
    
    return 0;
}
