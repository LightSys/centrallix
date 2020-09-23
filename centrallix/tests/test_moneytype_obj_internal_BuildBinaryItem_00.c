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
    pParamObjects testParamObjects = expCreateParamList();
    
    //Buffer must be at least 12 bytes
    unsigned char tmpbuf[12];

    /** NULL Case **/
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == -1);

    //For remaining cases, creating an ObjData with a moneytype inside, pObjData for parameters
    ObjData moneyData;
    MoneyType unionFiller = {70000};
    moneyData.Money = &unionFiller;
    pObjData moneyDataPtr = &moneyData;
    
    /** Positive Case **/
    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == 0);
    
    assert(tmpbuf[0] == 0x80);
    assert(tmpbuf[1] == 0x0);
    assert(tmpbuf[2] == 0x0);
    assert(tmpbuf[3] == 0x0);
    assert(tmpbuf[4] == 0x0);
    assert(tmpbuf[5] == 0x1);
    assert(tmpbuf[6] == 0x11);
    assert(tmpbuf[7] == 0x70);
    
    /** Negative Case **/
    unionFiller.Value = -70000;

    testExp = expPodToExpression(moneyDataPtr, 7, testExp);
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == 0);
    
    assert(tmpbuf[0] == 0x7f);
    assert(tmpbuf[1] == 0xff);
    assert(tmpbuf[2] == 0xff);
    assert(tmpbuf[3] == 0xff);
    assert(tmpbuf[4] == 0xff);
    assert(tmpbuf[5] == 0xfe);
    assert(tmpbuf[6] == 0xee);
    assert(tmpbuf[7] == 0x90);
    
    expFreeExpression(testExp);

    return 0;
}
