#include <assert.h>
#include "include/cxss/cxss.h"

long long
test(char** name)
    {
   *name = "policy_01 Empty dummy test";
    /*
    char* domain = "dummy text";
    char* type = "dummy text";
    char* path = "dummy text";
    char* attr = "dummy text";
    int access_type = CXSS_ACC_T_READ; 
    int log_mode = CXSS_LOG_T_ALL;

    char* policyPath = "/usr/local/src/cx-git/centrallix/etc/security.pol";
    char* altPolicyPath = "/usr/local/src/cx-git/centrallix-sysdoc/SecurityModel_SamplePolicy.pol";

    cxInitialize();
    cxssAuthorize(domain, type, path, attr, access_type, log_mode);
    */
    return 0;
}