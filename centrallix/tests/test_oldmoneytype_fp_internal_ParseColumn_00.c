#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "osdrivers/objdrv_fp.c"

long long
test(char** name)
{
    *name = "oldmoneytype_00 - fp_internal_ParseColumn";

    //This Test is making the assumption that the raw data passed in testString here (row_data in objdrv_fp.c)
    //is in units dollars.  So, the function now converts it to 1/10000ths like the new moneyType requires.

    FpColInf testColInf = {0};
    pFpColInf pTestColInf = &testColInf;

    //7 for money type
    testColInf.Type = 7;
    testColInf.Length = 8;
    testColInf.RecordOffset = 0;
    testColInf.DecimalOffset = 0;
    ObjData moneyData;
    pObjData moneyDataPtr = &moneyData;
    MoneyType testMoneyData = {0};
    testMoneyData.FractionPart = 0;
    pMoneyType pTestMoneyData = &testMoneyData;

    /** No Decimal Offset Case **/
    //Raw data for row data.
    char* testString = "450";
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, testString) == 0);
    
    assert(moneyDataPtr->Money->WholePart == 450);
    printf("%d", moneyDataPtr->Money->WholePart);
    printf("%u", moneyDataPtr->Money->FractionPart);
    assert(moneyDataPtr->Money->FractionPart == 0);

    /** Decimal Offset, Value > 1 Case **/
    //Value will be greater than one after dividing by
    //10 a DecimalOffset # of times
    testColInf.DecimalOffset = 1;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, testString) == 0);
    assert(moneyDataPtr->Money->WholePart == 45);
    assert(moneyDataPtr->Money->FractionPart == 0);

    /** Decimal Offset, Value < 1 Case **/
    testColInf.DecimalOffset = 3;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, testString) == 0);
    assert(moneyDataPtr->Money->WholePart == 0);
    assert(moneyDataPtr->Money->FractionPart == 4500);

    //Rounded or Truncated?
    char* testRoundedString = "45011999";
    testColInf.DecimalOffset = 5;
    assert(fp_internal_ParseColumn(pTestColInf, moneyDataPtr, (char*)pTestMoneyData, testRoundedString) == 0);
    assert(moneyDataPtr->Money->WholePart == 450);
    assert(moneyDataPtr->Money->FractionPart == 1199);

    return 0;
}