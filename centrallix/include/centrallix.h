#ifndef _CENTRALLIX_H
#define _CENTRALLIX_H

#include "stparse.h"


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
    }
    CxGlobals_t, *pCxGlobals_t;

extern CxGlobals_t CxGlobals;

#endif
