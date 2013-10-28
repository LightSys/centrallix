/* vim: set sw=3: */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2007 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_osrc.c                                            */
/* Author:      John Peebles & Joe Heth                                 */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for an object system                 */
/************************************************************************/


/** globals **/
static struct {
   int     idcnt;
} HTOSRC;

enum htosrc_autoquery_types { Unset=-1, Never=0, OnLoad=1, OnFirstReveal=2, OnEachReveal=3  };

#if 00
/*** AddRule: add a declarative osrc rule to the generated document.
 ***/
int
htosrc_internal_AddRule(pHtSession s, pWgtrNode tree, char* treename, pWgtrNode sub_tree)
    {
    char ruletype[32];
    int query_delay;
    int min_chars;
    int trailing_wildcard;
    int leading_wildcard;
    char fieldname[64];
    char* ptr;
    pExpression code;

	/** Get type of rule **/
	if (wgtrGetPropertyValue(sub_tree, "ruletype", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1, "HTOSRC", "ruletype is required for widget/osrc-rule");
	    return -1;
	    }
	strtcpy(ruletype, ptr, sizeof(ruletype));

	/** Handle the rule based on the type **/
	if (!strcmp(ruletype, "relationship"))
	    {
	    }
	else if (!strcmp(ruletype, "filter"))
	    {
	    trailing_wildcard = htrGetBoolean(sub_tree, "trailing_wildcard", 1);
	    leading_wildcard = htrGetBoolean(sub_tree, "leading_wildcard", 0);
	    if (wgtrGetPropertyValue(sub_tree, "min_chars", DATA_T_INTEGER, POD(&min_chars)) != 0)
		min_chars = 3;
	    if (wgtrGetPropertyValue(sub_tree, "query_delay", DATA_T_INTEGER, POD(&query_delay)) != 0)
		query_delay = 500;
	    if (wgtrGetPropertyValue(sub_tree, "fieldname", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(fieldname, ptr, sizeof(fieldname));
	    else
		{
		mssError(1, "HTOSRC", "fieldname is required for widget/osrc-rule of type 'filter'");
		return -1;
		}

	    /** Get target **/
	    if (wgtrGetPropertyType(sub_tree,"value") == DATA_T_CODE)
		{
		wgtrGetPropertyValue(sub_tree,"value",DATA_T_CODE,POD(&code));
		htrAddExpression(s, treename, wgtrGetDName(sub_tree), code);
		}

	    /** Write the init line **/
	    htrAddScriptInit_va(s, "    nodes[\"%STR&SYM\"].AddRule('%STR&SYM', {dname:'%STR&SYM', field:'%STR&ESCQ', qd:%INT, mc:%INT, tw:%INT, lw:%INT});\n",
		    treename, ruletype, wgtrGetDName(sub_tree), fieldname, 
		    query_delay, min_chars, trailing_wildcard, leading_wildcard);
	    }
	else if (!strcmp(ruletype, "keying"))
	    {
	    }

    return 0;
    }
#endif

/* 
   htosrcRender - generate the HTML code for the page.
   
   Don't know what this is, but we're keeping it for now - JJP, JDH
*/
int
htosrcRender(pHtSession s, pWgtrNode tree, int z)
   {
   int id;
   char name[40];
   char *ptr;
   int readahead;
   int scrollahead;
   int replicasize;
   char *sql;
   char *filter;
   char *baseobj;
   pWgtrNode sub_tree;
//   pObjQuery qy;
   enum htosrc_autoquery_types aq;
   int receive_updates;
   int send_updates;
   int count, i;
   int ind_activity;
   int use_having;
   int qy_reveal_only;
   char key_objname[32];

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTOSRC","Netscape DOM or W3C DOM1 HTML support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTOSRC.idcnt++);

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(name,ptr,sizeof(name));

   if (wgtrGetPropertyValue(tree,"replicasize",DATA_T_INTEGER,POD(&replicasize)) != 0)
      replicasize=6;
   if (wgtrGetPropertyValue(tree,"readahead",DATA_T_INTEGER,POD(&readahead)) != 0)
      readahead=replicasize/2;
   if (wgtrGetPropertyValue(tree,"scrollahead",DATA_T_INTEGER,POD(&scrollahead)) != 0)
      scrollahead=readahead;

   /** do queries on this osrc indicate end-user activity? **/
   ind_activity = htrGetBoolean(tree, "indicates_activity", 1);

   /** use HAVING clause for query filtering instead of WHERE? **/
   use_having = htrGetBoolean(tree, "use_having_clause", 0);

   /** Delay query until osrc is visible? **/
   qy_reveal_only = htrGetBoolean(tree, "delay_query_until_reveal", 0);

   /** try to catch mistakes that would probably make Netscape REALLY buggy... **/
   if(replicasize==1 && readahead==0) readahead=1;
   if(replicasize==1 && scrollahead==0) scrollahead=1;
   if(readahead>replicasize) replicasize=readahead;
   if(scrollahead>replicasize) replicasize=scrollahead;
   if(scrollahead<1) scrollahead=1;
   if(replicasize<1 || readahead<1)
      {
      mssError(1,"HTOSRC","You must give positive integer for replicasize and readahead");
      return -1;
      }

   /** Name/abbreviation of key object in SQL query **/
   if (wgtrGetPropertyValue(tree,"key_objname",DATA_T_STRING,POD(&ptr)) != 0)
      ptr = "";
   strtcpy(key_objname, ptr, sizeof(key_objname));

   /** Query autostart types **/
   if (wgtrGetPropertyValue(tree,"autoquery",DATA_T_STRING,POD(&ptr)) == 0)
      {
      if (!strcasecmp(ptr,"onLoad")) aq = OnLoad;
      else if (!strcasecmp(ptr,"onFirstReveal")) aq = OnFirstReveal;
      else if (!strcasecmp(ptr,"onEachReveal")) aq = OnEachReveal;
      else if (!strcasecmp(ptr,"never")) aq = Never;
      else
	 {
	 mssError(1,"HTOSRC","Invalid autostart type '%s' for objectsource '%s'",ptr,name);
	 return -1;
	 }
      }
   else
      {
      aq = Unset;
      }

   /** Get replication updates from server? **/
   receive_updates = htrGetBoolean(tree, "receive_updates", 0);

   /** Send updates to the server? **/
   send_updates = htrGetBoolean(tree, "send_updates", 1);

   if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&ptr)) == 0)
      {
      sql = nmSysStrdup(ptr);
      }
   else
      {
      mssError(1,"HTOSRC","You must give a sql parameter");
      return -1;
      }

   if (wgtrGetPropertyValue(tree,"baseobj",DATA_T_STRING,POD(&ptr)) == 0)
      baseobj = nmSysStrdup(ptr);
   else
      baseobj = NULL;

   if (wgtrGetPropertyValue(tree,"filter",DATA_T_STRING,POD(&ptr)) == 0)
      filter = nmSysStrdup(ptr);
   else
      filter = nmSysStrdup("");

   /** create our instance variable **/
   htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"osrc%POSloader\")",id);
   htrAddWgtrCtrLinkage(s, tree, "_parentctr");

   htrAddScriptGlobal(s, "osrc_syncid", "0", 0);
   htrAddScriptGlobal(s, "osrc_relationships", "[]", 0);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"        #osrc%POSloader { overflow:hidden; POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:1px;  WIDTH:1px; HEIGHT:1px; Z-INDEX:0; }\n",id);

   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    osrc_init({loader:wgtrGetNodeRef(ns,\"%STR&SYM\"), readahead:%INT, scrollahead:%INT, replicasize:%INT, sql:\"%STR&JSSTR\", filter:\"%STR&JSSTR\", baseobj:\"%STR&JSSTR\", name:\"%STR&SYM\", autoquery:%INT, requestupdates:%INT, ind_act:%INT, use_having:%INT, qy_reveal_only:%INT, send_updates:%INT, key_objname:\"%STR&JSSTR\"});\n",
	 name,readahead,scrollahead,replicasize,sql,filter,
	 baseobj?baseobj:"",name,aq,receive_updates, ind_activity,
	 use_having, qy_reveal_only, send_updates, key_objname);
   //htrAddScriptCleanup_va(s,"    %s.layers.osrc%dloader.cleanup();\n", parentname, id);

   htrAddScriptInclude(s, "/sys/js/htdrv_osrc.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

   /** HTML body element for the frame **/
   htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"osrc%POSloader",id);
   htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
   htrAddBodyItem(s, "\n");

    count = xaCount(&(tree->Children));
    for (i=0;i<count;i++)
	{
	sub_tree = xaGetItem(&(tree->Children), i);
#if 00
	if (wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "widget/osrc-rule"))
	    {
	    htosrc_internal_AddRule(s, tree, name, sub_tree);
	    }
	else
	    {
#endif
	    htrRenderWidget(s, sub_tree, z);
#if 00
	    }
#endif
	}

    nmSysFree(filter);
    if (baseobj) nmSysFree(baseobj);
    nmSysFree(sql);

   return 0;
}


/* 
   htosrcInitialize - register with the ht_render module.
*/
int htosrcInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML OSRC Driver");
   strcpy(drv->WidgetName,"osrc");
   drv->Render = htosrcRender;

   /** Add actions **/
   htrAddAction(drv,"Clear");
   htrAddAction(drv,"Query");
   htrAddAction(drv,"Delete");
   htrAddAction(drv,"Create");
   htrAddAction(drv,"Modify");

   htrAddAction(drv,"Sync");
   htrAddAction(drv,"ReverseSync");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTOSRC.idcnt = 0;

   /** Rule types **/
   htruleRegister("osrc_filter",
		"value",		DATA_T_CODE,
		"trailing_wildcard",	HT_DATA_T_BOOLEAN,
		"leading_wildcard",	HT_DATA_T_BOOLEAN,
		"query_delay",		DATA_T_INTEGER,
		"fieldname",		DATA_T_STRING,
		"min_chars",		DATA_T_INTEGER,
		NULL);

   htruleRegister("osrc_relationship",
		"target",		DATA_T_STRING,
		"key_1",		DATA_T_STRING,
		"target_key_1",		DATA_T_STRING,
		"key_2",		DATA_T_STRING,
		"target_key_2",		DATA_T_STRING,
		"key_3",		DATA_T_STRING,
		"target_key_3",		DATA_T_STRING,
		"key_4",		DATA_T_STRING,
		"target_key_4",		DATA_T_STRING,
		"key_5",		DATA_T_STRING,
		"target_key_5",		DATA_T_STRING,
		"is_slave",		HT_DATA_T_BOOLEAN,
		"revealed_only",	HT_DATA_T_BOOLEAN,
		"enforce_create",	HT_DATA_T_BOOLEAN,
		"autoquery",		HT_DATA_T_BOOLEAN,
		"master_norecs_action",	DATA_T_STRING,		/* what to do if no record in master (allrecs, sameasnull, norecs) */
		"master_null_action",	DATA_T_STRING,		/* what to do if master key is null (allrecs, nullisvalue, norecs) */
		"key_objname",		DATA_T_STRING,
		"target_key_objname",	DATA_T_STRING,
		NULL);

   htruleRegister("osrc_key",
		"key_fieldname",	DATA_T_STRING,
		"keying_method",	DATA_T_STRING,		/* maxplusone, counter, counterosrc, sql, value */
		"value",		DATA_T_CODE,
		"min_value",		DATA_T_ANY,
		"max_value",		DATA_T_ANY,
		"object_path",		DATA_T_STRING,
		"counter_attribute",	DATA_T_STRING,
		"sql",			DATA_T_STRING,
		"osrc",			DATA_T_STRING,
		NULL);

   return 0;
}
