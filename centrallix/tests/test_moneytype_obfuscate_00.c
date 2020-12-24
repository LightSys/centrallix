#include "obj.h"
#include <assert.h>
#include <stdio.h>
#include "expression.h"
#include "cxlib/xstring.h"
#include "obfuscate.h"

long long
test(char** name)
{
    *name = "moneytype_00 - obfuscate";

    //MoneyType
    int dataType = 7;
    
    pObfWord wordList;
    pObfWordCat catList;
    char* attrname, objname, type_name, key, which, param;
    char example[] = "exampleWord";
    char ex2[] = "key";
    char ex3[] = "type_name";
    char ex4[] = "objname";
    char ex5[] = "attrname";
    which = example; key = ex2; type_name = ex3; objname = ex4; attrname = ex5;
    
    void* generic = example;
    
    ObjData srcValFiller, dstValFiller;
    MoneyType unionFiller = {70000};
    srcValFiller.Money = &unionFiller;
    srcValFiller.Generic = generic;
    pObjData srcVal = &srcValFiller;
    pObjData dstVal = &dstValFiller;
    
    obfObfuscateData(srcVal, dstVal, dataType, attrname, objname, type_name, key, which, param, wordList, catList);
    
    return 0;
}
