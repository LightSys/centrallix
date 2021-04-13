#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "moneytype_00 - objBuildBinaryImage";
    
    pExpression testExp = expAllocExpression();
    pExpression testExpCmp = expAllocExpression();
    pParamObjects testParamObjects = expCreateParamList();
    
    //Buffer must be at least 12 bytes for data types not yet implemented.
    //Current highest byte size is 9, but 12 may come later    
    char buf[12];
    char cmpBuf[12];
    int buflen = 12;
    
    /** NULL Case **/
    assert(objBuildBinaryImage(buf, buflen, &testExp, 1, testParamObjects, 0) == -1);

    //For remaining cases, creating two ObjData with a moneytype inside, pObjData for parameters
    //After passing through BuildBinaryImage, they will be compared with memcmp() because it is not the binary value
    //itself that matters, but the comparison between the two objects.
    ObjData moneyData;
    MoneyType money = {70000};
    moneyData.Money = &money;
    pObjData moneyDataPtr = &moneyData;

    ObjData moneyDataCmp;
    MoneyType moneyCmp = {65000};
    moneyDataCmp.Money = &moneyCmp;
    pObjData moneyDataPtrCmp = &moneyDataCmp;

    /** Positive Case **/
    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    testExpCmp = expPodToExpression(moneyDataPtrCmp, 7, testExpCmp);
    objBuildBinaryImage(buf, buflen, &testExp, 1, testParamObjects, 0);
    objBuildBinaryImage(cmpBuf, buflen, &testExpCmp, 1, testParamObjects, 0);
    assert(memcmp(buf,cmpBuf,8) > 0);
   
    /** Negative Case **/
    money.Value = -70000;
    moneyCmp.Value = -65000;
    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    testExpCmp = expPodToExpression(moneyDataPtrCmp, 7, testExpCmp);
    objBuildBinaryImage(buf, buflen, &testExp, 1, testParamObjects, 0);
    objBuildBinaryImage(cmpBuf, buflen, &testExpCmp, 1, testParamObjects, 0);
    assert(memcmp(buf,cmpBuf,8) < 0);
    
    expFreeExpression(testExp);
    expFreeExpression(testExpCmp);
    
    return 0;
}
