#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "osdrivers/objdrv_fp.c"

long long
test(char** name)
{
    *name = "moneytype_00 - fp_internal_ParseColumn";
    
    //FpColInf testColInf = {0};
    pFpColInf = &FpColInf;
    
    //7 for money type
    FpColInf.Type = 7;
    FpColInf.Length = 8;
    FpColInf.RecordOffset = 0;
    FpColInf.DecimalOffset = 4;
    ObjData moneyData;
    pObjData moneyDataPtr = &moneyData;
    MoneyType testMoneyData = {0};
    pMoneyType pTestMoneyData = &testMoneyData;
    
    //Raw data for row data.
    char* testString = "450";
    
    assert(fp_internal_ParseColumn(pFpColInf, moneyDataPtr, (char*)pTestMoneyData, testString) == 0);

    /** NULL Case **/

    /** Positive Case **/
    testExp->Types.Money.Value = 90000;
    testExp->DataType = DATA_T_MONEY;
    testExp->NodeType = EXPR_N_MONEY;


    /** Negative Case **/
    //test.Value = -70500;
    //data_ptr = "Negative Seven And 05/100 ";
    //returnStr = objDataToWords(7,&test);
    //assert(strcmp(data_ptr, returnStr) == 0);

    return 0;
}
