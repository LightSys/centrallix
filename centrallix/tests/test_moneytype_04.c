#include "obj.h"
#include <assert.h>
#include <stdio.h>
//#include <string.h>

long long
test(char** name)
{
    *name = "moneytype_04 - objDataCompare";
    MoneyType test = {70000};

    /** Compare Int with Money Case **/
    /** Greater Than **/
    int testInt = 8;
    assert(objDataCompare(1,&testInt,7,&test) == 1);
    /** Less Than **/
    testInt = 6;
    assert(objDataCompare(1,&testInt,7,&test) == -1);
    /** Equal To **/
    testInt = 7;
    assert(objDataCompare(1,&testInt,7,&test) == 0);
    
    /** Compare Double with Money Case **/
    /** Greater Than **/
    double testDouble = 7.5;
    assert(objDataCompare(3,&testDouble,7,&test) == 1);
    /** Less Than **/
    testDouble = 5.5;
    assert(objDataCompare(3,&testDouble,7,&test) == -1);
    /** Equal To **/
    testDouble = 7.0;
    assert(objDataCompare(3,&testDouble,7,&test) == 0);
    
    /** Compare String with Money Case **/
    /** Greater Than **/
    char data_ptr[] = "$8.50";
    assert(objDataCompare(2,data_ptr,7,&test) == 1);
    /** Less Than **/
    char data_ptr2[] = "$4.50";
    assert(objDataCompare(2,data_ptr2,7,&test) == -1);
    /** Equal To **/
    char data_ptr3[] = "$7.00";
    assert(objDataCompare(2,data_ptr3,7,&test) == 0);
    
    /** Compare Money with Money Case **/
    /** Greater Than **/
    MoneyType testMoney = {80500};
    assert(objDataCompare(7,&testMoney,7,&test) == 1);
    /** Less Than **/
    testMoney.Value = 50000;
    assert(objDataCompare(7,&testMoney,7,&test) == -1);
    /** Equal To **/
    testMoney.Value = 70000;
    assert(objDataCompare(7,&testMoney,7,&test) == 0);

    /** Compare DateTime with Money Case **/
    DateTime testDate = {0};
    assert(objDataCompare(4,&testDate,7,&test) == -2);

    /** Compare IntV with Money Case **/
    /** IntV fails with any value other than 2 **/
    IntVec testIV = {0};
    assert(objDataCompare(5,&testIV,7,&test) == -2);

    int* argArray[2];
    IntVec validTestIV = {0};
    validTestIV.nIntegers = 2;
    validTestIV.Integers = argArray;
    validTestIV.Integers[0] = 9;
    validTestIV.Integers[1] = 20;
    assert(objDataCompare(5,&validTestIV,7,&test) == 1);

    validTestIV.Integers[0] = 4;
    validTestIV.Integers[1] = 35;
    assert(objDataCompare(5,&validTestIV,7,&test) == -1);

    validTestIV.Integers[0] = 7;
    validTestIV.Integers[1] = 0;
    assert(objDataCompare(5,&validTestIV,7,&test) == 0);

    /** Compare StrV with Money Case **/
    StringVec testSV = {0};
    assert(objDataCompare(6,&testSV,7,&test) == -2);

    return 0;
}
