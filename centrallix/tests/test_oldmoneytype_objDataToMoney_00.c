#include "obj.h"
#include <assert.h>
#include <stdio.h>

long long
test(char** name)
{
    *name = "Old MoneyType - objDataToMoney 00";
    MoneyType test = {0,0};

    /** String Case **/
    char data_ptr[] = "$4.50";
    assert(objDataToMoney(2,data_ptr,&test) == 0);
    assert(test.WholePart == 4);
    assert(test.FractionPart == 5000);

    char data_ptr2[] = "-$4.50";
    assert(objDataToMoney(2,data_ptr2,&test) == 0);
    assert(test.WholePart == -5);
    assert(test.FractionPart == 5000);

    /** Overflow Case (intval > LL max) **/
    char overflow_ptr[] = "$10000000000000000000.50";
    assert(objDataToMoney(2,overflow_ptr,&test) == -1);

    /** Double Case **/
    double testDouble = 5.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.WholePart == 5);
    assert(test.FractionPart == 5000);

    testDouble = -5.5;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.WholePart == -6);
    assert(test.FractionPart == 5000);

    //Double Fraction Overflow (this one rounds)
    testDouble = 5.159999999999;
    assert(objDataToMoney(3,&testDouble,&test) == 0);
    assert(test.WholePart == 5);
    assert(test.FractionPart == 1600);

    /** Int Case **/
    int testInt = 6;
    assert(objDataToMoney(1,&testInt,&test) == 0);
    assert(test.WholePart == 6);
    assert(test.FractionPart == 0);

    /** Money Case **/
    MoneyType testMoney = {90,0};
    assert(objDataToMoney(7,&testMoney,&test) == 0);
    assert(test.WholePart == 90);
    assert(test.FractionPart == 0);

    return 0;
}