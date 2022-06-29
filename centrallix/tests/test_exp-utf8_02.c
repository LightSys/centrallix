#include <assert.h>
#include "expression.h"
#include "charsets.h"
long long
test(char** name)
    {
    pExpression tree1 = nmMalloc(sizeof(Expression));
    pExpression tree2 = nmMalloc(sizeof(Expression));
    pExpression wordlist = nmMalloc(sizeof(Expression));
    pExpression string = nmMalloc(sizeof(Expression));
    
    expAllocExpression(tree1);
    expAllocExpression(tree2);
    expAllocExpression(wordlist);
    expAllocExpression(string);

    string->DataType = DATA_T_STRING;
    string->String = "test";

    wordlist->DataType = DATA_T_STRING;
    wordlist->String = "test,more,yup";
    
    *name = "Exp UTF-8: UTF-8 Mixed"; /* provides tests for the internal function found in charsets */

    /** Test UTF8 mixed against mixed **/
    printf("Test: %s\n",  tree2->Types.StringBuf);
    printf("Tree1: %s, Tree2: %s\n", tree1->String, tree2->String);
    exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    exp_fn_mixed(tree2, NULL, string, wordlist, NULL);
    printf("Tree1: %s, Tree2: %s\n", tree1->String, tree2->String);
    
    //exp_fn_mixed(NULL, NULL, NULL, NULL, NULL);
    
    return 0;
    }
