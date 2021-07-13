#include "config.h"
#ifdef USE_SYBASE
#include "obj.h"
#include <assert.h>
#include "osdrivers/objdrv_sybase.c"
#endif

long long
test(char** name)
{
    *name = "moneytype_00 - objdrv_sybase";

    #ifdef USE_SYBASE
    MoneyType moneyResult;
    ObjData objDataResult;
    objDataResult.Money = &moneyResult;

    /** smallmoney **/
    int smallMoneyType = 21;
    int smallMoneyToConvert = 45000;
    assert(sybd_internal_GetCxValue(&smallMoneyToConvert, smallMoneyType, &objDataResult, DATA_T_MONEY) == 0);
    assert(objDataResult.Money->Value == 45000);

    /** positive 8-byte money **/

    /** negative 8-byte money (in two's complement!) **/

    /** explicit test of bit shifting? 0x44447777 or something **/

    return 0;
    #else
    return 1;
    #endif
}
