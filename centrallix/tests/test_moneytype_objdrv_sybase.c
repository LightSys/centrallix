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

        /** 8-byte money **/
        int eightByteMoneyType = 11;
        //Sybase 8 byte money is mixed endian: 0 = least significant bit, f = most significant bit
        unsigned long long moneyToConvert = 0x76543210fedcba98;
        assert(sybd_internal_GetCxValue(&moneyToConvert, eightByteMoneyType, &objDataResult, DATA_T_MONEY) == 0);
        //mixed-endianness should be fixed now
        assert(objDataResult.Money->Value == 0xfedcba9876543210);

        /** confirm positive 8-byte money works as well **/
        moneyToConvert = 0xfedcba9876543210;
        assert(sybd_internal_GetCxValue(&moneyToConvert, eightByteMoneyType, &objDataResult, DATA_T_MONEY) == 0);
        assert(objDataResult.Money->Value == 0x76543210fedcba98);

        return 0;

    #else
        return 1;
    #endif
}
