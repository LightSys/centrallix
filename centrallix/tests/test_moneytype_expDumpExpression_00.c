#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "testhelpers/stdoutHelper.h"

long long
test(char** name)
{
    *name = "moneytype_00 - expDumpExpression";
    
    pExpression testExp = expAllocExpression();
    ObjData moneyData;
    MoneyType money = {70000};
    moneyData.Money = &money;
    pObjData moneyDataPtr = &moneyData;

    testExp = expPodToExpression(moneyDataPtr, DATA_T_MONEY, testExp);

    stdoutHelper_startCapture();
    expDumpExpression(testExp);
    char* stdoutContents = stdoutHelper_stopCaptureAndGetContents();
    assert(strstr(stdoutContents, "MONEY = 70000, 0 child(ren), money=$7.00, NEW"));

    /** Money > INT_MAX **/
    money.Value = 18888888888888888;
    testExp = expPodToExpression(moneyDataPtr, DATA_T_MONEY, testExp);
    stdoutHelper_startCapture();
    expDumpExpression(testExp);
    stdoutContents = stdoutHelper_stopCaptureAndGetContents();
    assert(strstr(stdoutContents, "MONEY = 18888888888888888, 0 child(ren), money=$1888888888888.88, NEW"));

    expFreeExpression(testExp);
    
    return 0;
}
