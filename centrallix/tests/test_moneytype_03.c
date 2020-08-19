#include "obj.h"
#include <assert.h>
#include <stdio.h>

long long
test(char** name)
{
    *name = "moneytype_03 - objDataToMoney";
    MoneyType test = {0};

    /** String Case **/
    char data_ptr[] = "$4.50";
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == 45000);

    /** Double Case **/
    double testDouble = 5.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == 55000);
    
    /** Int Case **/
    int testInt = 6;
    assert(objDataToMoney(1,&testInt,&test) == 0);
    assert(test.Value == 60000);
    
    /** Money Case **/
    MoneyType testMoney = {900000};
    assert(objDataToMoney(7,&testMoney,&test) == 0);
    assert(test.Value == 900000);

    /** Overflow Case (intval > LL max) **/
    char overflow_ptr[] = "$10000000000000000000.50";
    assert(objDataToMoney(2,overflow_ptr,&test) == -1);
    
    return 0;
}
