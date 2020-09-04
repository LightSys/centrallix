#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"

long long
test(char** name)
{
    *name = "moneytype_00 - fp_internal_ParseColumn";
    char** item;
    int* itemlen;
    pExpression testExp = expAllocExpression();
    pParamObjects testParamObjects = expCreateParamList();
    unsigned char tmpbuf[12];

    /** NULL Case **/
    assert(obj_internal_BuildBinaryItem(item, itemlen, testExp, testParamObjects, tmpbuf) == -1);

    /** Positive Case **/
    testExp->Types.Money.Value = 90000;
    testExp->DataType = DATA_T_MONEY;
    testExp->NodeType = EXPR_N_MONEY;


    /** Negative Case **/
    //test.Value = -70500;
    //data_ptr = "Negative Seven And 05/100 ";
    //returnStr = objDataToWords(7,&test);
    //assert(strcmp(data_ptr, returnStr) == 0);

    expFreeExpression(testExp);

    return 0;
}
