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
/* Module: 	htdrv_textarea.c         				*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	July 9, 2002						*/
/* Description:	HTML Widget driver for a multi-line textarea.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: htdrv_textarea.c,v 1.7 2002/07/12 20:16:13 pfinley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textarea.c,v $

    $Log: htdrv_textarea.c,v $
    Revision 1.7  2002/07/12 20:16:13  pfinley
    Undid previous change :)

    Revision 1.6  2002/07/12 19:46:22  pfinley
    added cvs logging to file.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTX;


/*** httxVerify - not written yet.
 ***/
int
httxVerify()
    {
    return 0;
    }


/*** httxRender - generate the HTML code for the editbox widget.
 ***/
int
httxRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    /*char sbuf[HT_SBUF_SIZE];*/
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_readonly = 0;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];

    	/** Get an id for this. **/
	id = (HTTX.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (objGetAttrValue(w_obj,"maxchars",POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (objGetAttrValue(w_obj,"readonly",POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgColor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Style of Textarea - raised/lowered **/
	if (objGetAttrValue(w_obj,"style",POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
	if (is_raised)
	    {
	    c1 = "white_1x1.png";
	    c2 = "dkgrey_1x1.png";
	    }
	else
	    {
	    c1 = "dkgrey_1x1.png";
	    c2 = "white_1x1.png";
	    }

	if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
	    {
	    strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	    }
	else 
	    { 
	    fieldname[0]='\0';
	    } 

	/** Ok, write the style header items. **/
	htrAddHeaderItem(s,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem_va(s,"\t#tx%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,"    </STYLE>\n");

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "tx_ibeam", "null", 0);
	htrAddScriptGlobal(s, "tx_metric", "null", 0);
	htrAddScriptGlobal(s, "tx_current", "null", 0);

	/** Textarea text encoding function **/
	htrAddScriptFunction(s, "tx_encode", "\n"
		"function tx_encode(s)\n"
		"    {\n"
		"    rs = '';\n"
		"    for(i=0;i<s.length;i++)\n"
		"        {\n"
		"        if (s[i] == '<') rs += '&lt;';\n"
		"        else if (s[i] == '>') rs += '&gt;';\n"
		"        else if (s[i] == '&') rs += '&amp;';\n"
		"        else if (s[i] == ' ') rs += '&nbsp;';\n"
		"        else rs += s[i];\n"
		"        }\n"
		"    return rs;\n"
		"    }\n", 0);

	/** Get value function **/
	htrAddScriptFunction(s, "tx_getvalue", "\n"
		"function tx_getvalue()\n"
		"    {\n"
		"    var txt = '';\n"
		"    for (var i=0;i<this.rows.length;i++)\n"
		"        {\n"
		"        if (this.rows[i].newLine) txt += '\\n'+this.rows[i].content;\n"
		"        else txt += this.rows[i].content;\n"
		"        }\n"
		"    return txt;\n"
		"    }\n", 0);

	/** Set value function **/
	htrAddScriptFunction(s, "tx_setvalue", "\n"
		"function tx_setvalue(txt)\n"
		"    {\n"
		"    var reg = /(.*)(\\n*)/;\n"
		"    do\n"
		"        {\n"
		"        reg.exec(txt);\n"
		"        var para = RegExp.$1;\n"
		"        var newL = RegExp.$2;\n"
		"        txt = txt.substr(para.length+newL.length);\n"
		"        tx_wordWrapDown(this,this.rows.length-1,para,0);\n"
		"        if (newL)\n"
		"            {\n"
		"            for(var i=0;i<newL.length;i++)\n"
		"                {\n"
		"                tx_insertRow(this,this.rows.length,'');\n"
		"                this.rows[this.rows.length-1].newLine = 1;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"    while (txt)\n"
		"    for(var i in this.rows)\n"
		"        {\n"
		"        this.rows[i].hiddenLayer.visibility = 'hidden';\n"
		"        this.rows[i].contentLayer.visibility = 'inherit';\n"
		"        this.rows[i].changed = 0;\n"
		"        }\n"
		"    }\n", 0);

	/** Clear function **/
	htrAddScriptFunction(s, "tx_clearvalue", "\n"
		"function tx_clearvalue()\n"
		"    {\n"
		"    if (this.getvalue() == '') return;\n"
		"    tx_updateRow(this,0,'');\n"
		"    for(var i=0;i<this.rows.length;i++)\n"
		"        {\n"
		"        this.rows[i].contentLayer.visibility = 'hidden';\n"
		"        this.rows[i].hiddenLayer.visibility = 'hidden';\n"
		"        }\n"
		"    if (this.rows.length > 0) this.rows.splice(1,this.rows.length-1);\n"
		"    if (tx_current == this)\n"
		"        {\n"
		"        this.cursorRow = 0;\n"
		"        this.cursorCol = 0;\n"
		"        tx_ibeam.moveToAbsolute(this.rows[this.cursorRow].contentLayer.pageX + this.cursorCol*tx_metric.charWidth, this.rows[this.cursorRow].contentLayer.pageY);\n"
		"        tx_ibeam.visibility = 'inherit';\n"
		"        }\n"
		"    }\n", 0);

	/** Enable control function **/
	htrAddScriptFunction(s, "tx_enable", "\n"
		"function tx_enable()\n"
		"    {\n"
		"    eval('this.document.'+this.bg);\n"
		"    this.enabled='full';\n"
		"    }\n", 0);

	/** Disable control function **/
	htrAddScriptFunction(s, "tx_disable", "\n"
		"function tx_disable()\n"
		"    {\n"
		"    this.document.background='';\n"
		"    this.document.bgColor='#e0e0e0';\n"
		"    this.enabled='disabled';\n"
		"    }\n", 0);

	/** Readonly-mode function **/
	htrAddScriptFunction(s, "tx_readonly", "\n"
		"function tx_readonly()\n"
		"    {\n"
		"    eval('this.document.'+this.bg);\n"
		"    this.enabled='readonly';\n"
		"    }\n", 0);

	/** Changes the actual cursor position (cursorRow & cursorCol) **/
	htrAddScriptFunction(s, "tx_getCursorPos", "\n"
		"function tx_getCursorPos(l,chng,opt)\n"
		"    {\n"
		"    l.cursorPos += chng;\n"
		"    pos = l.cursorPos;\n"
		"    var i = 0;\n"
		"    while (i < l.rows.length && pos-l.rows[i].content.length > 0)\n"
		"        {\n"
		"        pos -= l.rows[i].content.length;\n"
		"        i++;\n"
		"        if (l.rows[i].newLine) pos--;\n"
		"        }\n"
		"    l.cursorRow = i;\n"
		"    l.cursorCol = pos;\n"
		"    if (l.rows[i+1] && !l.rows[i+1].newLine && pos == l.rows[i].content.length && opt)\n"
		"        {\n"
		"        l.cursorRow++;\n"
		"        l.cursorCol = 0;\n"
		"        }\n"
		"    }\n", 0);

	/** Sets the positioning of the cursor within the text (cursorPos) **/
	htrAddScriptFunction(s, "tx_setCursorPos", "\n"
		"function tx_setCursorPos(l,row,col)\n"
		"    {\n"
		"    var pos=0;\n"
		"    for(var i=0;i<row;i++)\n"
		"        {\n"
		"        pos += l.rows[i].content.length;\n"
		"        if (l.rows[i].newLine) pos++;\n"
		"        }\n"
		"    if (l.rows[i].newLine) pos++;\n"
		"    pos += col;\n"
		"    return pos;\n"
		"    }\n", 0);

	/** Inserts an editable row to the textarea **/
	htrAddScriptFunction(s, "tx_insertRow", "\n"
		"function tx_insertRow(l, index, txt)\n"
		"    {\n"
		"    if (index > l.rows.length) index = l.rows.length;\n"
		"    r = new Object();\n"
		"    r.content = txt;\n"
		"    r.contentLayer = new Layer(l.clip.width-2, l);\n"
		"    r.hiddenLayer = new Layer(l.clip.width-1, l);\n"
		"    r.contentLayer.visibility = 'hidden';\n"
		"    r.hiddenLayer.visibility = 'hidden';\n"
		"    r.contentLayer.mainlayer = l;\n"
		"    r.hiddenLayer.mainlayer = l;\n"
		"    r.contentLayer.kind = 'tx';\n"
		"    r.hiddenLayer.kind = 'tx';\n"
		"    r.contentLayer.document.write('<PRE>' + tx_encode(txt) + '</PRE> ');\n"
		"    r.contentLayer.document.close();\n"
		"    r.changed = 1;\n"
		"    l.rows.splice(index,0,r);\n"
		"    for(i=index;i<l.rows.length;i++)\n"
		"    	{\n"
		"        if (l.rows[i] != null)\n"
		"            {\n"
		"            l.rows[i].contentLayer.moveTo(1,i*tx_metric.charHeight+1);\n"
		"            l.rows[i].hiddenLayer.moveTo(1,i*tx_metric.charHeight+1);\n"
		"            }\n"
		"        }\n"
		"    }\n", 0);

	/** Deletes an editable row from the textarea **/
	htrAddScriptFunction(s, "tx_deleteRow", "\n"
		"function tx_deleteRow(l, index)\n"
		"    {\n"
		"    l.rows[index].contentLayer.visibility = 'hidden';\n"
		"    l.rows[index].hiddenLayer.visibility = 'hidden';\n"
		"    l.rows.splice(index,1);\n"
		"    for(i=index;i<l.rows.length;i++)\n"
		"        {\n"
		"        if (l.rows[i] != null)\n"
		"            {\n"
		"            l.rows[i].contentLayer.moveTo(1, i*tx_metric.charHeight);\n"
		"            l.rows[i].hiddenLayer.moveTo(1, i*tx_metric.charHeight);\n"
		"            }\n"
		"        }\n"
		"    }\n", 0);

	/** Updates an existing row's contents **/
	htrAddScriptFunction(s, "tx_updateRow", "\n"
		"function tx_updateRow(l, index, txt)\n"
		"    {\n"
		"    if (index > l.rows.length) index = l.rows.length-1;\n"
		"    r = l.rows[index];\n"
		"    r.hiddenLayer.document.write('<PRE>' + tx_encode(txt) + '</PRE> ');\n"
		"    r.hiddenLayer.document.close();\n"
		"    tmp = r.contentLayer;\n"
		"    r.contentLayer = r.hiddenLayer;\n"
		"    r.hiddenLayer = tmp;\n"
		"    r.content = txt;\n"
		"    r.changed = 1;\n"
		"    }\n", 0);

	/** Wraps words from beginning of row to the end of the above row (recursive) **/
	htrAddScriptFunction(s, "tx_wordWrapUp", "\n"
		"function tx_wordWrapUp(l,index,txt,c)\n"
		"    {\n"
		"    if (!l.rows[index+1] || l.rows[index+1].newLine)\n"
		"        {\n"
		"        if (!txt && !l.rows[index].newLine && index>0)\n"
		"            {\n"
//		"            if (index == 0) return c;\n"
//		"            if (!l.rows[index+1]) return c;\n"
//		"            if (index == 0 && l.rows[index+1] && l.rows[index+1].newLine) return c;\n"
		"            tx_deleteRow(l,index);\n"
		"            }\n"
		"        return c;\n"
		"        }\n"
		"    var open = l.rowCharLimit - txt.length;\n"
		"    if (open == 0) return c;\n"
		"    var avail = l.rows[index+1].content.substr(0,open);\n"
		"    if (l.rows[index+1].content[open] == ' ' || !l.rows[index+1].content[open])\n"
		"        {\n"
		"        var add = avail;\n"
		"        var sub = l.rows[index+1].content.substr(open);\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        for (var i=open; avail[i]!=' ' && i>=0; i--) {}\n"
		"        if (i < 0) return c;\n"
		"        var add = avail.substr(0,i+1);\n"
		"        var sub = l.rows[index+1].content.substr(i+1);\n"
		"        }\n"
		"    tx_updateRow(l,index,txt+add);\n"
		"    tx_updateRow(l,index+1,sub);\n"
		"    return tx_wordWrapUp(l,index+1,sub,1);\n"
		"    }\n", 0);

	/** Wraps words from the end of row to the beginning of below row (recursive) **/
	htrAddScriptFunction(s, "tx_wordWrapDown", "\n"
		"function tx_wordWrapDown(l,index,txt,c)\n"
		"    {\n"
		"    if (!txt) return c;\n"
		"    if (txt.length <= l.rowCharLimit) \n"
		"        {\n"
		"        tx_updateRow(l,index,txt);\n"
		"        return c;\n"
		"        }\n"
		"    if (txt[l.rowCharLimit] == ' ')\n"
		"        {\n"
		"        var sub = txt.substr(0,l.rowCharLimit);\n"
		"        var add = txt.substr(l.rowCharLimit);\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        for (var i=l.rowCharLimit-1; txt[i]!=' ' && i>=0; i--){}\n"
		"        var sub = txt.substr(0,i+1);\n"
		"        var add = txt.substr(i+1); \n"
		"        }\n"
		"    tx_updateRow(l,index,sub);\n"
		"    if (!l.rows[index+1] || l.rows[index+1].newLine) tx_insertRow(l,index+1,'');\n"
		"    return tx_wordWrapDown(l,index+1,add+l.rows[index+1].content,1);\n"
		"    }\n", 0);

	/** Textarea keyboard handler **/
	htrAddScriptFunction(s, "tx_keyhandler", "\n"
		"function tx_keyhandler(l,e,k)\n"
		"    {\n"
		"    if (!tx_current) return true;\n"
		"    if (tx_current.enabled!='full') return 1;\n"
		"    if (tx_current.form) tx_current.form.DataNotify(tx_current);\n"
		"    if (k >= 32 && k < 127)\n"
		"        {\n"
		"        txt = l.rows[l.cursorRow].content;\n"
		"        if(txt.length == l.rowCharLimit && k!=32)\n"			// This is a work-around for a bug:
		"            {\n"							// if you try inserting a character other
		"            for(var i=0;i<txt.length;i++) if(txt[i] == ' ') break;\n"	// than a space when a row is full and
		"            if(i==txt.length) return false;\n"				// contains no spaces, it will go into an infinite
		"            }\n"							// loop and crash netscape... needs fixing.
		"        if (l.rows[l.cursorRow+1] && l.cursorCol == l.rows[l.cursorRow].content.length && l.rows[l.cursorRow+1].content[0] != ' ' && k!=32 && !l.rows[l.cursorRow+1].newLine)\n"
		"            {\n"
		"            tx_wordWrapDown(l,l.cursorRow+1,String.fromCharCode(k)+l.rows[l.cursorRow+1].content,0);\n"
		"            tx_getCursorPos(l,1,0);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            txt = l.rows[l.cursorRow].content;\n"
		"            newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);\n"
		"            tx_wordWrapDown(l,l.cursorRow,newtxt,0);\n"
		"            if (l.cursorRow > 0) tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0);\n"
		"            tx_getCursorPos(l,1,0);\n"
		"            }\n"
		"        }\n"
		"    else if (k == 13)\n"
		"        {\n"
		"        txt = l.rows[l.cursorRow].content;\n"
		"        oldrow = txt.substr(0,l.cursorCol);\n"
		"        newrow = txt.substr(l.cursorCol,txt.length);\n"
		"        tx_updateRow(l,l.cursorRow,oldrow);\n"
		"        tx_insertRow(l,l.cursorRow+1,newrow);\n"
		"        l.rows[l.cursorRow+1].newLine = 1;\n"
		"        tx_wordWrapUp(l,l.cursorRow+1,newrow,0);\n"
		"        tx_getCursorPos(l,1,0);\n"
		"        }\n"
		"    else if (k == 8)\n"
		"        {\n"
		"        txt = l.rows[l.cursorRow].content;\n"
		"        var beginP = l.rows[l.cursorRow].newLine;\n"
		"        if (l.cursorCol == 0)\n"
		"            {\n"
		"            if (l.cursorRow == 0) return false;\n"
		"            var txtpre = l.rows[l.cursorRow-1].content;\n"
		"            if (l.rows[l.cursorRow].newLine)\n"
		"                {\n"
		"                l.rows[l.cursorRow].newLine = 0;\n"
		"                tx_deleteRow(l,l.cursorRow);\n"
		"                tx_wordWrapDown(l,l.cursorRow-1,txtpre+txt,0);\n"
		"                tx_getCursorPos(l,-1,0);\n"
		"                }\n"
		"            else\n"
		"                {\n"
		"                tx_deleteRow(l,l.cursorRow);\n"
		"                tx_wordWrapDown(l,l.cursorRow-1,txtpre.substr(0,txtpre.length-1) + txt,0);\n"
		"                tx_getCursorPos(l,-1,0);\n"
		"                }\n"
		"            }\n"
		"        else if (l.cursorCol == 1 && l.cursorRow > 0 && l.rows[l.cursorRow].content[0] == ' ' && !beginP)\n"
		"            {\n"
		"            tx_deleteRow(l,l.cursorRow);\n"
		"            tx_wordWrapDown(l,l.cursorRow-1,l.rows[l.cursorRow-1].content + txt.substr(1));\n"
		"            tx_getCursorPos(l,-1,0);\n"
		"            }\n"
		"        else if (l.cursorCol == l.rows[l.cursorRow].content.length && l.rows[l.cursorRow+1] && l.rows[l.cursorRow+1].content[0] != ' ' && !l.rows[l.cursorRow+1].newLine)\n"
		"            {\n"
		"            var nextRow = l.rows[l.cursorRow+1].content;\n"
		"            tx_deleteRow(l,l.cursorRow+1);\n"
		"            tx_wordWrapDown(l,l.cursorRow,txt.substr(0,txt.length-1) + nextRow,0);\n"
		"            tx_getCursorPos(l,-1,0);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol);\n"
		"            tx_updateRow(l,l.cursorRow,newtxt);\n"
		"            if (l.cursorRow > 0 && !beginP) var f = tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0);\n"
		"            if (!f) tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0);\n"
		"            if (l.rows[l.cursorRow] && l.rows[l.cursorRow].content[0] == ' ') tx_getCursorPos(l,-1,0);\n"
		"            else tx_getCursorPos(l,-1,1);\n"
		"            }\n"
		"        }\n"
		"    else if (k == 127)\n"
		"        {\n"
		"        txt = l.rows[l.cursorRow].content;\n"
		"        var beginP = l.rows[l.cursorRow].newLine;\n"
		"        if (l.cursorCol == txt.length)\n"
		"            {\n"
		"            if (l.cursorRow == l.rows.length-1) return false;\n"
		"            if (l.rows[l.cursorRow+1].newLine)\n"
		"                {\n"
		"                l.rows[l.cursorRow+1].newLine = 0;\n"
		"                var newtxt = txt+l.rows[l.cursorRow+1].content;\n"
		"                tx_deleteRow(l,l.cursorRow+1);\n"
		"                tx_wordWrapDown(l,l.cursorRow,newtxt);\n"
		"                tx_getCursorPos(l,0,0);\n"
		"                }\n"
		"            else\n"
		"                {\n"
		"                tx_updateRow(l,l.cursorRow+1,l.rows[l.cursorRow+1].content.substr(1,l.rows[l.cursorRow+1].content.length-1));\n"
		"                if (!tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0))\n"
		"                    if (l.rows[l.cursorRow+1]) tx_wordWrapUp(l,l.cursorRow+1,l.rows[l.cursorRow+1].content,0);\n"
		"                tx_getCursorPos(l,0,1);\n"
		"                }\n"
		"            }\n"
		"        else if (l.cursorCol == txt.length-1 && txt[txt.length-1] == ' ' && l.rows[l.cursorRow+1] && l.rows[l.cursorRow+1].content[0] != ' ' && !l.rows[l.cursorRow+1].newLine)\n"
		"            {\n"
		"            var nextRow = l.rows[l.cursorRow+1].content;\n"
		"            tx_deleteRow(l,l.cursorRow+1);\n"
		"            tx_wordWrapDown(l,l.cursorRow,txt.substr(0,txt.length-1) + nextRow,0);\n"
		"            tx_getCursorPos(l,-1,0);\n"
		"            }\n"
		"        else if (l.cursorCol == 0 && l.cursorRow > 0 && l.rows[l.cursorRow].content[0] == ' ' && !beginP) \n"
		"            {\n"
		"            tx_deleteRow(l,l.cursorRow);\n"
		"            tx_wordWrapDown(l,l.cursorRow-1,l.rows[l.cursorRow-1].content + txt.substr(1));\n"
		"            tx_getCursorPos(l,0,0);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            newtxt = txt.substr(0,l.cursorCol) + txt.substr(l.cursorCol+1,txt.length);\n"
		"            tx_updateRow(l,l.cursorRow,newtxt);\n"
		"            if (l.cursorRow > 0) { var f = tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0); }\n"
		"            if (!f) tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0);\n"
		"            tx_getCursorPos(l,0,0);\n"
		"            }\n"
		"        }\n"
		"    else return true;\n"
		"    for(i=0;i<l.rows.length;i++)\n"
		"        {\n"
		"        if (l.rows[i].changed == 1)\n"
		"            {\n"
		"            l.rows[i].hiddenLayer.visibility = 'hidden';\n"
		"            l.rows[i].contentLayer.visibility = 'inherit';\n"
		"            l.rows[i].changed = 0;\n"
		"            }\n"
		"        }\n"
		"    tx_ibeam.moveToAbsolute(l.rows[l.cursorRow].contentLayer.pageX + l.cursorCol*tx_metric.charWidth, l.rows[l.cursorRow].contentLayer.pageY);\n"
		"    tx_ibeam.visibility = 'inherit';\n"
		"    return false;\n"
		"    }\n", 0);

	/** Set focus to a new textarea **/
	htrAddScriptFunction(s, "tx_select", "\n"
		"function tx_select(x,y,l,c,n)\n"
		"    {\n"
		"    if (l.form) l.form.FocusNotify(l);\n"
		"    if (l.enabled != 'full') return 0;\n"
		"    l.cursorRow = Math.floor(y/tx_metric.charHeight);\n"
		"    l.cursorCol = Math.round(x/tx_metric.charWidth);\n"
		"    if (l.cursorRow >= l.rows.length)\n"
		"        {\n"
		"            l.cursorRow = l.rows.length - 1;\n"
		"            l.cursorCol = l.rows[l.cursorRow].content.length;\n"
		"        }\n"
		"    else if (l.cursorCol > l.rows[l.cursorRow].content.length) l.cursorCol = l.rows[l.cursorRow].content.length;\n"
		"    l.cursorPos = tx_setCursorPos(l,l.cursorRow,l.cursorCol);\n"
		"    l.cursorlayer = tx_ibeam;\n"
		"    tx_current = l;\n"
		"    tx_ibeam.visibility = 'hidden';\n"
		"    tx_ibeam.moveAbove(l);\n"
		"    tx_ibeam.moveToAbsolute(l.rows[0].contentLayer.pageX + l.cursorCol*tx_metric.charWidth, l.rows[0].contentLayer.pageY + l.cursorRow*tx_metric.charHeight);\n"
		"    tx_ibeam.zIndex = l.zIndex + 2;\n"
		"    tx_ibeam.visibility = 'inherit';\n"
		"    return 1;\n"
		"    }\n", 0);

	/** Take focus away from textarea **/
	htrAddScriptFunction(s, "tx_deselect", "\n"
		"function tx_deselect()\n"
		"    {\n"
		"    tx_ibeam.visibility = 'hidden';\n"
		"    if (tx_current) tx_current.cursorlayer = null;\n"
		"    tx_current = null;\n"
		"    return true;\n"
		"    }\n", 0);

	/** Textarea initializer **/
	htrAddScriptFunction(s, "tx_init", "\n"
		"function tx_init(l,fieldname,is_readonly,main_bg)\n"
		"    {\n"
		"    if (!main_bg) l.bg = \"bgcolor='#c0c0c0'\";\n"
		"    else l.bg = main_bg;\n"
		"    l.kind = 'textarea';\n"
		"    l.fieldname = fieldname;\n"
		"    if (!tx_ibeam || !tx_metric)\n"
		"        {\n"
		"        tx_metric = new Layer(24);\n"
		"        tx_metric.visibility = 'hidden';\n"
		"        tx_metric.document.write('<pre>xx</pre>');\n"
		"        tx_metric.document.close();\n"
		"        w2 = tx_metric.clip.width;\n"
		"        h1 = tx_metric.clip.height;\n"
		"        tx_metric.document.write('<pre>x\\nx</pre>');\n"
		"        tx_metric.document.close();\n"
		"        tx_metric.charHeight = tx_metric.clip.height - h1;\n"
		"        tx_metric.charWidth = w2 - tx_metric.clip.width;\n"
		"        tx_ibeam = new Layer(1);\n"
		"        tx_ibeam.visibility = 'hidden';\n"
		"        tx_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');\n"
		"        tx_ibeam.document.close();\n"
		"        tx_ibeam.resizeTo(1,tx_metric.charHeight);\n"
		"        }\n"
		"    l.rowCharLimit = Math.floor((l.clip.width-2)/tx_metric.charWidth);\n"
		"    l.cursorPos = 0;\n"
		"    l.rows = new Array();\n"
		"    tx_insertRow(l,0,'');\n"
		"    l.keyhandler = tx_keyhandler;\n"
		"    l.getfocushandler = tx_select;\n"
		"    l.losefocushandler = tx_deselect;\n"
		"    l.getvalue = tx_getvalue;\n"
		"    l.setvalue = tx_setvalue;\n"
		"    l.clearvalue = tx_clearvalue;\n"
		"    l.setoptions = null;\n"
		"    l.enablenew = tx_enable;\n"
		"    l.disable = tx_disable;\n"
		"    l.readonly = tx_readonly;\n"
		"    if (is_readonly)\n"
		"        {\n"
		"        l.enablemodify = tx_disable;\n"
		"        l.enabled = 'disable';\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        l.enablemodify = tx_enable;\n"
		"        l.enabled = 'full';\n"
		"        }\n"
		"    l.isFormStatusWidget = false;\n"
		"    pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'tbox', 'tbox', 1);\n"
		"    if (fm_current) fm_current.Register(l);\n"
		"    l.form = fm_current;\n"
		"    l.changed = false;\n"
		"    return l;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = tx_init(%s.layers.tx%dbase, \"%s\", %d, \"%s\");\n",
		nptr, parentname, id, 
		fieldname, is_readonly, main_bg);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"tx%dbase\"><BODY %s>\n",id, main_bg);
	htrAddBodyItem_va(s, "    <TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem_va(s, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);

	/** Check for objects within the editbox. **/
	/** The editbox can have no subwidgets **/
	/*sprintf(sbuf,"%s.mainlayer.document",nptr);*/
	/*sprintf(sbuf2,"%s.mainlayer",nptr);*/
	/*htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);*/

	/** End the containing layer. **/
	htrAddBodyItem(s, "</BODY></DIV>\n");

    return 0;
    }


/*** httxInitialize - register with the ht_render module.
 ***/
int
httxInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Multiline Textarea Driver");
	strcpy(drv->WidgetName,"textarea");
	drv->Render = httxRender;
	drv->Verify = httxVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);
	
	/** Register. **/
	htrRegisterDriver(drv);

	HTTX.idcnt = 0;

    return 0;
    }
