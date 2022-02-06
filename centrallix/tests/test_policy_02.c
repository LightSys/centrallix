#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "include/cxss/cxss.h"
//#include "include/cxss/policy.h"
long long
test(char** name)
    { 
    //This is just to make sure the code runs
    *name = "policy_02 blank match rule test";
    char* domain = "aa";
    char* type = "bb"; 
    char* path = "cc"; 
    char* attr = "dd";
    int access_type = 1;
    CxssPolRule rule = {.MatchObject = ":::"};
    assert(cxssIsRuleMatch(domain, type, path, attr, access_type, &rule) == CXSS_MATCH_T_TRUE);
    return 0;
}
