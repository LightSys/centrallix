#include "net_http.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_http.h, net_http.c, net_http_conn.c, net_http_sess.c, net_http_osml.c, net_http_app.c			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing an HTTP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/


/* This file will eventually contain much of the application logic from the GET and OSML fucntions */
/* 11:38 AM 6/25/2008 jncraton */


/*** nht_internal_GetGeom() - deploy a snippet of javascript to the browser
 *** to fetch the window geometry and reload the application.
 ***/
int
nht_internal_GetGeom(pObject target_obj, pFile output)
    {
    char bgnd[128];
    char* ptr;
    int font_size = -1;
    char font_name[128];

	/** Font sizes have an impact here **/
	objGetAttrValue(target_obj, "font_size", DATA_T_INTEGER, POD(&font_size));
	if (font_size < 5 || font_size > 100) font_size = -1;
	if (objGetAttrValue(target_obj, "font_name", DATA_T_STRING, POD(&ptr)) == 0 && ptr)
	    strtcpy(font_name, ptr, sizeof(font_name));
	else
	    strcpy(font_name, "");

	/** Do we have a bgcolor / background? **/
	if (objGetAttrValue(target_obj, "bgcolor", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    snprintf(bgnd, sizeof(bgnd), "bgcolor='%.100s'", ptr);
	    }
	else if (objGetAttrValue(target_obj, "background", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    snprintf(bgnd, sizeof(bgnd), "background='%.100s'", ptr);
	    }
	else
	    {
	    strcpy(bgnd, "bgcolor='white'");
	    }

	/** Generate the snippet **/
	fdQPrintf(output,"<html>\n"
			 "<head>\n"
			 "    <meta http-equiv=\"Pragma\" CONTENT=\"no-cache\">\n"
			 "    <style type=\"text/css\">\n"
			 "        #l1 { POSITION:absolute; VISIBILITY: hidden; left:0px; top:0px; }\n"
			 "        #l2 { POSITION:absolute; VISIBILITY: hidden; left:0px; top:0px; }\n"
			 "        body { %[font-size:%POSpx; %]%[font-family:%STR&CSSVAL; %]}\n"
			 "    </style>\n"
			 "<script type=\"text/javascript\" language=\"javascript\" src=\"/sys/js/startup.js\"></script>\n"
			 "</head>\n"
			 "<body %STR onload='startup();'>\n"
			 "    <img src='/sys/images/loading.gif'>\n"
			 "    <div id=\"l1\">x<br>x</div>\n"
			 "    <div id=\"l2\">xx</div>\n"
			 "</body>\n"
			 "</html>\n", font_size > 0, font_size, *font_name, font_name, bgnd);

    return 0;
    }

int
nhtRenderApp(pFile output, pObjSession s, pObject obj, pStruct url_inf, pWgtrClientInfo client_info, char* method, pNhtSessionData nsess)
    {
    pWgtrNode tree;
    int rval;
    int i = 0;

#if 00
    if (strncmp(url_inf->StrVal, "/INTERNAL/cache", 15))
	{
#endif
	if(! (tree = wgtrParseOpenObject(obj, url_inf, client_info->Templates)))
	    {
	    if(tree) wgtrFree(tree);
	    return -1;
	    }
	if(! (wgtrMergeOverlays(tree, objGetPathname(obj), client_info->AppPath, client_info->Overlays, client_info->Templates) >= 0))
	    {
	    if(tree) wgtrFree(tree);
	    return -1;
	    }

#if 00
	/** cache the app **/
	pCachedApp pca = (pCachedApp) nmSysMalloc(sizeof(CachedApp));
	pca->Node = tree;
	pca->Key = NHT.numbCachedApps;
	NHT.numbCachedApps++;
	xhAdd(nsess->CachedApps, (void*)&pca->Key, (void*)pca);
	}
    else
	{
	tree = ((pCachedApp)xhLookup(nsess->CachedApps, (void*)&i))->Node; //TODO: caching is not fully implemented
	}
#endif

    if(! (wgtrVerify(tree, client_info) >= 0))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}

    rval = wgtrRender(output, s, tree, url_inf, client_info, method);

    if(tree) wgtrFree(tree);
    //if(tree) wgtrFree(tree); //by Seth: because all trees are being cached, the trees must be freed somewhere else. Probably at the closing of the session. //SETH: ?? is that already taken care of?
    return rval;

    }

