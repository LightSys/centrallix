#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "ht_render.h"
#include "obj.h"
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
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
    int show, i, count;
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
    pStruct c_param;
    time_t t;
    struct tm* timeptr;
    char timestr[80];
    XArray endorsements;
    XArray contexts;
    int max_requests = 1;

	if(!((s->Capabilities.Dom0NS || s->Capabilities.Dom0IE || (s->Capabilities.Dom1HTML && s->Capabilities.Dom2Events)) && s->Capabilities.CSS1) )
	    {
	    mssError(1,"HTPAGE","CSS Level 1 Support and (Netscape DOM support or (W3C Level 1 DOM support and W3C Level 2 Events support required))");
	    return -1;
	    }

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

	/** Page icon? **/
	if (wgtrGetPropertyValue(tree, "icon", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s, "\t<link rel='shortcut icon' href='%STR&HTE'>\n",ptr);
	    }

    	/** Check for a title. **/
	if (wgtrGetPropertyValue(tree, "title", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s, "\t<title>%STR&HTE</title>\n", ptr);
	    }

    	/** Check for page load status **/
	show = htrGetBoolean(tree, "loadstatus", 0);

	/** Initialize the html-related interface stuff **/
	if (ifcHtmlInit(s, tree) < 0)
	    {
	    mssError(0, "HTR", "Error Initializing Html Interface code...continuing, but things might not work for client");
	    }
	
	/** Auto-call startup and cleanup **/
	htrAddBodyParam_va(s, " onLoad=\"startup_%STR&SYM();\" onUnload=\"cleanup();\"",
		s->Namespace->DName);

	/** Check for bgcolor. **/
	if (htrGetBackground(tree, NULL, 1, bgstr, sizeof(bgstr)) == 0)
	    {
	    htrAddBodyParam_va(s, " style=\"%STR\"", bgstr);
	    }

	/** Check for text color **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " TEXT=\"%STR&HTE\"",ptr);
	    }

	/** Check for link color **/
	if (wgtrGetPropertyValue(tree,"linkcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " LINK=\"%STR&HTE\"",ptr);
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

	show_diag = htrGetBoolean(tree, "show_diagnostics", 0);
	dpi_scaling = htrGetBoolean(tree, "dpi_scaling", 0);

	wgtrGetPropertyValue(tree, "font_size", DATA_T_INTEGER, POD(&font_size));
	if (font_size < 5 || font_size > 100) font_size = 12;

	if (wgtrGetPropertyValue(tree, "font_name", DATA_T_STRING, POD(&ptr)) ==  0 && ptr)
	    strtcpy(font_name, ptr, sizeof(font_name));
	else
	    strcpy(font_name, "");

	/** Add global for page metadata **/
	htrAddScriptGlobal(s, "page", "{}", 0);

	/** Add a list of highlightable areas **/
	/** These are javascript global variables**/
	htrAddScriptGlobal(s, "pg_arealist", "[]", 0);
	htrAddScriptGlobal(s, "pg_keylist", "[]", 0);
	htrAddScriptGlobal(s, "pg_curarea", "null", 0);
	htrAddScriptGlobal(s, "pg_curlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdarea", "null", 0);
	htrAddScriptGlobal(s, "pg_lastkey", "-1", 0);
	htrAddScriptGlobal(s, "pg_lastmodifiers", "null", 0);
	htrAddScriptGlobal(s, "pg_keytimeoutid", "null", 0);
	htrAddScriptGlobal(s, "pg_keyschedid", "0", 0);
	htrAddScriptGlobal(s, "pg_modallayer", "null", 0);
	htrAddScriptGlobal(s, "pg_key_ie_shifted", "false", 0);
	htrAddScriptGlobal(s, "pg_attract", "null", 0);
	htrAddScriptGlobal(s, "pg_gshade", "null", 0);
	htrAddScriptGlobal(s, "pg_closetype", "null", 0);
	htrAddScriptGlobal(s, "pg_explist", "[]", 0);
	htrAddScriptGlobal(s, "pg_schedtimeout", "null", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutlist", "[]", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutid", "0", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutstamp", "0", 0);
	htrAddScriptGlobal(s, "pg_insame", "false", 0);
	htrAddScriptGlobal(s, "cn_browser", "null", 0);
	htrAddScriptGlobal(s, "ibeam_current", "null", 0);
	htrAddScriptGlobal(s, "util_cur_mainlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_loadqueue", "[]", 0);
	htrAddScriptGlobal(s, "pg_loadqueue_busy", "999999", 0);
	htrAddScriptGlobal(s, "pg_debug_log", "null", 0);
	htrAddScriptGlobal(s, "pg_isloaded", "false", 0);
	htrAddScriptGlobal(s, "pg_username", "null", 0);
	htrAddScriptGlobal(s, "pg_msg_handlers", "[]", 0);
	htrAddScriptGlobal(s, "pg_msg_layer", "null", 0);
	htrAddScriptGlobal(s, "pg_msg_timeout", "null", 0);
	htrAddScriptGlobal(s, "pg_diag", show_diag?"true":"false", 0); /* causes pop-up boxes for certain non-fatel warnings */
	htrAddScriptGlobal(s, "pg_dpi_scaling", (dpi_scaling) ? "true" : "false", 0); /* Whether to scale launched windows based on display dots-per-inch */
	htrAddScriptGlobal(s, "pg_width", "0", 0);
	htrAddScriptGlobal(s, "pg_height", "0", 0);
	htrAddScriptGlobal(s, "pg_charw", "0", 0);
	htrAddScriptGlobal(s, "pg_charh", "0", 0);
	htrAddScriptGlobal(s, "pg_parah", "0", 0);
	htrAddScriptGlobal(s, "pg_namespaces", "{}", 0);
	htrAddScriptGlobal(s, "pg_handlertimeout", "null", 0); /* this is used by htr_mousemovehandler */
	htrAddScriptGlobal(s, "pg_mousemoveevents", "[]", 0);
	htrAddScriptGlobal(s, "pg_handlers", "[]", 0); /* keeps track of handlers for basic events tied to document (click, mousemove, keypress, etc) */
	htrAddScriptGlobal(s, "pg_capturedevents", "0", 0); /* is the binary OR of all event flags that the document currently has registered */
	htrAddScriptGlobal(s, "pg_tiplayer", "null", 0);
	htrAddScriptGlobal(s, "pg_tipindex", "0", 0);
	htrAddScriptGlobal(s, "pg_tiptmout", "null", 0);
	htrAddScriptGlobal(s, "pg_waitlyr", "null", 0);
	htrAddScriptGlobal(s, "pg_appglobals", "[]", 0);
	htrAddScriptGlobal(s, "pg_sessglobals", "[]", 0);
	htrAddScriptGlobal(s, "pg_scripts", "[]", 0);
	htrAddScriptGlobal(s, "pg_endorsements", "[]", 0);
	htrAddScriptGlobal(s, "pg_max_requests", "1", 0);

	/** Add script include to get function declarations **/
	if(s->Capabilities.JS15 && s->Capabilities.Dom1HTML)
	    {
	    /*htrAddScriptInclude(s, "/sys/js/htdrv_page_js15.js", 0);*/
	    }
	htrAddScriptInclude(s, "/sys/js/htdrv_page.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/jquery/jquery-1.11.1.js", 0);

	/** Write named global **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	htrAddWgtrObjLinkage(s, tree, "window");
	htrAddWgtrCtrLinkage(s, tree, "document");

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
		htrAddScriptInit_va(s, "\tpg_servertime = new Date(%STR&DQUOT);\n", timestr);
		if (strftime(timestr, sizeof(timestr), "%Y %m %d %T", timeptr) > 0)
		    htrAddScriptInit_va(s, "\tpg_servertime_notz = new Date(%STR&DQUOT);\n", timestr);
		htrAddScriptInit_va(s, "\tpg_clienttime = new Date();\n");
		}
	    }

	/** Page init **/
	htrAddScriptInit   (s, "\tif (typeof(pg_status_init) === 'function') pg_status_init();\n");
	htrAddScriptInit_va(s, "\tpg_init(wgtrGetNodeRef(ns,'%STR&SYM'),%INT);\n", name, attract);
	htrAddScriptInit_va(s, "\tpg_username = '%STR&JSSTR';\n", mssUserName());
	htrAddScriptInit_va(s, "\tpg_width = %INT;\n", w);
	htrAddScriptInit_va(s, "\tpg_height = %INT;\n", h);
	htrAddScriptInit_va(s, "\tpg_max_requests = %INT;\n", max_requests);
	htrAddScriptInit_va(s, "\tpg_charw = %INT;\n", s->ClientInfo->CharWidth);
	htrAddScriptInit_va(s, "\tpg_charh = %INT;\n", s->ClientInfo->CharHeight);
	htrAddScriptInit_va(s, "\tpg_parah = %INT;\n", s->ClientInfo->ParagraphHeight);

	c_param = stLookup_ne(s->Params, "cx__obscure");
	if (c_param && !strcasecmp(c_param->StrVal,"yes"))
	    htrAddScriptInit(s, "\tobscure_data = true;\n");
	else
	    htrAddScriptInit(s, "\tobscure_data = false;\n");

	/** Add template paths **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    {
	    /** Ignore null templates. **/
	    path = wgtrGetTemplatePath(tree, i);
	    if (path == NULL) continue;
	    
	    /** Write code for the template. **/
	    htrAddScriptInit_va(s,
		"\twgtrGetNodeRef(ns,'%STR&SYM').templates.push('%STR&JSSTR');\n",
		name, path
	    );
	    }

	/** Endorsements **/
	xaInit(&endorsements,16);
	xaInit(&contexts,16);
	cxssGetEndorsementList(&endorsements, &contexts);
	for(i=0;i<endorsements.nItems;i++)
	    {
	    htrAddScriptInit_va(s,
		"\tpg_endorsements.push({ e:'%STR&JSSTR', ctx:'%STR&JSSTR' });\n",
		(char*)endorsements.Items[i], (char*)contexts.Items[i]
	    );
	    nmSysFree((char*)endorsements.Items[i]);
	    nmSysFree((char*)contexts.Items[i]);
	    }
	xaDeInit(&endorsements);
	xaDeInit(&contexts);

	/** Shutdown **/
	htrAddScriptCleanup(s, "\tpg_cleanup();\n");

	/** Add focus box. **/
	if(s->Capabilities.HTML40)
	    {
	    htrAddStylesheetItem(s,
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
	    );
	    }
	else
	    {
	    htrAddStylesheetItem(s,
		"\t\t.pg     { position:absolute; visibility:hidden; }\n"
		"\t\t.pgclip { clip:rect(1,1); z-index:1000; }\n"
		"\t\t#pgtop  { left:0; top:0;  width:1152; height:1;   }\n"
		"\t\t#pgbtm  { left:0; top:0;  width:1152; height:1;   }\n"
		"\t\t#pgrgt  { left:0; top:0;  width:1;    height:864; }\n"
		"\t\t#pglft  { left:0; top:0;  width:1;    height:864; }\n"
		"\t\t#pgktop { left:0; top:0;  width:1152; height:1;   }\n"
		"\t\t#pgkbtm { left:0; top:0;  width:1152; height:1;   }\n"
		"\t\t#pgkrgt { left:0; top:0;  width:1;    height:864; }\n"
		"\t\t#pgklft { left:0; top:0;  width:1;    height:864; }\n"
		"\t\t#pgtvl  { left:0; top:0;  width:1;    height:1;   z-index:0;  }\n"
		"\t\t#pginpt { left:0; top:20;                         z-index:20; }\n"
		"\t\t#pgping { left:0; top:0;                          z-index:0;  }\n"
		"\t\t#pgmsg  { left:0; top:0;                          z-index:0;  }\n"
	    );
	    }

	if (show == 1)
	    {
	    htrAddStylesheetItem(s, "\t\t#pgstat { position:absolute; visibility:visible; left:0;top:0;width:100%;height:99%; z-index:100000;}\n");
	    
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

	htrAddStylesheetItem_va(s,
	    "\t\thtml { "
		"overflow:hidden; "
	    "}\n"
	);
	htrAddStylesheetItem_va(s,
	    "\t\tbody { "
		"overflow:hidden; "
		"%[font-size:%POSpx; %]"
		"%[font-family:%STR&CSSVAL; %]"
	    "}\n",
	    (font_size > 0), font_size,
	    (*font_name), font_name
	);
	htrAddStylesheetItem(s,
	    "\t\tpre {"
		"font-size:90%; "
	    "}\n"
	);

	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,
		"\t\ttd, font { "
		    "%[font-size:%POSpx; %]"
		    "%[font-family:%STR&CSSVAL; %]"
		"}\n",
		(font_size > 0), font_size,
		(*font_name), font_name
	    );
	    }

	htrAddBodyItem(s, "<div id='pgtop'  class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1152' height='1'   alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgbtm'  class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1152' height='1'   alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgrgt'  class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1'    height='864' alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pglft'  class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1'    height='864' alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgktop' class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1152' height='1'   alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgkbtm' class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1152' height='1'   alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgkrgt' class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1'    height='864' alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgklft' class='pg pgclip'><img src='/sys/images/trans_1.gif' width='1'    height='864' alt=''></div>\n");
	htrAddBodyItem(s, "<div id='pgtvl'  class='pg'></div>\n");

	htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"pgping",0, NULL);
	htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
	htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"pgmsg",0, NULL);
	htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
	htrAddBodyItem(s, "\n");

	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	htrAddScriptInit_va(s, "\tpg_ping_init(htr_subel(wgtrGetNodeRef(ns, '%STR&SYM'), 'pgping'), %INT);\n", name, watchdogtimer/2*1000);

	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "pg", "pg_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "pg", "pg_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "pg", "pg_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "pg", "pg_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "pg", "pg_mouseup");
	htrAddEventHandlerFunction(s, "document", "SCROLL", "pg", "pg_scroll");
	if (s->Capabilities.Dom1HTML)
	    htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "pg", "pg_contextmenu");

	/** W3C DOM Level 2 Event model doesn't require a textbox to get keystrokes **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");
	    htrAddEventHandlerFunction(s, "document", "MOUSEUP", "pg2", "pg_mouseup_ns4");
	    }

	/** Set colors for the focus layers **/
	htrAddScriptInit_va(s,
	    "\tpage.kbcolor1 = '%STR&JSSTR';\n"
	    "\tpage.kbcolor2 = '%STR&JSSTR';\n"
	    "\tpage.mscolor1 = '%STR&JSSTR';\n"
	    "\tpage.mscolor2 = '%STR&JSSTR';\n"
	    "\tpage.dtcolor1 = '%STR&JSSTR';\n"
	    "\tpage.dtcolor2 = '%STR&JSSTR';\n",
	    kbfocus1, kbfocus2,
	    msfocus1, msfocus2,
	    dtfocus1, dtfocus2
	);

	/** Write code to ensure cursor updating starts. **/
	htrAddScriptInit(s, "\tpg_togglecursor();\n");

	/** Write event handling functions. **/
	htrAddEventHandlerFunction(s, "document", "KEYDOWN", "pg", "pg_keydown");
	htrAddEventHandlerFunction(s, "document", "KEYUP", "pg", "pg_keyup");
	htrAddEventHandlerFunction(s, "document", "KEYPRESS", "pg", "pg_keypress");

	/** create the root node of the wgtr **/
	count = xaCount(&(tree->Children));
	for (i=0;i<count;i++)
	    {
	    htrRenderWidget(s, xaGetItem(&(tree->Children),i), z+1);
	    }

	/** keyboard input for NS4 **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit(s,
		"\tsetTimeout(pg_mvpginpt, 1, document.layers.pginpt);\n"
		"\tdocument.layers.pginpt.moveTo(window.innerWidth-2, 20);\n"
		"\tdocument.layers.pginpt.visibility = 'inherit';\n"
		"\tdocument.layers.pginpt.document.tmpform.x.focus();\n"
	    );
	    }

	htrAddScriptInit(s,
	    "\tif(typeof(pg_status_close) === 'function') pg_status_close();\n"
	    "\tpg_loadqueue_busy = false;\n"
	    "\tpg_serialized_load_doone();\n"
	);
	
	return 0;
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
