#ifndef _CENTRALLIX_H
#define _CENTRALLIX_H

#include "stparse.h"


/*** Platform-independent types definition ***/
typedef long long CXINT64;
typedef int CXINT32;
typedef unsigned int CXUINT32;
typedef short CXINT16;
typedef unsigned short CXUINT16;
typedef char CXINT8;
typedef unsigned char CXUINT8;
typedef char CXCHAR;
#define CX_CONV8(x)	(x)
#define CX_CONV16(x)	((CX_CONV8((x)&0xff)<<8)|CX_CONV8(((x)>>8)&0xff))
#define CX_CONV32(x)	((CX_CONV16((x)&0xffff)<<16)|CX_CONV16(((x)>>16)&0xffff))
#define CX_CONV64(x)	((CX_CONV32((x)&0xffffffffLL)<<32)|CX_CONV32(((x)>>32)&0xffffffffLL))


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
    XArray	ShutdownHandlers;
    int		ShuttingDown;
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

#define CX_CURRENT_IFACE	(2)
extern int CxSupportedInterfaces[];

/** shutdown handlers function **/
typedef void (*ShutdownHandlerFunc)();
int cxAddShutdownHandler(ShutdownHandlerFunc);

/*** startup functions ***/
int cxInitialize();
int cxHtInit();
int cxNetworkInit();

#endif
