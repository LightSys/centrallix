#ifndef _CENTRALLIX_H
#define _CENTRALLIX_H

#include "stparse.h"


/*** Loaded module info ***/
typedef struct _CXM
    {
    struct _CXM* Next;
    void*	dlhandle;
    int		(*InitFn)();
    int		(*DeinitFn)();
    char**	Prefix;
    char**	Description;
    int*	Version;
    int*	InterfaceVer;
    }
    CxModule, *pCxModule;

/*** System globals. ***/
extern char* cx__version;
extern int cx__build;
extern int cx__subbuild;
extern char* cx__stability;
extern char* cx__years;


/*** Other Centrallix system-wide globals ***/
typedef struct _CXG
    {
    char	ConfigFileName[256];
    pStructInf	ParsedConfig;
    int		QuietInit;
    pCxModule	ModuleList;
    }
    CxGlobals_t, *pCxGlobals_t;

extern CxGlobals_t CxGlobals;


/*** Loadable modules use this to define init/finish functions.
 *** Note that Centrallix init/finish functions are quite
 *** distinct from the _init/_fini functions that libdl uses.  A
 *** module can use those also, but those are not recommended
 *** for use when calling Centrallix core routines to register
 *** the module - use these for those purposes.
 ***/
#ifdef MODULE
#define MODULE_INIT(x)		int moduleInitialize() { return (x)(); }
#define MODULE_DEINIT(x)	int moduleDeInitialize() { return (x)(); }
#define MODULE_PREFIX(x)	char* modulePrefix = (x)
#define MODULE_DESC(x)		char* moduleDescription = (x)
#define MODULE_VERSION(x,y,z)	int moduleVersion = ((x<<24)+(y<<16)+z)
#define MODULE_IFACE(x)		int moduleInterface = (x)
#else
#define MODULE_INIT(x)		
#define MODULE_DEINIT(x)	
#define MODULE_PREFIX(x)	
#define MODULE_DESC(x)		
#define MODULE_VERSION(x,y,z)	
#define MODULE_IFACE(x)		
#endif

#define CX_CURRENT_IFACE	(1)
extern int CxSupportedInterfaces[];

/*** startup functions ***/
int cxInitialize();
int cxHtInit();
int cxNetworkInit();

#endif
