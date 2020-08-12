#include "cxlib/mtask.h"
#include "obj.h"
#include <stdio.h>

void
start(void* v)
{
    MoneyType test = {1, 1};
    printf("%s\n", objFormatMoneyTmp(&test, NULL));
}

int
main()
{
    mtInitialize(0, start);
    return 0;
}
