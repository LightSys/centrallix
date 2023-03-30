#include <assert.h>
#include "expression.h"
#include "charsets.h"
long long
test(char** name)
    {
    pExpression tree1;
    pExpression tree2;
    pExpression wordlist;
    pExpression string;

    *name = "Exp UTF-8: UTF-8 Mixed"; /* provides tests for the expression function found in charsets */
    
    tree1 = expAllocExpression();
    tree2 = expAllocExpression();
    wordlist = expAllocExpression();
    string = expAllocExpression();

    setlocale(0, "en_US.UTF-8");

    string->DataType = DATA_T_STRING;
    string->String = nmMalloc(100);
    strcpy(string->String, "testing, test test");

    wordlist->DataType = DATA_T_STRING;
    wordlist->String = nmMalloc(100);
    strcpy(wordlist->String, "tEst,more,yup");

    tree1->DataType = DATA_T_STRING;
    tree2->DataType = DATA_T_STRING;

    /** Test UTF8_mixed against mixed **/
    int result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    result = exp_fn_mixed(tree2, NULL, string, wordlist, NULL); 
    assert(result == 0);
    assert(strcmp(tree1->String, tree2->String) == 0);

    strcpy(string->String, "a test with dash-in it");
    result = exp_fn_utf8_mixed(tree1, NULL, string, NULL, NULL);
    assert(result == 0);
    result = exp_fn_mixed(tree2, NULL, string, NULL, NULL); 
    assert(result == 0);
    assert(strcmp(tree1->String, tree2->String) == 0);
    assert(strcmp("A Test With Dash-in It", tree1->String) == 0);

    strcpy(string->String, "a test with quote'in it");
    result = exp_fn_utf8_mixed(tree1, NULL, string, NULL, NULL);
    assert(result == 0);
    result = exp_fn_mixed(tree2, NULL, string, NULL, NULL); 
    assert(result == 0);
    assert(strcmp(tree1->String, tree2->String) == 0);
    assert(strcmp("A Test With Quote'in It", tree1->String) == 0);

    strcpy(string->String, "a tEst With MIXED cAsEs");
    result = exp_fn_utf8_mixed(tree1, NULL, string, NULL, NULL);
    assert(result == 0);
    result = exp_fn_mixed(tree2, NULL, string, NULL, NULL); 
    assert(result == 0);
    assert(strcmp(tree1->String, tree2->String) == 0);
    assert(strcmp("A Test With Mixed Cases", tree1->String) == 0);

    strcpy(string->String, "old mcdonald had a farm");
    strcpy(wordlist->String, "test,Mc*,a,fArM");
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    result = exp_fn_mixed(tree2, NULL, string, wordlist, NULL); 
    assert(result == 0);
    assert(strcmp(tree1->String, tree2->String) == 0);
    assert(strcmp("Old McDonald Had a fArM", tree1->String) == 0);

    /** test with untf8 specific characters**/
    strcpy(string->String, "à sèntence that ábÙsés latìn-Líkê chãräctërs");
    result = exp_fn_utf8_mixed(tree1, NULL, string, NULL, NULL);
    assert(result == 0);
    assert(strcmp("À Sèntence That Ábùsés Latìn-líkê Chãräctërs", tree1->String) == 0);

    strcpy(string->String, "В начале сотворил Бог небо и землю.");
    strcpy(wordlist->String, "сотворил,БОГ,НАЧ*");
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    assert(strcmp("В НАЧАле сотворил БОГ Небо И Землю.", tree1->String) == 0);

    strcpy(string->String, "in THE BeGinNiNg goD CREATED the heavEns and the EARTH");
    strcpy(wordlist->String, "the,beginning,created,aNd,test,not-used,useless,etc");
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    assert(strcmp("In the beginning God created the Heavens aNd the Earth", tree1->String) == 0);

    /* test with ignoring library */
    strcpy(string->String, "in THE BeGinNiNg goD CREATED the heavEns and the EARTH");
    strcpy(wordlist->String, "the,beginning,created,aNd,test,not-used,useless,etc");
    wordlist->Flags |= EXPR_F_NULL;
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    assert(strcmp("In The Beginning God Created The Heavens And The Earth", tree1->String) == 0);
    wordlist->Flags &= ~EXPR_F_NULL;

    /** test error handling **/
    string->Flags |= EXPR_F_NULL;
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == 0);
    assert(tree1->Flags & EXPR_F_NULL);
    string->Flags &= ~EXPR_F_NULL; /* clean up*/
    tree1->Flags &= ~EXPR_F_NULL;

    string->DataType = -1; /* not a string */
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == -1);
    string->DataType = DATA_T_STRING;

    wordlist->DataType = -1; /* not a string */
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == -1);
    wordlist->DataType = DATA_T_STRING;

    /* invalid character */
    strcpy(string->String, "in THE BeGinNiNg goD CREATED \xff the heavEns and the EARTH"); 
    result = exp_fn_utf8_mixed(tree1, NULL, string, wordlist, NULL);
    assert(result == -1);


    nmFree(string->String, 100);
    nmFree(wordlist->String, 100);
    nmFree(tree1, sizeof(Expression));
    nmFree(tree2, sizeof(Expression));
    nmFree(string, sizeof(Expression));
    nmFree(wordlist, sizeof(Expression));

    return 0;
    }
