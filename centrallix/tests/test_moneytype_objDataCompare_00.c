#include "obj.h"
#include <assert.h>
#include <stdio.h>
//#include <string.h>

long long
test(char** name)
{
    *name = "moneytype_00 - objDataCompare";
    MoneyType test = {70000};

    /** Compare Int with Money Case **/
    /** Greater Than **/
    int testInt = 8;
    assert(objDataCompare(DATA_T_INTEGER, &testInt, DATA_T_MONEY, &test) == 1);
    /** Less Than **/
    testInt = 6;
    assert(objDataCompare(DATA_T_INTEGER, &testInt, DATA_T_MONEY, &test) == -1);
    /** Equal To **/
    testInt = 7;
    assert(objDataCompare(DATA_T_INTEGER, &testInt, DATA_T_MONEY, &test) == 0);
    /** Int multiplied to be > INT_MAX **/
    testInt = 2147483647;
    assert(objDataCompare(DATA_T_INTEGER, &testInt, DATA_T_MONEY, &test) == 1);
    
    /** Compare Double with Money Case **/
    /** Greater Than **/
    double testDouble = 7.5;
    assert(objDataCompare(DATA_T_DOUBLE, &testDouble, DATA_T_MONEY, &test) == 1);
    /** Less Than **/
    testDouble = 5.5;
    assert(objDataCompare(DATA_T_DOUBLE, &testDouble, DATA_T_MONEY, &test) == -1);
    /** Equal To **/
    testDouble = 7.0;
    assert(objDataCompare(DATA_T_DOUBLE, &testDouble, DATA_T_MONEY, &test) == 0);
    
    /** Compare String with Money Case **/
    /** Greater Than **/
    char data_ptr[] = "$8.50";
    assert(objDataCompare(DATA_T_STRING, data_ptr, DATA_T_MONEY, &test) == 1);
    /** Less Than **/
    char data_ptr2[] = "$4.50";
    assert(objDataCompare(DATA_T_STRING, data_ptr2, DATA_T_MONEY, &test) == -1);
    /** Equal To **/
    char data_ptr3[] = "$7.00";
    assert(objDataCompare(DATA_T_STRING, data_ptr3, DATA_T_MONEY, &test) == 0);
    
    /** Compare Money with Money Case **/
    /** Greater Than **/
    MoneyType testMoney = {80500};
    assert(objDataCompare(DATA_T_MONEY, &testMoney, DATA_T_MONEY, &test) == 1);
    /** Less Than **/
    testMoney.Value = 50000;
    assert(objDataCompare(DATA_T_MONEY, &testMoney, DATA_T_MONEY, &test) == -1);
    /** Equal To **/
    testMoney.Value = 70000;
    assert(objDataCompare(DATA_T_MONEY, &testMoney, DATA_T_MONEY, &test) == 0);

    /** Compare DateTime with Money Case **/
    DateTime testDate = {0};
    assert(objDataCompare(DATA_T_DATETIME, &testDate, DATA_T_MONEY, &test) == -2);

    /** Compare IntV with Money Case **/
    /** IntV fails with any value other than 2 **/
    IntVec testIV = {0};
    assert(objDataCompare(DATA_T_INTVEC, &testIV, DATA_T_MONEY, &test) == -2);

    int* argArray[2];
    IntVec validTestIV = {0};
    validTestIV.nIntegers = 2;
    validTestIV.Integers = argArray;
    validTestIV.Integers[0] = 9;
    validTestIV.Integers[1] = 20;
    assert(objDataCompare(DATA_T_INTVEC, &validTestIV, DATA_T_MONEY, &test) == 1);

    validTestIV.Integers[0] = 4;
    validTestIV.Integers[1] = 35;
    assert(objDataCompare(DATA_T_INTVEC, &validTestIV, DATA_T_MONEY, &test) == -1);

    validTestIV.Integers[0] = 7;
    validTestIV.Integers[1] = 0;
    assert(objDataCompare(DATA_T_INTVEC, &validTestIV, DATA_T_MONEY, &test) == 0);

    /** Compare StrV with Money Case **/
    StringVec testSV = {0};
    assert(objDataCompare(DATA_T_STRINGVEC, &testSV, DATA_T_MONEY, &test) == -2);

    return 0;
}
