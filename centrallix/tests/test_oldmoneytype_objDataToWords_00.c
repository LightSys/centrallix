#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "Old MoneyType - objDataToWords 00";
    MoneyType test = {7, 500};

    /** Positive Case **/
    char* data_ptr = "Seven And 05/100 ";
    char* returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    /** Negative Case **/
    test.WholePart = -8;
    test.FractionPart = 9500;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    /**objDataToWords Truncates fractional cent values past 100ths**/
    /** Fractional Case **/
    test.WholePart = -8;
    test.FractionPart = 9475;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    /** Fractional Case **/
    test.WholePart = -8;
    test.FractionPart = 9425;
    data_ptr = "Negative Seven And 05/100 ";
    returnStr = objDataToWords(7,&test);
    assert(strcmp(data_ptr, returnStr) == 0);

    return 0;
}