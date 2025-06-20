#ifndef _CENTRALLIX_H
#define _CENTRALLIX_H

#include "stparse.h"
#include "cxlib/mtsession.h"


#define	CX_USERNAME_SIZE    (MSS_USERNAME_SIZE)
#define	CX_PASSWORD_SIZE    (MSS_PASSWORD_SIZE)


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
    char	PidFile[256];
    char**	ArgV;
    pStructInf	ParsedConfig;
    int		QuietInit;
    pCxModule	ModuleList;
    XArray	ShutdownHandlers;
    int		Flags;
    int		ClkTck;
    pFile	DebugFile;
    }
    CxGlobals_t, *pCxGlobals_t;

extern CxGlobals_t CxGlobals;

#define CX_F_SHUTTINGDOWN	1	/* shutting down */
#define CX_F_ENABLEREMOTEPW	2	/* enable sending auth to remote services */
#define CX_F_DEBUG		4	/* for testing only */
#define CX_F_SERVICE		8	/* become a background service */


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
int cxDriverInit();
int cxHtInit();
int cxNetworkInit();
int cxLinkSigningSetup(pStructInf my_config);

/*** Debugging ***/
int cxDebugLog(char* fmt, ...);


/*** Sys Info data structure for object/tree additions ***/
typedef struct _CXSI
    {
    char*	Path;			/* Path under /sys/cx.sysinfo/ */
    char*	Name;			/* name of this particular object. */
    pXArray	(*AttrEnumFn)(void*, char*);	/* Attribute enumerator */
    pXArray	Attrs;			/* -or- Hardcoded list of attrs */
    pXArray	(*ObjEnumFn)(void*);		/* Subobject name enumerator */
    pXArray	Objs;			/* -or- Hardcoded list of object names */
    pXArray	(*MethodEnumFn)(void*, char*);	/* Method enumerator */
    pXArray	Methods;		/* -or- Hardcoded list of methods */
    int		(*GetAttrTypeFn)(void*, char*, char*);	/* func to get one attribute type */
    pXArray	AttrTypes;		/* -or- if Attrs given, we can specify AttrTypes */
    int		(*GetAttrValueFn)(void*, char*, char*, void*);	/* func to get one attribute value */
    int		(*ExecFn)(void*, char*, char*, char*);	/* func to execute a method */
    int		AdminOnly;		/* only allow "root" to see it.  N.B.: change this once the sec model is in! */
    void*	Context;
    pXArray	Subtree;
    }
    SysInfoData, *pSysInfoData;

/*** Sys Info driver functions for registering subtrees and objects.
 *** Enum funcs must return an XArray to be freed by the func's caller!!
 *** objname = NULL on funcs to get stuff on subtree root
 *** void* value is a pObjData (see datatypes.h).
 ***/
pSysInfoData sysAllocData(char* path, pXArray (*attrfn)(void*, char* objname), pXArray (*objfn)(void*), pXArray (*methfn)(void*, char* objname), int (*getfn)(void*, char* objname, char* attrname), int (*valuefn)(void*, char* objname, char* attrname, void* value), int (*execfn)(void*, char* objname, char* methname, char* methparam), int admin_only);
int sysRegister(pSysInfoData, void* context);
int sysAddAttrib(pSysInfoData, char*, int);
int sysAddObj(pSysInfoData, char*);
int sysAddMethod(pSysInfoData, char*);

#endif
