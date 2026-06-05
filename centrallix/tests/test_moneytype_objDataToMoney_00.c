#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

long long
test(char** name)
{
    *name = "moneytype_00 - objDataToMoney";
    MoneyType test = {0};

    /** String Case **/
    char data_ptr[20] = "$4.50";
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == 45000);

    strcpy(data_ptr, "-$4.50");
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == -45000);

    strcpy(data_ptr, "-$0.01");
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == -100);
    
    /** String Case that would overflow INT_MAX and INT_MIN **/
    strcpy(data_ptr, "$2200000000.00");
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == 22000000000000);

    strcpy(data_ptr, "-$2200000000.00");
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.Value == -22000000000000);
    
    /** Overflow Case (intval > LL max) **/
    char overflow_ptr[] = "$10000000000000000000.50";
    assert(objDataToMoney(2,overflow_ptr,&test) == -1);

    /** Double Case **/
    double testDouble = 5.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == 55000);

    testDouble = -5.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == -55000);
    
    /** Double Case that would overflow INT_MAX and INT_MIN **/
    testDouble = 2200000000.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == 22000000005000);

    testDouble = -2200000000.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == -22000000005000);
    
    /** Double Fraction Overflow **/
    testDouble = 5.159999999999;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.Value == 51600);
    
    /** Int Case **/
    int testInt = 6;
    assert(objDataToMoney(1,&testInt,&test) == 0);
    assert(test.Value == 60000);
    
    /** Money Case **/
    MoneyType testMoney = {900000};
    assert(objDataToMoney(7,&testMoney,&test) == 0);
    assert(test.Value == 900000);

    
    
    return 0;
}
