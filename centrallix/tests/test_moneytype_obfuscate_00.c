#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "utility/obfuscate.c"

long long
test(char** name)
{
    *name = "moneytype_00 - obfuscate";

    obfObfuscateData();
    
    return 0;
}
