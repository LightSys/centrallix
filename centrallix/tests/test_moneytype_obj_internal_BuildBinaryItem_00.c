#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "moneytype_00 - obj_internal_BuildBinaryItem";
    char** item;
    item = (char**)malloc(24);
    int* itemlen;
    itemlen = (int*)malloc(4);
    
    //Utilizing expression.h functions to initialize expression and paramobjects
    pExpression testExp = expAllocExpression();
    pExpression testExpCmp = expAllocExpression();
    pParamObjects testParamObjects = expCreateParamList();
    
    //Buffer must be at least 12 bytes for data types not yet implemented.
    //Current highest byte size is 9, but 12 may come later
    unsigned char tmpbuf[12];
    unsigned char cmpbuf[12];

    /** NULL Case **/
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == -1);

    //For remaining cases, creating two ObjData with a moneytype inside, pObjData for parameters
    //After passing through BuildBinaryItem, they will be compared with memcmp() because it is not the binary value
    //itself that matters, but the comparison between the two objects.
    ObjData moneyData;
    MoneyType unionFiller = {70000};
    moneyData.Money = &unionFiller;
    pObjData moneyDataPtr = &moneyData;

    ObjData moneyDataCmp;
    MoneyType unionFiller2 = {65000};
    moneyDataCmp.Money = &unionFiller2;
    pObjData moneyDataPtrCmp = &moneyDataCmp;
    
    /** Positive Case **/
    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    testExpCmp = expPodToExpression(moneyDataPtrCmp, 7, testExpCmp);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == 0);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExpCmp, testParamObjects, cmpbuf) == 0);
    
    assert(memcmp(tmpbuf,cmpbuf,8) > 0);
    
    /** Negative Case **/
    unionFiller.Value = -70000;
    unionFiller2.Value = -65000;
    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    testExpCmp = expPodToExpression(moneyDataPtrCmp, 7, testExpCmp);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == 0);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExpCmp, testParamObjects, cmpbuf) == 0);

    assert(memcmp(tmpbuf,cmpbuf,8) < 0);
    
    expFreeExpression(testExp);
    expFreeExpression(testExpCmp);
    
    return 0;
}
