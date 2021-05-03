#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "obfuscate.h"
#include "testhelpers/mssErrorHelper.h"

void
testObfuscationWIntegerMultiples()
{
    ObjData srcVal, dstVal;
    long long moneyValue = 1100;
    MoneyType money = {moneyValue};
    srcVal.Money = &money;
    int numsToObfuscate = 50;
    int obfuscated = 0;
    int i;

    for (i = 0; i < numsToObfuscate; i++)
    {
        money.Value = moneyValue;
        int rval = obfObfuscateData(&srcVal, &dstVal, DATA_T_MONEY, NULL, NULL, NULL, NULL, "V", "i", NULL, NULL);
        assert(rval == 0);
        if (dstVal.Money->Value != srcVal.Money->Value) {
            obfuscated++;
        }
        moneyValue *= 2;
    }
    float percentObfuscated = (float)obfuscated / (float)numsToObfuscate;
    assert(percentObfuscated > 0.70);
}

void
testObfuscationWIntegerObfuscate()
{
    ObjData srcVal, dstVal;
    long long moneyValue = 1100;
    MoneyType money = {moneyValue};
    srcVal.Money = &money;
    int i;

    for (i = 0; i < 50; i++)
    {
        money.Value = moneyValue;
        int rval = obfObfuscateData(&srcVal, &dstVal, DATA_T_MONEY, NULL, NULL, NULL, NULL, "V", NULL, NULL, NULL);
        assert(rval == 0);
        assert(dstVal.Money->Value != srcVal.Money->Value);
        moneyValue *= 2;
    }
}

void
testMoneyTooLarge()
{
    ObjData srcVal, dstVal;
    MoneyType money = {25000000000000000000};
    srcVal.Money = &money;

    mssErrorHelper_init();
    obfObfuscateData(&srcVal, &dstVal, DATA_T_MONEY, NULL, NULL, NULL, NULL, "V", NULL, NULL, NULL);
    assert(mssErrorHelper_mostRecentErrorContains("Obfuscate not supported for Money Type greater than INT_MAX"));
}

long long
test(char** name)
{
    *name = "moneytype_00 - obfuscate";

    testObfuscationWIntegerMultiples();
    testObfuscationWIntegerObfuscate();
    testMoneyTooLarge();

    return 0;
}
