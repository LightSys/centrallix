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
    
    ////Edge Cases
    //
    ///** Compare Double with Money Case **/
    //double testDouble = 5.5;
    //assert(objDataToMoney(3,&testDouble,&test) == 0);
    //assert(test.Value == 55000);
    //
    ///** Compare String with Money Case **/
    //char data_ptr[] = "$4.50";
    //int testInt = 6;
    //assert(objDataToMoney(1,&testInt,&test) == 0);
    //assert(test.Value == 60000);
    //
    ///** Compare Money with Money Case **/
    //MoneyType testMoney = {900000};
    //assert(objDataToMoney(7,&testMoney,&test) == 0);
    //assert(test.Value == 900000);

    ///** Compare DateTime with Money Case **/
    //MoneyType testMoney = {900000};
    //assert(objDataToMoney(7,&testMoney,&test) == 0);
    //assert(test.Value == 900000);

    ///** Compare IntV with Money Case **/
    //MoneyType testMoney = {900000};
    //assert(objDataToMoney(7,&testMoney,&test) == 0);
    //assert(test.Value == 900000);

    ///** Compare StrV with Money Case **/
    //MoneyType testMoney = {900000};
    //assert(objDataToMoney(7,&testMoney,&test) == 0);
    //assert(test.Value == 900000);

    return 0;
}
