#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"

long long
test(char** name)
{
    *name = "moneytype_00 - expCompareExpressionValues";
    
    pExpression exp1 = expAllocExpression();
    ObjData moneyData1;
    MoneyType money1 = {70000};
    moneyData1.Money = &money1;
    exp1 = expPodToExpression(&moneyData1, DATA_T_MONEY, exp1);

    pExpression exp2 = expAllocExpression();
    ObjData moneyData2;
    MoneyType money2 = {70000};
    moneyData2.Money = &money2;
    exp2 = expPodToExpression(&moneyData2, DATA_T_MONEY, exp2);

    // true case (exp1 == exp2)
    assert(expCompareExpressionValues(exp1, exp2) == 1);

    // false case (exp1 != exp2)
    money2.Value = 8000;
    exp2 = expPodToExpression(&moneyData2, DATA_T_MONEY, exp2);
    assert(expCompareExpressionValues(exp1, exp2) == 0);

    expFreeExpression(exp1);
    expFreeExpression(exp2);

    return 0;
}
