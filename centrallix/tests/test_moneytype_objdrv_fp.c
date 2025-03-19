#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "osdrivers/objdrv_fp.c"

long long
test(char** name)
{
    *name = "moneytype_00 - objdrv_fp";
    
    //This test is making the assumption that the raw data passed in testString here (row_data in objdrv_fp.c)
    //is in units of dollars (and so dbl_internal_ParseColumn should convert it to 1/10000ths of a dollar when putting
    //it into a MoneyType representation)
    
    FpColInf testColInf;
    pFpColInf pTestColInf = &testColInf;
    
    testColInf.Type = DATA_T_MONEY;
    testColInf.Length = 8;
    testColInf.RecordOffset = 0;
    testColInf.DecimalOffset = 0;
    ObjData moneyData;
    pObjData moneyDataPtr = &moneyData;
    MoneyType testMoneyData = {0};
    pMoneyType pTestMoneyData = &testMoneyData;

    /** No Decimal Offset Case **/
    char* rowData = "450";
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, rowData) == 0);
    assert(moneyDataPtr->Money->Value == 4500000);

    /** Decimal Offset, multiplier > 1 case **/
    testColInf.DecimalOffset = 1;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, rowData) == 0);
    assert(moneyDataPtr->Money->Value == 450000);
    
    /** Decimal Offset, multiplier < 1 case **/
    testColInf.DecimalOffset = 5;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, rowData) == 0);
    assert(moneyDataPtr->Money->Value == 45);
    
    /** Rounding for floating point error handling **/
    char* testRoundedString = "45011999";
    testColInf.DecimalOffset = 5;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, testRoundedString) == 0);
    assert(moneyDataPtr->Money->Value == 4501200);
                 
    return 0;
}
