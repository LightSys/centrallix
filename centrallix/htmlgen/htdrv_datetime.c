#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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
/* Module: 	htdrv_datetime.c        				*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	June 26, 2002						*/
/* Description:	HTML driver for a 'date time' widget			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_datetime.c,v 1.8 2002/07/15 20:36:32 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_datetime.c,v $

    $Log: htdrv_datetime.c,v $
    Revision 1.8  2002/07/15 20:36:32  lkehresman
    Added another check for blank or invalid dates

    Revision 1.7  2002/07/15 18:16:39  lkehresman
    * Removed some flickering
    * Fixed a couple minor bugs with invalid dates

    Revision 1.6  2002/07/12 14:56:27  lkehresman
    Added a *simple* fix for invalid dates stored in the database.  This needs
    to be expanded in the future, but now it at least doesn't throw javascript
    errors any more.

    Revision 1.5  2002/07/12 14:24:54  lkehresman
    Added form interaction with the datetime widget.  Works with the Kardia demo!!

    Revision 1.4  2002/07/10 20:54:29  lkehresman
    Corrected the leapyear detection

    Revision 1.3  2002/07/10 20:18:01  lkehresman
    Added time controls

    Revision 1.2  2002/07/09 18:58:49  lkehresman
    Added double buffering to the datetime widget to reduse flickering

    Revision 1.1  2002/07/09 14:09:04  lkehresman
    Added first revision of the datetime widget.  No form interatction, and no
    time setting functionality, only date.  This has been on my laptop for a
    while and I wanted to get it into CVS for backup purposes.  More functionality
    to come soon.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTDT;


/*** htdtVerify - not written yet.
 ***/
int
htdtVerify()
    {
    return 0;
    }


/*** htdtRender - generate the HTML code for the page.
 ***/
int
htdtRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char *sql;
    char *str;
    char *attr;
    char name[128];
    char initialdate[64];
    char fgcolor[64];
    char bgcolor[128];
    char fieldname[30];
    int type, rval;
    int x,y,w,h;
    int id;
    DateTime dt;
    ObjData od;
    pObject qy_obj;
    pObjQuery qy;

	/** Get an id for this. **/
	id = (HTDT.idcnt++);

	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'height' property");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
	    strncpy(fieldname,ptr,30);
	else 
	    fieldname[0]='\0';

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get initial date **/
	if (objGetAttrValue(w_obj, "sql", POD(&sql)) == 0) 
	    {
	    if ((qy = objMultiQuery(w_obj->Session, sql))) 
		{
		while ((qy_obj = objQueryFetch(qy, O_RDONLY)))
		    {
		    attr = objGetFirstAttr(qy_obj);
		    if (!attr)
			{
			mssError(1, "HTDT", "There was an error getting date from your SQL query");
			return -1;
			}
		    type = objGetAttrType(qy_obj, attr);
		    rval = objGetAttrValue(qy_obj, attr, &od);
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    snprintf(initialdate, 64, "%s", str);
		    objClose(qy_obj);
		    }
		objQueryClose(qy);
		}
	    }
	else if (objGetAttrValue(w_obj,"initialdate",POD(&ptr)) == 0)
	    {
	    memccpy(initialdate, ptr, '\0', 63);
	    }
	else
	    {
	    initialdate[0]='\0';
	    }
	if (strlen(initialdate))
	    {
	    printf("ID: '%s'\n", initialdate);
	    objDataToDateTime(DATA_T_STRING, initialdate, &dt, NULL);
	    snprintf(initialdate, 64, "%s %d %d, %d:%d:%d", obj_short_months[dt.Part.Month], 
	                          dt.Part.Day+1,
	                          dt.Part.Year+1900,
	                          dt.Part.Hour,
	                          dt.Part.Minute,
	                          dt.Part.Second);
	    }
	

	/** Get colors **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(bgcolor,"bgcolor=%.100s",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(bgcolor,"background='%.90s'",ptr);
	else
	    strcpy(bgcolor,"bgcolor=#c0c0c0");
	if (objGetAttrValue(w_obj,"fgcolor",POD(&ptr)) == 0)
	    sprintf(fgcolor,"%.63s",ptr);
	else
	    strcpy(fgcolor,"black");

	/** Ok, write the style header items. **/
	htrAddHeaderItem(s,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem_va(s,"\t#dt%dpane1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	htrAddHeaderItem_va(s,"\t#dt%dbg1   { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:0; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w,h,z+1);
	htrAddHeaderItem_va(s,"\t#dt%dbg2   { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-1,h-1,z+2);
	htrAddHeaderItem_va(s,"\t#dt%dbody  { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+3);
	htrAddHeaderItem_va(s,"\t#dt%dimg   { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:1; WIDTH:18; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+4);
	htrAddHeaderItem_va(s,"\t#dt%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:182; HEIGHT:190; Z-INDEX:%d; }\n",id,x,y+h,w,z+5);
	htrAddHeaderItem(s,"    </STYLE>\n");

	/** Write named global **/
	//nptr = (char*)nmMalloc(strlen(name)+1);
	sprintf(name, "%s.layers.dt%dpane", parentname, id);
	//htrAddScriptGlobal(s, name, "null", HTR_F_NAMEALLOC);
	htrAddScriptGlobal(s, "dt_current", "null", 0);

	/** Get Value function **/
	htrAddScriptFunction(s, "dt_getvalue", "\n"
		"function dt_getvalue() {\n"
		"   return dt_formatdate(this, this.wdate, 0);\n"
		"   return str;\n"
		"}\n", 0);

	/** Set Value function **/
	htrAddScriptFunction(s, "dt_setvalue", "\n"
		"function dt_setvalue(v) {\n"
		"   this.wdate = new Date(v);\n"
		"   this.tmpdate = new Date(v);\n"
		"   if (this.wdate == 'Invalid Date')\n"
		"      {\n"
		"      this.wdate = new Date();\n"
		"      this.tmpdate = new Date();\n"
		"      }\n"
		"   dt_drawdate(this.lbdy, this.tmpdate);\n"
		"   dt_drawmonth(this.ld, this.tmpdate);\n"
		"   dt_drawtime(this, this.tmpdate);\n"
		"}\n", 0);

	/** Clear Value function **/
	htrAddScriptFunction(s, "dt_clearvalue", "\n"
		"function dt_clearvalue() {\n"
		"   this.wdate = new Date('');\n"
		"   this.tmpdate = new Date('');\n"
		"   dt_drawdate(this.lbdy, '');\n"
		"}\n", 0);

	/** Reset Value function **/
	htrAddScriptFunction(s, "dt_resetvalue", "\n"
		"function dt_resetvalue() {\n"
		"   this.wdate = new Date(this.initialdateStr);\n"
		"   this.tmpdate = new Date(this.initialdateStr);\n"
		"   dt_drawdate(this.lbdy, this.tmpdate);\n"
		"}\n", 0);

	/** Enable function **/
	htrAddScriptFunction(s, "dt_enable", "\n"
		"function dt_enable() {\n"
		"   this.enabled = 'full';\n"
		"}\n", 0);

	/** Read-Only function **/
	htrAddScriptFunction(s, "dt_readonly", "\n"
		"function dt_readonly() {\n"
		"   this.enabled = 'readonly';\n"
		"}\n", 0);

	/** Disable function **/
	htrAddScriptFunction(s, "dt_disable", "\n"
		"function dt_disable() {\n"
		"   this.enabled = 'disabled';\n"
		"}\n", 0);

	htrAddScriptFunction(s, "dt_strpad", "\n"
		"function dt_strpad(str, pad, len)\n"
		"    {\n"
		"    str = new String(str);\n"
		"    for (var i=0; i < len-str.length; i++)\n"
		"        str = pad+str;\n"
		"    return str;\n"
		"    }\n", 0);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "dt_init", "\n"
		"function dt_init(l,lbg1,lbg2,lbdy,limg,l2,w,h,dt,bgcolor,fgcolor,fn)\n"
		"    {\n"
		"    this.enabled = 'full';\n"
		"    l.fieldname = fn;\n"
		"    l.setvalue = dt_setvalue;\n"
		"    l.getvalue = dt_getvalue;\n"
		"    l.enable = dt_enable;\n"
		"    l.readonly = dt_readonly;\n"
		"    l.disable = dt_disable;\n"
		"    l.clearvalue = dt_clearvalue;\n"
		"    l.resetvalue = dt_resetvalue;\n"
		"    l.form = fm_current;\n"
		"    l.kind = 'dt';\n"
		"    lbg1.kind = 'dt';\n"
		"    lbg2.kind = 'dt';\n"
		"    lbdy.kind = 'dt';\n"
		"    limg.kind = 'dt';\n"
		"    l2.kind = 'dt';\n"
		"    limg.document.images[0].kind = 'dt';\n"
		"    limg.document.images[0].subkind = 'dt_button';\n"
		"    limg.document.images[0].layer = l;\n"
		"    lbdy.subkind = 'dt_button';\n"
		"    lbg1.subkind = 'dt_button';\n"
		"    lbg2.subkind = 'dt_button';\n"
		"    lbdy.subkind = 'dt_button';\n"
		"    limg.subkind = 'dt_button';\n"
		"    l2.subkind = 'dt_dropdown';\n"
		"    l.document.layer = l;\n"
		"    lbg1.document.layer = l;\n"
		"    lbg2.document.layer = l;\n"
		"    lbdy.document.layer = l;\n"
		"    limg.document.layer = l;\n"
		"    l2.document.layer = l2;\n"
		"    l2.document.mainlayer = l;\n"
		"    l.initialdateStr = dt;\n"
		"    if (dt) l.wdate = new Date(dt);\n"
		"    else l.wdate = new Date();\n"
		"    l.tmpdate = new Date(l.wdate);\n"
		"    l.std_w = 182;\n"
		"    l.std_h = 190;\n"
		"    l.w = w;\n"
		"    l.h = h;\n"
		"    l.ld = l2;\n"
		"    l.lbg1 = lbg1;\n"
		"    l.lbg2 = lbg2;\n"
		"    l.lbdy = lbdy;\n"
		"    l.limg = limg;\n"
		"    l.colorBG = bgcolor;\n"
		"    l.colorFG = fgcolor;\n"
		"    l.mode = 0;\n"
		"    l.ld.contentLayer = new Layer(1024, l.ld);\n"
		"    l.ld.contentLayer.visibility = 'inherit';\n"
		"    l.ld.hiddenLayer = new Layer(1024, l.ld);\n"
		"    l.ld.hiddenLayer.visibility = 'hide';\n"
		"    l.strMonthsAbbrev = Array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');\n"
		"    l.strDaysAbbrev = Array('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat');\n"
		"    l.ld.contentLayer.dayLayersArray = Array();\n"
		"    l.ld.hiddenLayer.dayLayersArray = Array();\n"
		"    l.vl = l.ld.contentLayer;\n" // visible layer
		"    l.hl = l.ld.hiddenLayer;\n" // hidden layer
		"    l.vl.clip.height=500; l.vl.clip.width=500;\n"
		"    l.hl.clip.height=500; l.hl.clip.width=500;\n"
		"    l.ld.tl_hr = new Layer(1024, l.ld);\n" // hour display layer
		"    l.ld.tl_hr.x = 3; l.ld.tl_hr.y = 168;\n"
		"    l.ld.tl_hr.clip.width = 70; l.ld.tl_hr.clip.height=20;\n"
		"    l.ld.tl_hr.visibility = 'inherit';\n"
		"    l.ld.tl_mn = new Layer(1024, l.ld);\n" // minute display layer
		"    l.ld.tl_mn.x = 76; l.ld.tl_mn.y = 168;\n"
		"    l.ld.tl_mn.clip.width = 70; l.ld.tl_mn.clip.height=20;\n"
		"    l.ld.tl_mn.visibility = 'inherit';\n"
		"    for (var i=0; i < 31; i++)\n"
		"        {\n"
		"        l.ld.contentLayer.dayLayersArray[i] = new Layer(128, l.ld.contentLayer);\n"
		"        l.ld.contentLayer.dayLayersArray[i].losefocushandler = dt_losefocus;\n"
		"        l.ld.contentLayer.dayLayersArray[i].document.mainlayer = l;\n"
		"        l.ld.contentLayer.dayLayersArray[i].document.layer = l.ld.contentLayer.dayLayersArray[i];\n"
		"        l.ld.contentLayer.dayLayersArray[i].kind = 'dt';\n"
		"        l.ld.contentLayer.dayLayersArray[i].visibility = 'inherit';\n"
		"        l.ld.contentLayer.dayLayersArray[i].subkind = 'dt_day';\n"

		"        l.ld.hiddenLayer.dayLayersArray[i] = new Layer(128, l.ld.hiddenLayer);\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].losefocushandler = dt_losefocus;\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].document.mainlayer = l;\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].document.layer = l.ld.hiddenLayer.dayLayersArray[i];\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].kind = 'dt';\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].visibility = 'inherit';\n"
		"        l.ld.hiddenLayer.dayLayersArray[i].subkind = 'dt_day';\n"
		"        }\n"
		"    l.ld.hdr = new Layer(1024, l.ld);\n"
		"    l.ld.hdr1 = new Layer(1024, l.ld);\n"
		"    l.ld.hdr2 = new Layer(1024, l.ld);\n"
		"    l.ld.hdr3 = new Layer(1024, l.ld);\n"
		"    l.ld.hdr4 = new Layer(1024, l.ld);\n"
		"    dt_drawmonth(l.ld, l.tmpdate);\n"
		"    if (l.initialdateStr) dt_drawdate(l.lbdy, l.tmpdate);\n"
		"    dt_drawtime(l, l.tmpdate);\n"
		"    if (fm_current) fm_current.Register(l);\n"
		"    }\n" ,0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    dt_init("
	                       "%s.layers.dt%dpane1, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbg1, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbg2, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbody, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dimg, "
	                       "%s.layers.dt%dpane2, "
	                       "%d,%d,\"%s\",\"%s\",\"%s\",\"%s\")\n",
			parentname,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id, 
			w,h, initialdate, bgcolor, fgcolor, fieldname);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s, /* Date Display Button */ 
		"<DIV ID=\"dt%dpane1\">\n"
		"<DIV ID=\"dt%dbg1\">\n"
		"  <BODY bgcolor=\"#ffffff\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD><IMG src=/sys/images/trans_1.gif height=1 width=1></TD></TR>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dbg2\">\n"
		"  <BODY bgcolor=\"#7a7a7a\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD><IMG src=/sys/images/trans_1.gif height=1 width=1></TD></TR>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dbg3\">\n"
		"  <BODY bgcolor=\"#7a7a7a\"></BODY>\n"
		"</DIV><DIV ID=\"dt%dbody\"><BODY %s>\n"
		"  <TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD align=center valign=middle nowrap></TD>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dimg\">\n"
		"  <TABLE border=0 cellspacing=0 cellpadding=0 %s width=18 height=%d>\n"
		"  <TD valign=middle align=right><img src=/sys/images/ico17.gif></TD></TR>\n"
		"  </TABLE>\n"
		"</DIV>\n"
		"</DIV>\n", 
		id, id, w, h, id, w-1, h-1, id, id, bgcolor, w-2, h-2, id, bgcolor, h-2); 
	htrAddBodyItem_va(s, 
		"<DIV ID=\"dt%dpane2\">\n"
		"<TABLE border=0 cellpadding=0 cellspacing=0 width=182 height=142>\n"
		"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>\n"
		"    <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=182></TD>\n"
		"    <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>\n"
		"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=190 width=1></TD>\n"
		"    <TD %s>&nbsp;</TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=190 width=1></TD></TR>\n"
		"<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=182></TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>\n"
		"</TABLE>\n"
		"</DIV>\n", 
		id, bgcolor);

	/** Add a function to be used to change the state (mode) of the button **/
	htrAddScriptFunction(s, "dt_toggledd", "\n"
		"function dt_toggledd(l)\n"
		"    {\n"
		"    if (l.ld.visibility == 'hide')\n"
		"        l.ld.visibility = 'show';\n"
		"    else\n"
		"        l.ld.visibility = 'hide';\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_setmode", "\n"
		"function dt_setmode(layer,mode)\n"
		"    {\n"
		"    layer.mode = mode;\n"
		"    if (mode == 0) /* CLICK */\n"
		"        {\n"
		"        layer.lbg1.bgColor = '#7a7a7a';\n"
		"        layer.lbg2.bgColor = '#ffffff';\n"
		"        layer.lbdy.moveBy(1,1);\n"
		"        layer.limg.moveBy(1,1);\n"
		"        layer.lbdy.clip.width -= 1;\n"
		"        layer.lbdy.clip.height -= 1;\n"
		"        }\n"
		"    else if (mode == 1) /* UNCLICK */\n"
		"        {\n"
		"        layer.lbg1.bgColor = '#ffffff';\n"
		"        layer.lbg2.bgColor = '#7a7a7a';\n"
		"        layer.lbdy.moveBy(-1,-1);\n"
		"        layer.limg.moveBy(-1,-1);\n"
		"        layer.lbdy.clip.width += 1;\n"
		"        layer.lbdy.clip.height += 1;\n"
		"        }\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_drawmonth", "\n"
		"function dt_drawmonth(l, d)\n"
		"    {\n"
		"    l.hdr.x = 28; l.hdr.y = 2;\n"
		"    l.hdr.visibility = 'inherit';\n"
		"    l.hdr.document.write('<table '+l.document.mainlayer.colorBG+' width=124 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=center valign=middle><b>'+l.document.mainlayer.strMonthsAbbrev[d.getMonth()]+' '+(d.getYear()+1900)+'</b></td></tr></table>');\n"
		"    l.hdr.document.layer = l;\n"
		"    l.hdr.document.kind = 'dt';\n"
		"    l.hdr.document.subkind = 'dt_dropdown';\n"
		"    l.hdr.document.close();\n"
		
		"    l.hdr1.x = 2; l.hdr.y = 2;\n"
		"    l.hdr1.visibility = 'inherit';\n"
		"    l.hdr1.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&lt;&lt;</B></td></tr></table>');\n"
		"    l.hdr1.document.mainlayer = l.document.mainlayer;\n"
		"    l.hdr1.document.kind = 'dt';\n"
		"    l.hdr1.document.subkind = 'dt_yearprev';\n"
		"    l.hdr1.document.close();\n"
		"    l.hdr1.losefocushandler = dt_losefocus;\n"
		
		"    l.hdr2.x = 27; l.hdr.y = 2;\n"
		"    l.hdr2.visibility = 'inherit';\n"
		"    l.hdr2.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&lt;</B></td></tr></table>');\n"
		"    l.hdr2.document.mainlayer = l.document.mainlayer;\n"
		"    l.hdr2.document.kind = 'dt';\n"
		"    l.hdr2.document.subkind = 'dt_monthprev';\n"
		"    l.hdr2.document.close();\n"
		"    l.hdr2.losefocushandler = dt_losefocus;\n"
		
		"    l.hdr3.x = l.document.mainlayer.std_w-52; l.hdr.y = 2;\n"
		"    l.hdr3.visibility = 'inherit';\n"
		"    l.hdr3.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&gt;</B></td></tr></table>');\n"
		"    l.hdr3.document.mainlayer = l.document.mainlayer;\n"
		"    l.hdr3.document.kind = 'dt';\n"
		"    l.hdr3.document.subkind = 'dt_monthnext';\n"
		"    l.hdr3.document.close();\n"
		"    l.hdr3.losefocushandler = dt_losefocus;\n"
		
		"    l.hdr4.x = l.document.mainlayer.std_w-27; l.hdr.y = 2;\n"
		"    l.hdr4.visibility = 'inherit';\n"
		"    l.hdr4.document.write('<table width=25 height=20 border=0 cellpadding=0 cellspacing=0><tr><td align=center valign=middle><B>&gt;&gt;</B></td></tr></table>');\n"
		"    l.hdr4.document.mainlayer = l.document.mainlayer;\n"
		"    l.hdr4.document.kind = 'dt';\n"
		"    l.hdr4.document.subkind = 'dt_yearnext';\n"
		"    l.hdr4.document.close();\n"
		"    l.hdr4.losefocushandler = dt_losefocus;\n"
		
		"    l.day = new Layer(1024, l);\n"
		"    l.day.x = 1; l.day.y = 21;\n"
		"    l.day.visibility = 'inherit';\n"
		"    l.day.document.write('<table width=181 height=20 cellpadding=0 cellspacing=0 border=0><tr>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>S</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>M</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>T</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>W</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>T</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>F</b></td>');\n"
		"    l.day.document.write('<td align=center valign=middle width=25><b>S</b></td>');\n"
		"    l.day.document.write('</tr></table>');\n"
		"    l.day.document.layer = l;\n"
		"    l.day.kind = 'dt';\n"
		"    l.day.subkind = 'dt_dropdown';\n"
		"    l.day.document.close();\n"
		"    var row=0;\n"
		"    var tmpDate = new Date(d);\n"
		"    tmpDate.setDate(1);\n"
		"    var col=tmpDate.getDay();\n"
		"    for (var i=1; i <= dt_getdaysinmonth(tmpDate); i++)\n"
		"        {\n"
		"        if (col!=0 && col%7==0) { row++; col=0; }\n"
		"        dt_drawday(l.document.mainlayer.ld.hiddenLayer.dayLayersArray[i-1], i, col, row);\n"
		"        col++;\n"
		"        }\n"
		"    for (;i <= 31; i++) { l.document.mainlayer.ld.hiddenLayer.dayLayersArray[i-1].visibility = 'hide'; }\n"
		"    l.document.mainlayer.ld.hiddenLayer.visibility = 'inherit';\n"
		"    l.document.mainlayer.ld.contentLayer.visibility = 'hidden';\n"
		"    tmp = l.document.mainlayer.ld.contentLayer;\n"
		"    l.document.mainlayer.ld.contentLayer = l.document.mainlayer.ld.hiddenLayer;\n"
		"    l.document.mainlayer.ld.hiddenLayer = tmp;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_drawday", "\n"
		"function dt_drawday(l, dy, col, row)\n"
		"    {\n"
		"    l.dateVal = new Date(l.document.mainlayer.tmpdate);\n"
		"    l.dateVal.setDate(dy);\n"
		"    l.x = 1+(col*26);\n"
		"    l.y = 42+(row*20);\n"
		"    l.clip.width=25; l.clip.height=20;\n"
		"    l.visibility = 'inherit';\n"
		"    l.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 width=25 height=20');\n"
		"    l.document.write('><TR><TD align=center valign=middle>'+dy+'</TD></TR></TABLE>');\n"
		"    l.document.close();\n"
		"    pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dt', 'dt', 0);\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_losefocus", "\n"
		"function dt_losefocus()\n"
		"    {\n"
		"    return true;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_drawdate", "\n"
		"function dt_drawdate(l, d)\n"
		"    {\n"
		"    l.document.write('<TABLE border=0 cellspacing=0 cellpadding=0 '+l.document.layer.colorBG+' width='+(l.document.layer.w-20)+' height='+(l.document.layer.h-2)+'>');\n"
		"    l.document.write('<TR><TD align=center valign=middle nowrap><FONT color=\"'+l.document.layer.colorFG+'\">');\n"
		"    if (d && d != 'Invalid Date')\n"
		"        l.document.write(dt_formatdate(l.document.layer, d, 0));\n"
		"    l.document.write('</FONT></TD></TR></TABLE>');\n"
		"    l.document.close();\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_drawtime", "\n"
		"function dt_drawtime(l, d)\n"
		"    {\n"
		"    lhr = l.ld.tl_hr;\n"
		"    lmn = l.ld.tl_mn;\n"

		"    lhr.document.write('<table width=70 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=right width=58>');\n"
		"    lmn.document.write(dt_strpad(l.wdate.getHours(), '0', 2));\n"
		"    lhr.document.write('&nbsp;</td><td width=12>');\n"
		"    lhr.document.write('<table cellpadding=0 cellspacing=0 border=0 width=12><tr><td valign=middle>');\n"
		"    lhr.document.write('<img src=/sys/images/spnr_up.gif></td></tr><tr><td>');\n"
		"    lhr.document.write('<img src=/sys/images/spnr_down.gif>');\n"
		"    lhr.document.write('</td></tr></table>');\n"
		"    lhr.document.write('</td></tr></table>');\n"
		"    lhr.document.images[0].kind = 'dt';\n"
		"    lhr.document.images[0].subkind = 'dt_hour_up';\n"
		"    lhr.document.images[0].mainlayer = l;\n"
		"    lhr.document.images[1].kind = 'dt';\n"
		"    lhr.document.images[1].subkind = 'dt_hour_down';\n"
		"    lhr.document.images[1].mainlayer = l;\n"
		"    lhr.document.close();\n"

		"    lmn.document.write('<table width=70 height=20 cellpadding=0 cellspacing=0 border=0><tr><td align=right width=58>');\n"
		"    lmn.document.write(dt_strpad(l.wdate.getMinutes(), '0', 2));\n"
		"    lmn.document.write('&nbsp;</td><td width=12>');\n"
		"    lmn.document.write('<table cellpadding=0 cellspacing=0 border=0 width=12><tr><td valign=middle>');\n"
		"    lmn.document.write('<img src=/sys/images/spnr_up.gif></td></tr><tr><td>');\n"
		"    lmn.document.write('<img src=/sys/images/spnr_down.gif>');\n"
		"    lmn.document.write('</td></tr></table>');\n"
		"    lmn.document.write('</td></tr></table>');\n"
		"    lmn.document.images[0].kind = 'dt';\n"
		"    lmn.document.images[0].subkind = 'dt_min_up';\n"
		"    lmn.document.images[0].mainlayer = l;\n"
		"    lmn.document.images[1].kind = 'dt';\n"
		"    lmn.document.images[1].subkind = 'dt_min_down';\n"
		"    lmn.document.images[1].mainlayer = l;\n"
		"    lmn.document.close();\n"
		"    }\n", 0);

	/**
	 **  Here is the leap year algorithm used.  To the best of my knowledge
	 **  it's the best way to detect leap years.  If I am wrong, please
	 **  correct me.  - LME (July 2002)
	 **
	 **    IF year is divisible by 4, it is a leap year
	 **    EXCEPT if a year is divisible by 100, it is not a leap year
	 **    EXCEPT if a year is divisible by 400, then it IS a leap year.
	 **/
	htrAddScriptFunction(s, "dt_isleapyear", "\n"
		"function dt_isleapyear(d)\n"
		"    {\n"
		"    var yr = d.getYear()+1900;\n"
		"    if (yr % 4 == 0)\n"
		"        {\n"
		"        if (yr % 100 == 0 && yr % 400 != 0)\n"
		"            {\n"
		"            return false;\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            return true;\n"
		"            }\n"
		"        }\n"
		"    else\n"
		"        return false;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_getdaysinmonth", "\n"
		"function dt_getdaysinmonth(d)\n"
		"    {\n"
		"    switch (d.getMonth())\n"
		"        {\n"
		"        case 0: return 31;\n"
		"        case 1: return (dt_isleapyear(d)?29:28);\n"
		"        case 2: return 31;\n"
		"        case 3: return 30;\n"
		"        case 4: return 31;\n"
		"        case 5: return 30;\n"
		"        case 6: return 31;\n"
		"        case 7: return 31;\n"
		"        case 8: return 30;\n"
		"        case 9: return 31;\n"
		"        case 10: return 30;\n"
		"        case 11: return 31;\n"
		"        }\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "dt_formatdate", "\n"
		"function dt_formatdate(l, d, fmt)\n"
		"    {\n"
		"    var str;\n"
		"    switch (fmt)\n"
		"        {\n"
		"        case 0:\n"
		"        default:\n"
		"            str  = l.strMonthsAbbrev[d.getMonth()] + ' ';\n"
		"            str += d.getDate() + ', ';\n"
		"            str += d.getYear()+1900 + ', ';\n"
		"            str += dt_strpad(d.getHours(), '0', 2)+':';\n"
		"            str += dt_strpad(d.getMinutes(), '0', 2);\n"
		"            break;\n"
		"        }\n"
		"    return str;\n"
		"    }\n", 0);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","dt",
		"    var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
		"    if (dt_current != null && targetLayer.layer != dt_current && targetLayer.subkind != 'dt_dropdown')\n"
		"        {\n"
		"        if (targetLayer.subkind == 'dt_day')\n"
		"            {\n"
		"            var ml = targetLayer.document.mainlayer;\n"
		"            var td = new Date(targetLayer.dateVal);\n"
		"            td.setHours(ml.wdate.getHours());\n"
		"            td.setMinutes(ml.wdate.getMinutes());\n"
		"            ml.wdate = new Date(td);\n"
		"            dt_drawdate(ml.lbdy, ml.wdate);\n"
		"            if (dt_current.form)\n"
		"                dt_current.form.DataNotify(dt_current);\n"
		"            dt_current.document.layer.ld.visibility = 'hide';\n"
		"            dt_current = null;\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_yearprev')\n"
		"            {\n"
		"            var ml = targetLayer.mainlayer;\n"
		"            ml.tmpdate.setYear(ml.tmpdate.getYear()+1899);\n"
		"            dt_drawmonth(ml.ld, ml.tmpdate);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_yearnext')\n"
		"            {\n"
		"            var ml = targetLayer.mainlayer;\n"
		"            ml.tmpdate.setYear(ml.tmpdate.getYear()+1901);\n"
		"            dt_drawmonth(ml.ld, ml.tmpdate);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_monthnext')\n"
		"            {\n"
		"            var ml = targetLayer.mainlayer;\n"
		"            ml.tmpdate.setMonth(ml.tmpdate.getMonth()+1);\n"
		"            dt_drawmonth(ml.ld, ml.tmpdate);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_monthprev')\n"
		"            {\n"
		"            var ml = targetLayer.mainlayer;\n"
		"            ml.tmpdate.setMonth(ml.tmpdate.getMonth()-1);\n"
		"            dt_drawmonth(ml.ld, ml.tmpdate);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_hour_up')\n"
		"            {\n"
		"            var d = targetLayer.mainlayer.wdate;\n"
		"            if (d.getHours() < 23)\n"
		"                {\n"
		"                targetLayer.mainlayer.tmpdate = new Date(d);\n"
		"                d.setHours(d.getHours()+1);\n"
		"                dt_drawtime(targetLayer.mainlayer, d);\n"
		"                dt_drawdate(targetLayer.mainlayer.lbdy, d);\n"
		"                }\n"
		"            if (targetLayer.mainlayer.form)\n"
		"                targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_hour_down')\n"
		"            {\n"
		"            var d = targetLayer.mainlayer.wdate;\n"
		"            if (d.getHours() > 0)\n"
		"                {\n"
		"                targetLayer.mainlayer.tmpdate = new Date(d);\n"
		"                d.setHours(d.getHours()-1);\n"
		"                dt_drawtime(targetLayer.mainlayer, d);\n"
		"                dt_drawdate(targetLayer.mainlayer.lbdy, d);\n"
		"                }\n"
		"            if (targetLayer.mainlayer.form)\n"
		"                targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_min_up')\n"
		"            {\n"
		"            var d = targetLayer.mainlayer.wdate;\n"
		"            if (d.getMinutes() < 59)\n"
		"                {\n"
		"                targetLayer.mainlayer.tmpdate = new Date(d);\n"
		"                d.setMinutes(d.getMinutes()+1);\n"
		"                dt_drawtime(targetLayer.mainlayer, d);\n"
		"                dt_drawdate(targetLayer.mainlayer.lbdy, d);\n"
		"                }\n"
		"            if (targetLayer.mainlayer.form)\n"
		"                targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);\n"
		"            }\n"
		"        else if (targetLayer.subkind == 'dt_min_down')\n"
		"            {\n"
		"            var d = targetLayer.mainlayer.wdate;\n"
		"            if (d.getMinutes() > 0)\n"
		"                {\n"
		"                targetLayer.mainlayer.tmpdate = new Date(d);\n"
		"                d.setMinutes(d.getMinutes()-1);\n"
		"                dt_drawtime(targetLayer.mainlayer, d);\n"
		"                dt_drawdate(targetLayer.mainlayer.lbdy, d);\n"
		"                }\n"
		"            if (targetLayer.mainlayer.form)\n"
		"                targetLayer.mainlayer.form.DataNotify(targetLayer.mainlayer);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            dt_current.document.layer.ld.visibility = 'hide';\n"
		"            dt_current = null;\n"
		"            }\n"
		"        }\n"
		"    else if (targetLayer != null && targetLayer.kind == 'dt')\n"
		"        {\n"
		"        if (targetLayer.subkind != 'dt_dropdown')\n"
		"            {\n"
		"            if (targetLayer.form)\n"
		"                targetLayer.form.FocusNotify(targetLayer);\n"
		"            dt_setmode(targetLayer,0);\n"
		"            targetLayer.ld.visibility = 'inherit';\n"
		"            dt_current = targetLayer;\n"
		"            }\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","dt",
		"    var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
		"    if (dt_current != null && targetLayer.kind == 'dt' && (targetLayer.subkind == null || targetLayer.subkind == 'dt_button'))\n"
		"        {\n"
		"        dt_setmode(targetLayer,1);\n"
		"        }\n"
		"\n" );

    return 0;
    }


/*** htdtInitialize - register with the ht_render module.
 ***/
int
htdtInitialize()
    {
    pHtDriver drv;

	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Date/Time Widget Driver");
	strcpy(drv->WidgetName,"datetime");
	drv->Render = htdtRender;
	drv->Verify = htdtVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTDT.idcnt = 0;

    return 0;
    }
