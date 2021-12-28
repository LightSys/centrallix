#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "include/cxss/cxss.h"
//#include "include/cxss/policy.h"
long long
test(char** name)
    { 
    //This is just to make sure the code runs
    *name = "policy_03 single match rule test";
    char* domain = "";
    char* type = ""; 
    char* path = ""; 
    char* attr = "";
    int access_type = 0;
    CxssPolRule rule = {.MatchObject = "aa:bb:cc:dd"};

    assert(cxssIsRuleMatch("aa", type, path, attr, access_type, &rule));
    assert(!cxssIsRuleMatch("bb", type, path, attr, access_type, &rule));
    assert(cxssIsRuleMatch(domain, "bb", path, attr, access_type, &rule));
    assert(!cxssIsRuleMatch(domain, "notbb", path, attr, access_type, &rule));
    assert(cxssIsRuleMatch(domain, type, "cc", attr, access_type, &rule));
    assert(!cxssIsRuleMatch(domain, type, "not cc", attr, access_type, &rule));
    assert(cxssIsRuleMatch(domain, type, path, "dd", access_type, &rule));
    assert(!cxssIsRuleMatch(domain, type, path, "not\ndd", access_type, &rule));
    return 0;
}
