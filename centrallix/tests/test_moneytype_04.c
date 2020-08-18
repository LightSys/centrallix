#include "obj.h"
#include <assert.h>
#include <stdio.h>

long long
test(char** name)
{
    *name = "moneytype_04 - objDataCompare";
    MoneyType test = {70500};

    /** Compare Int with Money Case **/
    int testInt = 6;
    assert(objDataCompare(1,&testInt,7,&test) == -1);
    
    /** Compare Double with Money Case **/
    double testDouble = 5.5;
    assert(objDataCompare(3,&testDouble,7,&test) == -1);
    
    /** Compare String with Money Case **/
    char data_ptr[] = "$4.50";
    assert(objDataCompare(2,data_ptr,7,&test) == -1);
    
    /** Compare Money with Money Case **/
    MoneyType testMoney = {70500};
    assert(objDataCompare(7,&testMoney,7,&test) == 0);

    /** Compare DateTime with Money Case **/
    DateTime testDate = {0};
    assert(objDataCompare(4,&testDate,7,&test) == -2);

    /** Compare IntV with Money Case **/
    IntVec testIV = {0};
    assert(objDataCompare(5,&testIV,7,&test) == -2);

    /** Compare StrV with Money Case **/
    StringVec testSV = {0};
    assert(objDataCompare(6,&testSV,7,&test) == -2);

    return 0;
}
