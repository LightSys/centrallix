#include <assert.h>
#include <regex.h>
#include "centrallix.h"
#include "application.h"
#include "include/cxss/cxss.h"
//#include "include/cxss/policy.h"
long long
test(char** name)
    { 

    *name = "policy_05 generate regex from object name";

    char* case1 = "/apps/*/secpolicy.pol"; 
    char* case2 = "/a/path/ending/with/*";
    char* case3 = "*/a/path/starting/with.star";

    /* Case 1: Star in the middle of a path */
    regex_t* regexPtr = genRegexFromObjName(case1);
    assert(regexPtr != NULL);

    //make sure matches properly
    assert(regexec(regexPtr, "/apps/*/secpolicy.pol",                 0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/apps/test/secpolicy.pol",              0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/apps/three/layers/deep/secpolicy.pol", 0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/apps/+^|.?\\[]{}()$&/secpolicy.pol",   0, NULL, 0) == 0);

    //make sure does not match the following:
    assert(regexec(regexPtr, "/notApps/apps/test/secpolicy.pol", 0, NULL, 0) != 0);
    assert(regexec(regexPtr, "test",                             0, NULL, 0) != 0);
    assert(regexec(regexPtr, "/apps/secpolicy.pol",              0, NULL, 0) != 0);
    assert(regexec(regexPtr, "apps/*/secpolicy.pol",             0, NULL, 0) != 0);
    assert(regexec(regexPtr, "/apps/*/secpolicy.pol/test",       0, NULL, 0) != 0);

    nmFree(regexPtr, sizeof(regex_t));

    /* Case 2: Star at the end of a path */
    regexPtr = genRegexFromObjName(case2);
    assert(regexPtr != NULL);

    //make sure matches properly
    assert(regexec(regexPtr, "/a/path/ending/with/*",                          0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/a/path/ending/with/test.file",                  0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/a/path/ending/with/three/layers/deep/test.txt", 0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/a/path/ending/with/*+^|.?\\[]{}()$&",           0, NULL, 0) == 0);

    //make sure does not match the following:
    assert(regexec(regexPtr, "/invalid/a/path/ending/with/*", 0, NULL, 0) != 0);
    assert(regexec(regexPtr, "test",                          0, NULL, 0) != 0);
    assert(regexec(regexPtr, "/a/ending/with/*",              0, NULL, 0) != 0);
    assert(regexec(regexPtr, "a/path/ending/with/*",          0, NULL, 0) != 0);
    assert(regexec(regexPtr, "/a/path/ending/with*",          0, NULL, 0) != 0);

    nmFree(regexPtr, sizeof(regex_t));

    /* Case 3: Star at the start of a path */
    regexPtr = genRegexFromObjName(case3);
    assert(regexPtr != NULL);

    //make sure matches properly
    assert(regexec(regexPtr, "*/a/path/starting/with.star",                  0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/a/path/starting/with.star",                   0, NULL, 0) == 0);
    assert(regexec(regexPtr, "/three/layers/deep/a/path/starting/with.star", 0, NULL, 0) == 0);
    assert(regexec(regexPtr, "*+^|.?\\[]{}()$&*/a/path/starting/with.star",  0, NULL, 0) == 0);

    //make sure does not match the following:
    assert(regexec(regexPtr, "*/a/path/starting/with.invalid", 0, NULL, 0) != 0);
    assert(regexec(regexPtr, "test",                           0, NULL, 0) != 0);
    assert(regexec(regexPtr, "*/path/starting/with.star",      0, NULL, 0) != 0);
    assert(regexec(regexPtr, "*/a/path/starting/with",         0, NULL, 0) != 0);
    assert(regexec(regexPtr, "*/a/path/starting/with.stars",    0, NULL, 0) != 0);

    nmFree(regexPtr, sizeof(regex_t));

    return 0;
}