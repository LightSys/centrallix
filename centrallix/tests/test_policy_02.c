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
    char* domain = "";
    char* type = ""; 
    char* path = ""; 
    char* attr = "";
    int access_type = 0;
    CxssPolRule rule = {.MatchObject = "aa:bb:cc:dd"};
    assert(cxssIsRuleMatch(domain, type, path, attr, access_type, &rule));
    return 0;
}
