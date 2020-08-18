#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "moneytype_06 - obj_internal_BuildBinaryItem";
    char** item;
    int* itemlen;
    pExpression testExp = expAllocExpression();
    pParamObjects testParamObjects = expCreateParamList();
    unsigned char tmpbuf[12];

    /** Positive Case **/
    testExp->Types.Money.Value = 90000;
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == -1);

    /** Negative Case **/
    //test.Value = -70500;
    //data_ptr = "Negative Seven And 05/100 ";
    //returnStr = objDataToWords(7,&test);
    //assert(strcmp(data_ptr, returnStr) == 0);
    
    expFreeExpression(testExp);

    return 0;
}
