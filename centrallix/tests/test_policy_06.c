
#include <assert.h>
#include "centrallix.h"
#include "application.h"

long long
test(char** name)
    {
    //pApplication app;
    
    //This is just to make sure the code runs
    *name = "policy_06 Test newSecurity.pol's first rule";
    
    /** Default global values **/
	strcpy(CxGlobals.ConfigFileName, "/home/devel/cxinst/etc/centrallix.conf");
	CxGlobals.QuietInit = 1;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	CxGlobals.ArgV = NULL;
	CxGlobals.Flags = 0;

    cxInitialize();
    cxssPushContext();

    //make a dummy call with junk data
    char* domain = "system";
    char* type = "dummy text";
    // /usr/local/src/cx-git/centrallix-os/INSTALL.txt
    char* path = "/apps/*/secpolicy.pol";
    char* attr = "dummy text";
    int access_type = -1;
    int log_mode = -1;

    int CXSS_ACT_T_DENY = 2;

    assert(cxssAuthorize(domain, type, path, attr, access_type, log_mode) == CXSS_ACT_T_DENY); 

    //try with a variety of access types  
    assert(cxssAuthorize(domain, type, path, attr, 1, log_mode) == CXSS_ACT_T_DENY); 
    assert(cxssAuthorize(domain, type, path, attr, 512, log_mode) == CXSS_ACT_T_DENY); 
    assert(cxssAuthorize(domain, type, path, attr, 10, log_mode) == CXSS_ACT_T_DENY); 

    

    //TODO: test exception with system:seckernel
    
    
    return 0;
}
