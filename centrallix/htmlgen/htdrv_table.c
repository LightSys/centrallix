#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/*									*/
/* Module: 	htdrv_table.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 29, 1999 					*/
/* Description:	HTML Widget driver for a data-driven table.  Has three	*/
/*		different modes -- static, dynamicpage, and dynamicrow.	*/
/*									*/
/*		Static means an inline table that can't be updated	*/
/*		without a parent container being completely reloaded.	*/
/*		DynamicPage means a table in a layer that can be	*/
/*		reloaded dynamically as a whole when necessary.  Good	*/
/*		when you need forward/back without reloading the page.	*/
/*		DynamicRow means each row is its own layer.  Good when	*/
/*		you need to insert rows dynamically and delete rows	*/
/*		dynamically at the client side without reloading the	*/
/*		whole table contents.					*/
/*									*/
/*		A static table's query is performed on the server side	*/
/*		and the HTML is generated at the server.  Both dynamic	*/
/*		types are built from a client-side query.  Static 	*/
/*		tables are generally best when the data will be read-	*/
/*		only.  Dynamicrow tables use the most client resources.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_table.c,v 1.36 2003/06/03 19:27:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_table.c,v $

    $Log: htdrv_table.c,v $
    Revision 1.36  2003/06/03 19:27:09  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.35  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.34  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.33  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.32  2002/08/26 20:49:33  lkehresman
    Added DblClick event to the table rows taht does essentially thte same
    thing that the Click event does.

    Revision 1.31  2002/08/18 18:46:27  jorupp
     * made the entire current record available on the event object (accessable as .data)

    Revision 1.30  2002/08/13 04:36:29  anoncvs_obe
    Changed the 't' table inf structure to be dynamically allocated to
    save the 1.2k that was used on the stack.

    Revision 1.29  2002/08/05 19:43:37  lkehresman
    Fixed the static table to reference the standard ".layer" rather than ".Layer"

    Revision 1.28  2002/07/25 20:05:15  mcancel
    Adding the function htrAddScriptInclude to the static table render
    function so the javascript code will be seen...

    Revision 1.27  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.26  2002/07/25 16:54:18  pfinley
    completely undoing the change made yesterday with aliasing of click events
    to mouseup... they are now two separate events. don't believe the lies i said
    yesterday :)

    Revision 1.25  2002/07/24 18:12:03  pfinley
    Updated Click events to be MouseUp events. Now all Click events must be
    specified as MouseUp within the Widget's event handler, or they will not
    work propery (Click can still be used as a event connector to the widget).

    Revision 1.24  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.23  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.22  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.21  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.20  2002/06/24 17:28:25  jorupp
     * fix bug where records would stay hidden following a query that returned 1 record when the next one returned multiple records

    Revision 1.19  2002/06/19 21:22:45  lkehresman
    Added a losefocushandler to the table.  Not having this broke static tables.

    Revision 1.18  2002/06/10 21:47:45  jorupp
     * bit of code cleanup
     * added movable borders to the dynamic table

    Revision 1.17  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.16  2002/06/03 18:43:45  jorupp
     * fixed a bug with the handling of empty fields in the dynamic table

    Revision 1.15  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.14  2002/05/31 02:40:38  lkehresman
    Added a horrible hack to fix the hang problem.  The previous hack involved
    alert windows.  This one is at least transparent (as it uses a while loop
    to pause).  Still VERY ugly, but I have no other ideas.

    Revision 1.13  2002/05/30 05:01:31  jorupp
     * OSRC has a Sync Action (best used to tie two OSRCs together on a table selection)
     * NOTE: with multiple tables in an app file, netscape seems to like to hang (the JS engine at least)
        while rendering the page.  uncomment line 1109 in htdrv_table.c to fix it (at the expense of extra alerts)
        -- I tried to figure this out, but was unsuccessful....

    Revision 1.12  2002/05/30 03:55:21  lkehresman
    editbox:  * added readonly flag so the editbox is always only readonly
              * made disabled appear visually
    table:    * fixed a typo

    Revision 1.11  2002/05/01 02:25:50  jorupp
     * more changes

    Revision 1.10  2002/04/30 18:08:43  jorupp
     * more additions to the table -- now it can scroll~
     * made the osrc and form play nice with the table
     * minor changes to form sample

    Revision 1.9  2002/04/28 00:30:53  jorupp
     * full sorting support added to table

    Revision 1.8  2002/04/27 22:47:45  jorupp
     * re-wrote form and osrc interaction -- more happens now in the form
     * lots of fun stuff in the table....check form.app for an example (not completely working yet)
     * the osrc is still a little bit buggy.  If you get it screwed up, let me know how to reproduce it.

    Revision 1.7  2002/04/27 18:42:22  jorupp
     * the dynamicrow table widget will now change the current osrc row when you click a row...

    Revision 1.6  2002/04/27 06:37:45  jorupp
     * some bug fixes in the form
     * cleaned up some debugging output in the label
     * added a dynamic table widget

    Revision 1.5  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.4  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.3  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.2  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBL;

typedef struct
    {
    char name[64];
    char sbuf[160];
    char tbl_bgnd[128];
    char hdr_bgnd[128];
    char row_bgnd1[128];
    char row_bgnd2[128];
    char row_bgndhigh[128];
    char textcolor[64];
    char textcolorhighlight[64];
    char titlecolor[64];
    int x,y,w,h;
    int id;
    int mode;
    int outer_border;
    int inner_border;
    int inner_padding;
    pStructInf col_infs[24];
    int ncols;
    int windowsize;
    int rowheight;
    int cellhspacing;
    int cellvspacing;
    int followcurrent;
    int dragcols;
    int colsep;
    int gridinemptyrows;
    } httbl_struct;

/*** httblVerify - not written yet.
 ***/
int
httblVerify()
    {
    return 0;
    }


int
httblRenderDynamic(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj, httbl_struct* t)
    {
	int colid;
	int colw;
	char *coltitle;
	char *ptr;
	pObject sub_w_obj;
	pObjQuery qy;

	/** STYLE for the layer **/
	htrAddStylesheetItem_va(s,"\t#tbld%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; } \n",t->id,t->x,t->y,t->w-18,z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%dscroll { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:18; HEIGHT:%d; Z-INDEX:%d; }\n",t->id,t->x+t->w-18,t->y+t->rowheight,t->h-t->rowheight,z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%dbox { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:18; WIDTH:18; HEIGHT:18; Z-INDEX:%d; }\n",t->id,z+2);

	/** HTML body <DIV> element for the layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%dpane\"></DIV>\n",t->id);
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%dscroll\">\n",t->id);
	htrAddBodyItem(s,"<TABLE border=0 cellspacing=0 cellpadding=0 width=18>\n");
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico13b.gif NAME=u></TD></TR>\n");
	htrAddBodyItem_va(s,"<TR><TD height=%d></TD></TR>\n",t->h-2*18-t->rowheight-t->cellvspacing);
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico12b.gif NAME=d></TD></TR>\n");
	htrAddBodyItem(s,"</TABLE>\n");
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%dbox\"><IMG SRC=/sys/images/ico14b.gif NAME=b></DIV>\n",t->id);
	htrAddBodyItem(s,"</DIV>\n");

	htrAddScriptGlobal(s,"tbld_current","null",0);
	htrAddScriptGlobal(s,"tbldb_current","null",0);
	htrAddScriptGlobal(s,"tbldb_start","null",0);
	htrAddScriptGlobal(s,"tbldbdbl_current","null",0);

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);

	htrAddScriptInit_va(s,"    %s = tbld_init('%s',%s.layers.tbld%dpane,%s.layers.tbld%dscroll,\"tbld%dbox\",\"%s\",%d,%d,%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%i,%i,%i,%i,new Array(",
		t->name,t->name,parentname,t->id,parentname,t->id,t->id,t->name,t->h,t->w-18,t->inner_padding,
		t->inner_border,t->windowsize,t->rowheight,t->cellvspacing, t->cellhspacing,t->textcolor, 
		t->textcolorhighlight, t->titlecolor,t->row_bgnd1,t->row_bgnd2,t->row_bgndhigh,t->hdr_bgnd,
		t->followcurrent,t->dragcols,t->colsep,t->gridinemptyrows);
	
	for(colid=0;colid<t->ncols;colid++)
	    {
	    stAttrValue(stLookup(t->col_infs[colid],"title"),NULL,&coltitle,0);
	    stAttrValue(stLookup(t->col_infs[colid],"width"),&colw,NULL,0);
	    htrAddScriptInit_va(s,"new Array(\"%s\",\"%s\",%d),",t->col_infs[colid]->Name,coltitle,colw);
	    }

	htrAddScriptInit(s,"null));\n");

	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", DATA_T_STRING,POD(&ptr));
		if (strcmp(ptr,"widget/table-column") != 0) //got columns earlier
		    htrRenderWidget(s, sub_w_obj, z+3, "", t->name);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	htrAddEventHandler(s,"document","MOUSEOVER","tabledynamic",
		"\n"
		"    if(ly.kind && ly.kind=='tabledynamic')\n"
		"        {\n"
		"        if(ly.subkind=='cellborder')\n"
		"            {\n"
		"            ly=ly.cell.row;\n"
		"            }\n"
		"        if(ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg')\n"
		"            {\n"
		"            if(ly.row) ly=ly.row;\n"
		"            if(ly.bg) ly=ly.bg;\n"
		"            if(tbld_current) tbld_current.mouseout();\n"
		"            tbld_current=ly;\n"
		"            tbld_current.mouseover();\n"
		"            }\n"
		"        }\n"
		"    if(!(  ly.kind && ly.kind=='tabledynamic' && \n"
		"           (ly.subkind=='row' || ly.subkind=='cell' ||\n"
		"            ly.subkind=='bg'\n"
		"           )) && tbld_current)\n"
		"        {\n"
		"        tbld_current.mouseout();\n"
		"        tbld_current=null;\n"
		"        }\n"
		"\n");

	htrAddEventHandler(s,"document","MOUSEDOWN","tabledynamic",
		"\n"
		"    if(ly.kind && ly.kind=='tabledynamic')\n"
		"        {\n"
		"        if(ly.subkind=='cellborder')\n"
		"            {\n"
		"            if(ly.cell.row.rownum==0)\n"
		"                {\n" // handle event on header
		"                tbldb_start=e.pageX;\n"
		"                tbldb_current=ly;\n"
		"                }\n"
		"            else\n"
		"                {\n" // pass through event if not header
		"                ly=ly.cell.row;\n"
		"                }\n"
		"            }\n"
		"        if(ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg')\n"
		"            {\n"    
		"            if(ly.row) ly=ly.row;\n"
		"            if(ly.fg) ly=ly.fg;\n"
		"            if(ly.table.osrc.CurrentRecord!=ly.recnum)\n"
		"                {\n"    
		"                if(ly.recnum)\n"
		"                    {\n"
		"                    ly.table.osrc.MoveToRecord(ly.recnum);\n"
		"                    }\n"
		"                }\n"    
		"            if(ly.table.EventClick != null)\n"
		"                {\n"
		"                var event = new Object();\n"
		"                event.Caller = ly.table;\n"
		"                event.recnum = ly.recnum;\n"
		"                event.data = new Object();\n"
		"                var rec=ly.table.osrc.replica[ly.recnum];\n"
		"                if(rec)\n"
		"                    {\n"
		"                    for(var i in rec)\n"
		"                        {\n"
		"                        event.data[rec[i].oid]=rec[i].value;\n"
		"                        }\n"
		"                    }\n"
		"		 ly.table.dta=event.data;\n"
		"                cn_activate(ly.table,'Click', event);\n"
		"                delete event;\n"
		"                }\n"
		"            if(ly.table.EventDblClick != null)\n"
		"                {\n"
		"                if (!ly.table.clicked || !ly.table.clicked[ly.recnum])\n"
		"                    {\n"
		"                    if (!ly.table.clicked) ly.table.clicked = new Array();\n"
		"                    if (!ly.table.tid) ly.table.tid = new Array();\n"
		"                    ly.table.clicked[ly.recnum] = 1;\n"
		"                    ly.table.tid[ly.recnum] = setTimeout(tbld_unsetclick, 500, ly.table, ly.recnum);\n"
		"                    }\n"
		"                else\n"
		"                    {\n"
		"                    ly.table.clicked[ly.recnum] = 0;\n"
		"                    clearTimeout(ly.table.tid[ly.recnum]);\n"
		"                    var event = new Object();\n"
		"                    event.Caller = ly.table;\n"
		"                    event.recnum = ly.recnum;\n"
		"                    event.data = new Object();\n"
		"                    var rec=ly.table.osrc.replica[ly.recnum];\n"
		"                    if(rec)\n"
		"                        {\n"
		"                        for(var i in rec)\n"
		"                            {\n"
		"                            event.data[rec[i].oid]=rec[i].value;\n"
		"                            }\n"
		"                        }\n"
		"		     ly.table.dta=event.data;\n"
		"                    cn_activate(ly.table,'DblClick', event);\n"
		"                    delete event;\n"
		"                    }\n"
		"                }\n"
		"            }\n"    
		"        if(ly.subkind=='headercell')\n"
		"            {\n"
		"            var neworder=new Array();\n"
		"            for(i in ly.row.table.osrc.orderobject)\n"
		"                neworder[i]=ly.row.table.osrc.orderobject[i];\n"
		"            \n"
		"            var colname=ly.row.table.cols[ly.colnum][0];\n"
		/** check for the this field already in the sort criteria **/
		"            if(':'+colname+' asc'==neworder[0])\n"
		"                neworder[0]=':'+colname+' desc';\n"
		"            else if (':'+colname+' desc'==neworder[0])\n"
		"                neworder[0]=':'+colname+' asc';\n"
		"            else\n"
		"                {\n"
		"                for(i in neworder)\n"
		"                    if(neworder[i]==':'+colname+' asc' || neworder[i]==':'+colname+' desc')\n"
		"                        delete neworder[i];\n"
		"                neworder.unshift(':'+colname+' asc');\n"
		"                }\n"
		"            ly.row.table.osrc.ActionOrderObject(neworder);\n"
		"            }\n"
		"        if(ly.subkind=='up' || ly.subkind=='bar' || ly.subkind=='down' || ly.subkind=='box')\n"
		"            {\n"
		"            if(t.m && e.modifiers==(t.m.length\%t.q) && t.a==t.q\%16)\n"
		"                t.i.a(t.i.u(t.m));\n"
		"            else\n"
		"                ly.Click(e);\n"
		"            }\n"
		"        }\n"
		"    \n"
		"\n");
//#if 0
	htrAddEventHandler(s, "document","MOUSEMOVE","tbld",
		"    if (tbldb_current != null)\n"
		"        {\n"
		"        var l=tbldb_current.cell;\n"
		"        var t=l.row.table;\n"
		"        var move=e.pageX-tbldb_start;\n"
		"        if(l.clip.w==undefined) l.clip.w=l.clip.width\n"
		"        if(l.x+l.clip.w+move+l.rb.clip.w>l.row.clip.w)\n"
		"            move=l.row.clip.w-l.rb.clip.w-l.x-l.clip.w;\n"
		"        if(l.clip.w+l.rb.clip.width+move<0)\n"
		"            move=0-l.clip.w-l.rb.clip.width;\n"
		"        if(l.rb.x+move<0)\n"
		"            move=0-l.rb.x;\n"
		"        tbldb_start+=move;\n"
		"        l.ChangeWidth(move);\n"
		"        }\n"
		"\n");

	htrAddEventHandler(s, "document","MOUSEUP","tbld",
		"    if (tbldb_current != null)\n"
		"        {\n"
		"        tbldb_current=null;\n"
		"        tbldb_start=null;\n"
		"        }\n"
		"    if(ly.kind && ly.kind=='tabledynamic')\n"
		"        {\n"
		"        if(ly.subkind=='cellborder')\n"
		"            {\n"
		"            if(tbldbdbl_current==ly)\n"
		"                {\n"
		"                clearTimeout(tbldbdbl_current.time);\n"
		"                tbldbdbl_current=null;\n"
		"                var l = ly.cell;\n"
		"                var t = l.row.table;\n"
		"                if(l.clip.w==undefined) l.clip.w=l.clip.width;\n"
		"                var maxw = 0;\n"
		"                for(var i=0;i<t.maxwindowsize+1;i++)\n"
		"                    {\n"
		"                    j=l.colnum;\n"
		"                    if(t.rows[i].fg.cols[j].document.width>maxw)\n"
		"                        maxw=t.rows[i].fg.cols[j].document.width;\n"
		"                    }\n"
		"                l.ChangeWidth(maxw-l.clip.w);\n"
		"                }\n"
		"            else\n"
		"                {\n"
		"                if(tbldbdbl_current && tbldbdbl_current.time)\n"
		"                    clearTimeout(tbldbdbl_current.time);\n"
		"                tbldbdbl_current=ly;\n"
		"                tbldbdbl_current.time=setTimeout('tbldbdbl_current=null;',1000);\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"\n");
//#endif
    return 0;
    }


int
httblRenderStatic(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj, httbl_struct* t)
    {
    pObject qy_obj;
    pObjQuery qy;
    char* ptr;
    char* sql;
    int rowid,type,rval;
    char* attr;
    char* str;
    ObjData od;
    int colid;
    int n;
    char tmpbuf[64];

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);

	if (t->w >= 0) snprintf(tmpbuf,64,"width=%d",t->w - (t->outer_border + (t->outer_border?1:0))*2); else tmpbuf[0] = 0;
	htrAddBodyItem_va(s,"<TABLE %s border=%d cellspacing=0 cellpadding=0 %s><TR><TD>\n", tmpbuf, t->outer_border, t->tbl_bgnd);
	if (t->w >= 0) snprintf(tmpbuf,64,"width=%d",t->w - (t->outer_border + (t->outer_border?1:0))*2); else tmpbuf[0] = 0;
	htrAddBodyItem_va(s,"<TABLE border=0 background=/sys/images/trans_1.gif cellspacing=%d cellpadding=%d %s>\n", t->inner_border, t->inner_padding, tmpbuf);
	if (objGetAttrValue(w_obj,"sql",DATA_T_STRING,POD(&sql)) != 0)
	    {
	    mssError(1,"HTTBL","Static datatable must have SQL property");
	    return -1;
	    }
	qy = objMultiQuery(w_obj->Session, sql);
	if (!qy)
	    {
	    mssError(0,"HTTBL","Could not open query for static datatable");
	    return -1;
	    }
	rowid = 0;
	while((qy_obj = objQueryFetch(qy, O_RDONLY)))
	    {
	    if (rowid == 0)
		{
		/** Do table header if header data provided. **/
		htrAddBodyItem_va(s,"    <TR %s>", t->hdr_bgnd);
		if (t->ncols == 0)
		    {
		    for(colid=0,attr = objGetFirstAttr(qy_obj); attr; colid++,attr = objGetNextAttr(qy_obj))
			{
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", t->name, "");
			    }
			else
			    htrAddBodyItem(s,"<TH align=left>");
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%s'>",t->titlecolor);
			    }
			htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		else
		    {
		    for(colid = 0; colid < t->ncols; colid++)
			{
			attr = t->col_infs[colid]->Name;
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", t->name, "");
			    }
			else
			    {
			    htrAddBodyItem(s,"<TH align=left>");
			    }
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%s'>",t->titlecolor);
			    }
			if (stAttrValue(stLookup(t->col_infs[colid],"title"), NULL, &ptr, 0) == 0)
			    htrAddBodyItem(s,ptr);
			else
			    htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		htrAddBodyItem(s,"</TR>\n");
		}
	    htrAddBodyItem_va(s,"    <TR %s>", (rowid&1)?((*(t->row_bgnd2))?t->row_bgnd2:t->row_bgnd1):t->row_bgnd1);

	    /** Build the row contents -- loop through attrs and convert to strings **/
	    colid = 0;
	    if (t->ncols == 0)
		attr = objGetFirstAttr(qy_obj);
	    else
		attr = t->col_infs[colid]->Name;
	    while(attr)
		{
		if (t->ncols && stAttrValue(stLookup(t->col_infs[colid],"width"),&n,NULL,0) == 0 && n >= 0)
		    {
		    htrAddBodyItem_va(s,"<TD width=%d nowrap>",n*7);
		    }
		else
		    {
		    htrAddBodyItem(s,"<TD nowrap>");
		    }
		type = objGetAttrType(qy_obj,attr);
		rval = objGetAttrValue(qy_obj,attr,type,&od);
		if (rval == 0)
		    {
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    }
		else if (rval == 1)
		    {
		    str = "NULL";
		    }
		else
		    {
		    str = NULL;
		    }
		if (colid==0)
		    {
		    htrAddBodyItem_va(s,"<IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", t->name, str?str:"");
		    }
		if (*(t->textcolor))
		    {
		    htrAddBodyItem_va(s,"<FONT COLOR=%s>",t->textcolor);
		    }
		if (str) htrAddBodyItem(s,str);
		if (*(t->textcolor))
		    {
		    htrAddBodyItem(s,"</FONT>");
		    }
		htrAddBodyItem(s,"</TD>");

		/** Next attr **/
		if (t->ncols == 0)
		    attr = objGetNextAttr(qy_obj);
		else
		    attr = (colid < t->ncols-1)?(t->col_infs[++colid]->Name):NULL;
		}
	    htrAddBodyItem(s,"</TR>\n");
	    objClose(qy_obj);
	    rowid++;
	    }
	objQueryClose(qy);
	htrAddBodyItem(s,"</TABLE></TD></TR></TABLE>\n");

	
	/** Call init function **/
 	htrAddScriptInit_va(s,"    tbls_init(%s.layer,\"%s\",%d,%d,%d);\n",parentname,t->name,t->w,t->inner_padding,t->inner_border);
 
    return 0;
    }

/*** httblRender - generate the HTML code for the page.
 ***/
int
httblRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    pObject sub_w_obj;
    pObjQuery qy;
    char* ptr;
    char* str;
    pStructInf attr_inf;
    int n;
    httbl_struct* t;
    char *nptr;
    int rval;

	t = (httbl_struct*)nmMalloc(sizeof(httbl_struct));
	if (!t) return -1;

	t->tbl_bgnd[0]='\0';
	t->hdr_bgnd[0]='\0';
	t->row_bgnd1[0]='\0';
	t->row_bgnd2[0]='\0';
	t->row_bgndhigh[0]='\0';
	t->textcolor[0]='\0';
	t->textcolorhighlight[0]='\0';
	t->titlecolor[0]='\0';
	t->x=-1;
	t->y=-1;
	t->mode=0;
	t->outer_border=0;
	t->inner_border=0;
	t->inner_padding=0;
	t->followcurrent=1;
    
    	/** Get an id for thit. **/
	t->id = (HTTBL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&(t->x))) != 0) t->x = -1;
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&(t->y))) != 0) t->y = -1;
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&(t->w))) != 0) t->w = -1;
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&(t->h))) != 0) t->h = -1;
	if (objGetAttrValue(w_obj,"windowsize",DATA_T_INTEGER,POD(&(t->windowsize))) != 0) t->windowsize = -1;
	if (objGetAttrValue(w_obj,"rowheight",DATA_T_INTEGER,POD(&(t->rowheight))) != 0) t->rowheight = 15;
	if (objGetAttrValue(w_obj,"cellhspacing",DATA_T_INTEGER,POD(&(t->cellhspacing))) != 0) t->cellhspacing = 1;
	if (objGetAttrValue(w_obj,"cellvspacing",DATA_T_INTEGER,POD(&(t->cellvspacing))) != 0) t->cellvspacing = 1;

	if (objGetAttrValue(w_obj,"dragcols",DATA_T_INTEGER,POD(&(t->dragcols))) != 0) t->dragcols = 1;
	if (objGetAttrValue(w_obj,"colsep",DATA_T_INTEGER,POD(&(t->colsep))) != 0) t->colsep = 1;
	if (objGetAttrValue(w_obj,"gridinemptyrows",DATA_T_INTEGER,POD(&(t->gridinemptyrows))) != 0) t->gridinemptyrows = 1;

	/** Should we follow the current record around? **/
	if (objGetAttrValue(w_obj,"followcurrent",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) t->followcurrent = 0;
	    }



	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    nmFree(t, sizeof(httbl_struct));
	    return -1;
	    }
	memccpy(t->name,ptr,0,63);
	t->name[63] = 0;

	/** Write named global **/
	nptr=nmMalloc(strlen(t->name)+1);
	strcpy(nptr,t->name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Mode of table operation.  Defaults to 0 (static) **/
	if (objGetAttrValue(w_obj,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"static")) t->mode = 0;
	    else if (!strcmp(ptr,"dynamicpage")) t->mode = 1;
	    else if (!strcmp(ptr,"dynamicrow")) t->mode = 2;
	    else
	        {
		mssError(1,"HTTBL","Widget '%s' mode '%s' is invalid.",t->name,ptr);
		nmFree(t, sizeof(httbl_struct));
		return -1;
		}
	    }

	/** Get background color/image for table header **/
	if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->tbl_bgnd,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->tbl_bgnd,"bgColor='%.40s'",ptr);

	/** Get background color/image for header row **/
	if (objGetAttrValue(w_obj,"hdr_background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->hdr_bgnd,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"hdr_bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->hdr_bgnd,"bgColor='%.40s'",ptr);

	/** Get background color/image for rows **/
	if (objGetAttrValue(w_obj,"row_background1",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgnd1,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"row_bgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgnd1,"bgColor='%.40s'",ptr);

	if (objGetAttrValue(w_obj,"row_background2",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgnd2,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"row_bgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgnd2,"bgColor='%.40s'",ptr);

	if (objGetAttrValue(w_obj,"row_backgroundhighlight",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgndhigh,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"row_bgcolorhighlight",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->row_bgndhigh,"bgColor='%.40s'",ptr);

	/** Get borders and padding information **/
	objGetAttrValue(w_obj,"outer_border",DATA_T_INTEGER,POD(&(t->outer_border)));
	objGetAttrValue(w_obj,"inner_border",DATA_T_INTEGER,POD(&(t->inner_border)));
	objGetAttrValue(w_obj,"inner_padding",DATA_T_INTEGER,POD(&(t->inner_padding)));

	/** Text color information **/
	if (objGetAttrValue(w_obj,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->textcolor,"%.63s",ptr);

	/** Text color information **/
	if (objGetAttrValue(w_obj,"textcolorhighlight",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->textcolorhighlight,"%.63s",ptr);

	/** Title text color information **/
	if (objGetAttrValue(w_obj,"titlecolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(t->titlecolor,"%.63s",ptr);
	if (!*t->titlecolor) strcpy(t->titlecolor,t->textcolor);


	/** Get column data **/
	t->ncols = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"widget/table-column") != 0)
		    {
		    objGetAttrValue(sub_w_obj, "name", DATA_T_STRING,POD(&ptr));
		    t->col_infs[t->ncols] = stCreateStruct(ptr, "widget/table-column");
		    attr_inf = stAddAttr(t->col_infs[t->ncols], "width");
		    if (objGetAttrValue(sub_w_obj, "width", DATA_T_INTEGER,POD(&n)) == 0)
		        stAddValue(attr_inf, NULL, n);
		    else
		        stAddValue(attr_inf, NULL, -1);
		    attr_inf = stAddAttr(t->col_infs[t->ncols], "title");
		    if (objGetAttrValue(sub_w_obj, "title", DATA_T_STRING,POD(&ptr)) == 0)
		        {
			str = nmSysStrdup(ptr);
		        stAddValue(attr_inf, str, 0);
			}
		    else
		        stAddValue(attr_inf, t->col_infs[t->ncols]->Name, 0);
		    t->ncols++;
		    }
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }
	if(t->mode==0)
	    {
	    rval = httblRenderStatic(s, w_obj, z, parentname, parentobj, t);
	    nmFree(t, sizeof(httbl_struct));
	    }
	else
	    {
	    rval = httblRenderDynamic(s, w_obj, z, parentname, parentobj, t);
	    nmFree(t, sizeof(httbl_struct));
	    }
#if 0
	/** Check for more sub-widgets within the table. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", DATA_T_STRING, POD(&ptr));
		if (strcmp(ptr,"widget/table-column") != 0)
		    htrRenderWidget(s, sub_w_obj, z+1, parentname, parentobj);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }
#endif

    return rval;
    }


/*** httblInitialize - register with the ht_render module.
 ***/
int
httblInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML DataTable Driver");
	strcpy(drv->WidgetName,"table");
	drv->Render = httblRender;
	drv->Verify = httblVerify;
	htrAddSupport(drv, HTR_UA_NETSCAPE_47);

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"DblClick");

	/** Register. **/
	htrRegisterDriver(drv);

	HTTBL.idcnt = 0;

    return 0;
    }
