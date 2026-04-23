#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/util.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "centrallix.h"
#include "wgtr.h"
#include "iface.h"
#include "stparse.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_page.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Widget driver for the overall HTML page.		*/
/************************************************************************/



int
htpageRender(pHtSession s, pWgtrNode tree, int z)
    {
    char *ptr;
    char name[64];
    int attract = 0;
    int watchdogtimer;
    char bgstr[128];
    int show, i;
    char kbfocus1[64];	/* kb focus = 3d raised */
    char kbfocus2[64];
    char msfocus1[64];	/* ms focus = black rectangle */
    char msfocus2[64];
    char dtfocus1[64];	/* dt focus = navyblue rectangle */
    char dtfocus2[64];
    int font_size = 12;
    char font_name[128];
    int show_diag = 0;
    int dpi_scaling = 0;
    int w,h;
    char* path;
    time_t t;
    struct tm* timeptr;
    char timestr[80];
    XArray endorsements = { nAlloc: 0 };
    XArray contexts  = { nAlloc: 0 };
    int max_requests = 1;

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTPAGE", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Set default focus colors. **/
	strcpy(msfocus1,"#ffffff");	/* ms focus = 3d raised */
	strcpy(msfocus2,"#7a7a7a");
	strcpy(kbfocus1,"#000000");	/* kb focus = black rectangle */
	strcpy(kbfocus2,"#000000");
	strcpy(dtfocus1,"#000080");	/* dt focus = navyblue rectangle */
	strcpy(dtfocus2,"#000080");

    	/** If not at top-level, don't render the page. **/
	/** Z is set to 10 for the top-level page. **/
	if (z != 10) return 0;

	/** These are always set for a page widget **/
	wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w));
	wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h));

	/** Max active server requests at one time **/
	wgtrGetPropertyValue(tree,"max_requests",DATA_T_INTEGER,POD(&max_requests));

	/** Handle page icon. **/
	if (wgtrGetPropertyValue(tree, "icon", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (htrAddHeaderItem_va(s, "\t<link rel='shortcut icon' href='%STR&HTE'>\n",ptr) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write HTML for page icon.");
		goto err;
		}
	    }

    	/** Handle page title. **/
	if (wgtrGetPropertyValue(tree, "title", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (htrAddHeaderItem_va(s, "\t<title>%STR&HTE</title>\n", ptr) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write HTML for page title.");
		goto err;
		}
	    }

    	/** Check for page load status **/
	show = htrGetBoolean(tree, "loadstatus", 0);
	if (show < 0) goto err;

	/** Initialize the html-related interface stuff **/
	if (ifcHtmlInit(s, tree) < 0)
	    {
	    mssError(0, "HTR", "Error Initializing Html Interface code...continuing, but things might not work for client");
	    }
	
	/** Auto-call startup and cleanup **/
	if (htrAddBodyParam_va(s, " onLoad='startup_%STR&SYM();' onUnload='cleanup();'", s->Namespace->DName) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to register startup and cleanup functions.");
	    goto err;
	    }
	if (htrAddScriptCleanup(s, "\tpg_cleanup();\n") != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write page cleanup JS.");
	    goto err;
	    }

	/** Check for bgcolor. **/
	if (htrGetBackground(tree, NULL, 1, bgstr, sizeof(bgstr)) == 0)
	    {
	    if (htrAddBodyParam_va(s, " style=\"%STR\"", bgstr) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write page background color.");
		goto err;
		}
	    }

	/** Check for text color **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (htrAddBodyParam_va(s, " text='%STR&HTE'", ptr) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write page text color.");
		goto err;
		}
	    }

	/** Check for link color **/
	if (wgtrGetPropertyValue(tree,"linkcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (htrAddBodyParam_va(s, " link='%STR&HTE'", ptr) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write page link color.");
		goto err;
		}
	    }

	/** Keyboard Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"kbdfocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(kbfocus1,ptr,sizeof(kbfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"kbdfocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(kbfocus2,ptr,sizeof(kbfocus2));
	    }

	/** Mouse Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"mousefocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(msfocus1,ptr,sizeof(msfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"mousefocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(msfocus2,ptr,sizeof(msfocus2));
	    }

	/** Data Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"datafocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(dtfocus1,ptr,sizeof(dtfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"datafocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(dtfocus2,ptr,sizeof(dtfocus2));
	    }

	/** Cx windows attract to browser edges? if so, by how much **/
	if (wgtrGetPropertyValue(tree,"attract",DATA_T_INTEGER,POD(&ptr)) == 0)
	    attract = (intptr_t)ptr;

	/** Get booleans. **/
	show_diag = htrGetBoolean(tree, "show_diagnostics", 0);
	dpi_scaling = htrGetBoolean(tree, "dpi_scaling", 0);

	/** Get font data. **/
	wgtrGetPropertyValue(tree, "font_size", DATA_T_INTEGER, POD(&font_size));
	if (font_size < 5 || font_size > 100) font_size = 12;
	if (wgtrGetPropertyValue(tree, "font_name", DATA_T_STRING, POD(&ptr)) ==  0 && ptr)
	    strtcpy(font_name, ptr, sizeof(font_name));
	else
	    strcpy(font_name, "");

	/** Add global for page metadata **/
	if (htrAddScriptGlobal(s, "page", "{}", 0) != 0) goto err;

	/** Add a list of highlightable areas **/
	/** These are javascript global variables**/
	if (htrAddScriptGlobal(s, "pg_arealist", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_keylist", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_curarea", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_curlayer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_curkbdlayer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_curkbdarea", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_lastkey", "-1", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_lastmodifiers", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_keytimeoutid", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_keyschedid", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_modallayer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_key_ie_shifted", "false", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_attract", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_gshade", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_closetype", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_explist", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_schedtimeout", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_schedtimeoutlist", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_schedtimeoutid", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_schedtimeoutstamp", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_insame", "false", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "cn_browser", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "ibeam_current", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "util_cur_mainlayer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_loadqueue", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_loadqueue_busy", "999999", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_debug_log", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_isloaded", "false", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_username", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_msg_handlers", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_msg_layer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_msg_timeout", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_diag", (show_diag) ? "true" : "false", 0) != 0) goto err; /* Use pop-ups for certain non-fatal warnings */
	if (htrAddScriptGlobal(s, "pg_dpi_scaling", (dpi_scaling) ? "true" : "false", 0) != 0) goto err; /* Scale launched windows using display dots-per-inch */
	if (htrAddScriptGlobal(s, "pg_width", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_height", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_charw", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_charh", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_parah", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_namespaces", "{}", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_handlertimeout", "null", 0) != 0) goto err; /* Used by htr_mousemovehandler() in ht_render.js. */
	if (htrAddScriptGlobal(s, "pg_mousemoveevents", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_handlers", "[]", 0) != 0) goto err; /* List of handlers for basic document events (click, mousemove, keypress, etc.). */
	if (htrAddScriptGlobal(s, "pg_capturedevents", "0", 0) != 0) goto err; /* Binary OR of all event flags currently registered on the document. */
	if (htrAddScriptGlobal(s, "pg_tiplayer", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_tipindex", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_tiptmout", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_waitlyr", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_appglobals", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_sessglobals", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_scripts", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_endorsements", "[]", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "pg_max_requests", "1", 0) != 0) goto err;

	/** Include necessary scripts. **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_page.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/jquery/jquery-1.11.1.js", 0) != 0) goto err;

	/** Write named global **/
	if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));

 	/** Link the window and document to widgets. **/
	if (htrAddWgtrObjLinkage(s, tree, "window") != 0) goto err;
	if (htrAddWgtrCtrLinkage(s, tree, "document") != 0) goto err;

	/** Set the application key **/
	/*** TODO: Greg - This should be a call to htrAddScriptGlobal(), but I
	 *** do not know how to do that with a the dynamic, formatted value.
	 ***/
	htrAddScriptInit_va(s, "\tif (typeof window.akey == 'undefined') window.akey = '%STR&JSSTR';\n", s->ClientInfo->AKey);

	/** Send server's time to the client. **/
	t = time(NULL);
	timeptr = localtime(&t);
	if (timeptr)
	    {
	    /** This isn't 100% ideal -- it causes a couple seconds of clock drift **/
	    if (strftime(timestr, sizeof(timestr), "%Y %m %d %T %Z", timeptr) > 0)
		{
		if (htrAddScriptInit_va(s, "\tpg_servertime = new Date(%STR&DQUOT);\n", timestr))
		    {
		    mssError(0, "HTPAGE", "Failed to write server time in JS.");
		    goto err;
		    }
		if (strftime(timestr, sizeof(timestr), "%Y %m %d %T", timeptr) > 0)
		    {
		    if (htrAddScriptInit_va(s, "\tpg_servertime_notz = new Date(%STR&DQUOT);\n", timestr) != 0)
			{
			mssError(0, "HTPAGE", "Failed to write server time notz in JS.");
			goto err;
			}
		    }
		if (htrAddScriptInit(s, "\tpg_clienttime = new Date();\n") != 0)
		    {
		    mssError(0, "HTPAGE", "Failed to write client time in JS.");
		    goto err;
		    }
		}
	    }

	/** Write JS to initialize page variables. **/
	htrAddScriptInit_va(s,
	    "\tif (typeof(pg_status_init) === 'function') pg_status_init();\n"
	    "\tpg_init(wgtrGetNodeRef(ns,'%STR&SYM'),%INT);\n"
	    "\tpg_username = '%STR&JSSTR';\n"
	    "\tpg_width = %INT;\n"
	    "\tpg_height = %INT;\n"
	    "\tpg_max_requests = %INT;\n"
	    "\tpg_charw = %INT;\n"
	    "\tpg_charh = %INT;\n"
	    "\tpg_parah = %INT;\n",
	    name, attract,
	    mssUserName(),
	    w, h,
	    max_requests,
	    s->ClientInfo->CharWidth,
	    s->ClientInfo->CharHeight,
	    s->ClientInfo->ParagraphHeight
	);
	
	/** Write obscore_data. **/
	pStruct c_param = stLookup_ne(s->Params, "cx__obscure");
	if (htrAddScriptInit_va(s,
	    "\tobscure_data = %STR;\n",
	    (c_param != NULL && strcasecmp(c_param->StrVal, "yes") == 0) ? "true" : "false"
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write obscure_data variable in JS.");
	    goto err;
	    }

	/** Add template paths **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    {
	    /** Ignore null templates. **/
	    path = wgtrGetTemplatePath(tree, i);
	    if (path == NULL) continue;
	    
	    /** Write code for the template. **/
	    if (htrAddScriptInit_va(s,
		"\twgtrGetNodeRef(ns,'%STR&SYM').templates.push('%STR&JSSTR');\n",
		name, path
	    ) != 0)
		{
		mssError(0, "HTPAGE",
		    "Failed to write JS for template path #%d: \"%s\"",
		    i + 1, path
		);
		goto err;
		}
	    }

	/** Endorsements **/
	bool error = false;
	if (check(xaInit(&endorsements, 16) != 0)) goto err;
	if (check(xaInit(&contexts, 16) != 0)) goto err;
	if (check_neg(cxssGetEndorsementList(&endorsements, &contexts)) < 0) goto err;
	for (unsigned int i = 0; i < endorsements.nItems; i++)
	    {
	    char* endorsement = endorsements.Items[i];
	    char* context = contexts.Items[i];
	    
	    /** Write endorsement. **/
	    if (htrAddScriptInit_va(s,
		"\tpg_endorsements.push({ e:'%STR&JSSTR', ctx:'%STR&JSSTR' });\n",
		endorsement, context
	    ) != 0)
		{
		mssError(0, "HTPAGE",
		    "Failed to write endorsement #%d: \"%s\" of \"%s\" (continuing...)",
		    i + 1, endorsement, context
		);
		error = true;
		}
	    
	    /** Clean up. **/
	    nmSysFree(endorsement);
	    nmSysFree(context);
	    }
	if (check(xaDeInit(&endorsements)) != 0) goto err;
	endorsements.nAlloc = 0;
	if (check(xaDeInit(&contexts)) != 0) goto err;
	contexts.nAlloc = 0;
	if (error)
	    {
	    mssError(0, "HTPAGE", "Failed to write one or more endorsement(s) (failing).");
	    goto err;
	    }

	/** Add focus box. **/
	if (htrAddStylesheetItem(s,
	    "\t\ttd img  { display: block; }\n"
	    "\t\t.pg     { position:absolute; visibility:hidden; }\n"
	    "\t\t.pgclip { overflow:hidden; z-index:1000; }\n"
	    "\t\t#pgtop  { left:-1000px; top:0px;     width:1152px; height:1px;   }\n"
	    "\t\t#pgbtm  { left:-1000px; top:0px;     width:1152px; height:1px;   }\n"
	    "\t\t#pgrgt  { left:0px;     top:-1000px; width:1px;    height:864px; }\n"
	    "\t\t#pglft  { left:0px;     top:-1000px; width:1px;    height:864px; }\n"
	    "\t\t#pgktop { left:-1000px; top:0px;     width:1152px; height:1px;   }\n"
	    "\t\t#pgkbtm { left:-1000px; top:0px;     width:1152px; height:1px;   }\n"
	    "\t\t#pgkrgt { left:0px;     top:-1000px; width:1px;    height:864px; }\n"
	    "\t\t#pgklft { left:0px;     top:-1000px; width:1px;    height:864px; }\n"
	    "\t\t#pgtvl  { left:0px;     top:0px;     width:1px;    height:1px;   z-index:0;  }\n"
	    "\t\t#pginpt { left:0px;     top:20px;                                z-index:20; }\n"
	    "\t\t#pgping { left:0px;     top:0px;     width:0px;    height:0px;   z-index:0;  }\n"
	    "\t\t#pgmsg  { left:0px;     top:0px;     width:0px;    height:0px;   z-index:0;  }\n"
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write focus box CSS.");
	    goto err;
	    }

	/** Write pgstat. **/
	if (show == 1)
	    {
	    if (htrAddStylesheetItem(s,
		"\t\t#pgstat { "
		    "position:absolute; "
		    "visibility:visible; "
		    "left:0; "
		    "top:0; "
		    "width:100%; "
		    "height:99%; "
		    "z-index:100000; "
		"}\n"
	    ) != 0)
		{
		mssError(0, "HTPAGE", "Failed to write CSS for pgstat.");
		goto err;
		
		}
	    
	    /** Write the pgstat DOM nodes. **/
	    htrAddBodyItemLayerStart(s,0,"pgstat",0, NULL);
	    htrAddBodyItem_va(s, "<body style=\"%STR\">", bgstr);
	    htrAddBodyItem   (s,
		"<table width='100%' height='100%' cellpadding=20>"
		    "<tr><td valign=top><img src='/sys/images/loading.gif' alt='loading'></td></tr>"
		"</table></body>\n"
	    );
	    htrAddBodyItemLayerEnd(s,0);
	    }

	if (htrAddStylesheetItem(s,
	    "\t\thtml { "
		"overflow:hidden; "
	    "}\n"
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write CSS for main <HTML> tag.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\tbody { "
		"overflow:hidden; "
		"%[font-size:%POSpx; %]"
		"%[font-family:%STR&CSSVAL; %]"
	    "}\n",
	    (font_size > 0), font_size,
	    (*font_name), font_name
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write CSS for main <body> tag.");
	    goto err;
	    }
	if (htrAddStylesheetItem(s,
	    "\t\tpre {"
		"font-size:90%; "
	    "}\n"
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write CSS for all <pre> tags.");
	    goto err;
	    }

	/** Write trans divs. **/
	bool trans_success = false;
	#define HT_IMG_TAGS "src='/sys/images/trans_1.gif' alt=''"
	if (htrAddBodyItem(s, "<div id='pgtop'  class='pg pgclip'><img "HT_IMG_TAGS" width='1152' height='1'  ></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgbtm'  class='pg pgclip'><img "HT_IMG_TAGS" width='1152' height='1'  ></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgrgt'  class='pg pgclip'><img "HT_IMG_TAGS" width='1'    height='864'></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pglft'  class='pg pgclip'><img "HT_IMG_TAGS" width='1'    height='864'></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgktop' class='pg pgclip'><img "HT_IMG_TAGS" width='1152' height='1'  ></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgkbtm' class='pg pgclip'><img "HT_IMG_TAGS" width='1152' height='1'  ></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgkrgt' class='pg pgclip'><img "HT_IMG_TAGS" width='1'    height='864'></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgklft' class='pg pgclip'><img "HT_IMG_TAGS" width='1'    height='864'></div>\n") != 0) goto err_trans;
	if (htrAddBodyItem(s, "<div id='pgtvl'  class='pg'></div>\n") != 0) goto err_trans;
	trans_success = true;
	#undef HT_IMG_TAGS
	
    err_trans:
        /** Handle errors. **/
	if (!trans_success)
	    {
	    mssError(0, "HTPAGE", "Failed to write page trans div.");
	    goto err;
	    }

	/** Write ping HTML. **/
	if (htrAddBodyItemLayerStart(s, HTR_LAYER_F_DYNAMIC, "pgping", 0, NULL) != 0
	    || htrAddBodyItemLayerEnd(s, HTR_LAYER_F_DYNAMIC) != 0
	    || htrAddBodyItem(s, "\n") != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write HTML for page ping DOM node.");
	    goto err;
	    }
	
	/** Write message HTML. **/
	if (htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"pgmsg",0, NULL) != 0
	    || htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC) != 0
	    || htrAddBodyItem(s, "\n") != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write HTML for page message DOM node.");
	    goto err;
	    }

	/** Write JS call to initialize the pinging system. **/
	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	if (htrAddScriptInit_va(s,
	    "\tpg_ping_init(htr_subel(wgtrGetNodeRef(ns, '%STR&SYM'), 'pgping'), %INT);\n",
	    name, watchdogtimer * 500
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write JS ping init call.");
	    goto err;
	    }


	/** Write event handling functions. **/
	if (htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "pg", "pg_contextmenu") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "KEYDOWN",     "pg", "pg_keydown")     != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "KEYPRESS",    "pg", "pg_keypress")    != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "KEYUP",       "pg", "pg_keyup")       != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN",   "pg", "pg_mousedown")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE",   "pg", "pg_mousemove")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",    "pg", "pg_mouseout")    != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER",   "pg", "pg_mouseover")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",     "pg", "pg_mouseup")     != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "SCROLL",      "pg", "pg_scroll")      != 0) goto err;

	/** Set colors for the focus layers **/
	if (htrAddScriptInit_va(s,
	    "\tpage.kbcolor1 = '%STR&JSSTR';\n"
	    "\tpage.kbcolor2 = '%STR&JSSTR';\n"
	    "\tpage.mscolor1 = '%STR&JSSTR';\n"
	    "\tpage.mscolor2 = '%STR&JSSTR';\n"
	    "\tpage.dtcolor1 = '%STR&JSSTR';\n"
	    "\tpage.dtcolor2 = '%STR&JSSTR';\n",
	    kbfocus1, kbfocus2,
	    msfocus1, msfocus2,
	    dtfocus1, dtfocus2
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write JS to set focus box colors.");
	    goto err;
	    }

	/** Write code to ensure cursor updating starts. **/
	if (htrAddScriptInit(s, "\tpg_togglecursor();\n") != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write JS call to start cursor updating.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z) != 0) goto err;

	/** Write JS to signal that the page is done loading. **/
	if (htrAddScriptInit(s,
	    "\tif(typeof(pg_status_close) === 'function') pg_status_close();\n"
	    "\tpg_loadqueue_busy = false;\n"
	    "\tpg_serialized_load_doone();\n"
	) != 0)
	    {
	    mssError(0, "HTPAGE", "Failed to write JS to signal that loading is done.");
	    goto err;
	    }
	
	/** Success. **/
	return 0;

    err:
	mssError(0, "HTPAGE",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
    }

/*** htpageInitialize - register with the ht_render module.
 ***/
int
htpageInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Page Driver");
	strcpy(drv->WidgetName,"page");
	drv->Render = htpageRender;
	/** Actions **/
	htrAddAction(drv, "LoadPage");
	htrAddParam(drv, "LoadPage", "Source", DATA_T_STRING);
	htrAddAction(drv, "Launch");
	htrAddParam(drv, "Launch", "Source", DATA_T_STRING);
	htrAddParam(drv, "Launch", "Width", DATA_T_INTEGER);
	htrAddParam(drv, "Launch", "Height", DATA_T_INTEGER);
	htrAddParam(drv, "Launch", "Name", DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");


    return 0;
    }
