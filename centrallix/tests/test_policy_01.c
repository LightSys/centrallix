#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "cxss/cxss.h"

long long
test(char** name)
    {
        
    *name = "policy_01 Simple Rule Test";
    
    //This is just to make sure the code runs
    *name = "policy_01 Basic csxx policy allow test";
    
    //make a dummy call with junk data
    char* domain = "dummy text";
    char* type = "dummy text";
    // /usr/local/src/cx-git/centrallix-os/INSTALL.txt
    char* path = "dummy text";
    char* attr = "dummy text";
    int access_type = -1;
    int log_mode = -1;

    /** Default global values **/
	strcpy(CxGlobals.ConfigFileName, "/home/devel/cxinst/etc/centrallix.conf");
	CxGlobals.QuietInit = 1;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	CxGlobals.ArgV = NULL;
	CxGlobals.Flags = 0;

    cxInitialize();

    cxssPushContext();

    assert(cxssAuthorize(domain, type, path, attr, access_type, log_mode) == 2);

    //free everything that is leaking
    //nmFree(CXSS.Policy, sizeof(pCxssPolicy));

    return 0;
}