#include "obj.h"
#include <assert.h>

long long
test(char** name)
{
    *name = "moneytype_00 - objGetOldRepresentationOfMoney";
    
    /* Positive case */
    MoneyType money = {75000};
    long long wholePart = 0;
    unsigned short fractionPart = 0;
    objGetOldRepresentationOfMoney(money, &wholePart, &fractionPart);
    assert(wholePart == 7);
    assert(fractionPart == 5000);

    /* Negative with non-zero fraction part */
    money.Value = -75000;
    wholePart = 0;
    fractionPart = 0;
    objGetOldRepresentationOfMoney(money, &wholePart, &fractionPart);
    assert(wholePart == -8);
    assert(fractionPart == 5000);

    /* Handles money.Value larger than INT_MAX */
    money.Value = 250000000000005000;
    wholePart = 0;
    fractionPart = 0;
    objGetOldRepresentationOfMoney(money, &wholePart, &fractionPart);
    assert(wholePart == 25000000000000);
    assert(fractionPart == 5000);

    return 0;
}
