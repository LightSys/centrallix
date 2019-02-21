#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/strtcpy.h"
#include "application.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2018 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	Application Management Layer (AML)                      */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	November 19, 2018                                       */
/*									*/
/* Description:	The application management layer provides session and	*/
/*		object management for running applications.		*/
/************************************************************************/


/*** Module-wide globals ***/
AML_t AML;


/*** app_i_DestroyAppData() - called when cleaning up an app data structure
 ***/
int
app_i_DestroyAppData(void* appdata_v, void* context)
    {
    pAppData appdata = (pAppData)appdata_v;

	/** Deallocate it **/
	if (appdata)
	    {
	    /** Finalize? **/
	    if (appdata->Finalize)
		appdata->Finalize(appdata->Data);

	    /** Release the memory **/
	    nmFree(appdata, sizeof(AppData));
	    }

    return 0;
    }


/*** appCreate() - create a new application instance.
 ***/
pApplication
appCreate(char* akey)
    {
    pApplication app = NULL;

	/** Allocate the application **/
	app = (pApplication)nmMalloc(sizeof(Application));
	if (!app)
	    goto error;

	/** Need to generate a temporary key? **/
	if (akey)
	    strtcpy(app->Key, akey, sizeof(app->Key));
	else
	    cxssGenerateHexKey(app->Key, 16);

	/** Set it up **/
	xhInit(&app->AppData, 255, 0);

	/** Set context **/
	cxssSetVariable("APP:akey", app->Key, 0);

	/** List it in our applications array **/
	xaAddItem(&AML.Applications, (void*)app);
	xhAdd(&AML.AppsByKey, app->Key, (void*)app);

	return app;

    error:
	if (app)
	    nmFree(app, sizeof(Application));
	return NULL;
    }


/*** appResume() - resume an existing application
 ***/
int
appResume(pApplication app)
    {

	/** Set context **/
	cxssSetVariable("APP:akey", app->Key, 0);

    return 0;
    }


/*** appDestroy() - destroys an existing application instance.
 ***/
int
appDestroy(pApplication app)
    {

	/** Destroy the app data **/
	xhClear(&app->AppData, app_i_DestroyAppData, NULL);
	xhDeInit(&app->AppData);

	/** Remove it from our applications array **/
	xaRemoveItem(&AML.Applications, xaFindItem(&AML.Applications, (void*)app));
	xhRemove(&AML.AppsByKey, app->Key);

	/** Free the app structure itself **/
	nmFree(app, sizeof(Application));

    return 0;
    }


/*** appRegisterAppData() - register a data structure with the application
 *** layer to facilitate that data having the same scope and lifetime as
 *** an application.
 ***/
int
appRegisterAppData(char* datakey, void* data, int (*finalize_fn)())
    {
    char* akey = NULL;
    pApplication app = NULL;
    pAppData appdata;

	/** Lookup the akey **/
	cxssGetVariable("APP:akey", &akey, NULL);
	if (akey)
	    {
	    app = (pApplication)xhLookup(&AML.AppsByKey, akey);
	    if (app)
		{
		appdata = (pAppData)nmMalloc(sizeof(AppData));
		if (appdata)
		    {
		    appdata->Key = nmSysStrdup(datakey);
		    appdata->Data = data;
		    appdata->Finalize = finalize_fn;
		    xhAdd(&app->AppData, appdata->Key, (void*)appdata);

		    return 0;
		    }
		}
	    }

    return -1;
    }


/*** appLookupAppData() - find data that has been registered with the
 *** application.
 ***/
void*
appLookupAppData(char* datakey)
    {
    char* akey = NULL;
    pApplication app = NULL;
    pAppData appdata;

	/** Lookup the akey **/
	cxssGetVariable("APP:akey", &akey, NULL);
	if (akey)
	    {
	    app = (pApplication)xhLookup(&AML.AppsByKey, akey);
	    if (app)
		{
		appdata = (pAppData)xhLookup(&app->AppData, datakey);
		if (appdata)
		    {
		    return appdata->Data;
		    }
		}
	    }

    return NULL;
    }


/*** appInitialize() - set up the application management layer
 ***/
int
appInitialize()
    {

	/** Set up tracking of running applications **/
	xaInit(&AML.Applications, 16);
	xhInit(&AML.AppsByKey, 255, 0);

    return 0;
    }

