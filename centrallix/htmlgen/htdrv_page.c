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
#include "centrallix.h"

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
/* Module: 	htdrv_page.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Widget driver for the overall HTML page.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_page.c,v 1.52 2002/12/24 09:41:07 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_page.c,v $

    $Log: htdrv_page.c,v $
    Revision 1.52  2002/12/24 09:41:07  jorupp
     * move output of cn_browser to ht_render, also moving up above the first place where it is needed

    Revision 1.51  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.50  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.49  2002/08/23 17:31:05  lkehresman
    moved window_current global to the page widget so it is always defined
    even if it is null.  This prevents javascript errors from the objectsource
    widget when no windows exist in the app.

    Revision 1.48  2002/08/15 13:58:16  pfinley
    Made graphical window closing and shading properties of the window widget,
    rather than globally of the page.

    Revision 1.47  2002/08/14 20:16:38  pfinley
    Added some visual effets for the window:
     - graphical window shading (enable by setting gshade="true")
     - added 3 new closing types (enable by setting closetype="shrink1","shrink2", or "shrink3")

    These are gloabl changes, and can only be set on the page widget... these
    will become part of theming once it is implemented (i think).

    Revision 1.46  2002/08/13 05:23:25  gbeeley
    I should have documented this one from the beginning.  The sense of the
    'flags' parameter ended up being changed, breaking the static mode table.
    Changed the implementation back to using the original sense of this
    param (bitmask 1 = allow mouse focus, 2 = allow kbd focus).  Tested on
    static table, dropdown, datetime, textarea, editbox.

    Revision 1.45  2002/08/12 17:51:16  pfinley
    - added an attract option to the page widget. if this is set, centrallix
      windows will attract to the edges of the browser window. set to how many
      pixels from border to attract.
    - also fixed a mainlayer issue with the window widget which allowed for
      the contents of a window to be draged and shaded (very interesting :)

    Revision 1.44  2002/08/08 16:23:07  lkehresman
    Added backwards compatible page area handling because the datetime widget
    needs to do some funky stuff with page areas (areas inside of areas) that
    worked with our previous model, but not with the mainlayer stuff.  Just
    added a few checks to see if mainlayer existed or not.

    Revision 1.43  2002/08/05 21:06:29  pfinley
    created a property that can optionally be added to layers that will keep
    the current keyboard focus if it is clicked on. I added this property so
    that keyboard focus would not be lost when you move a window. add the
    property '.keep_kbd_focus = true' to any layer that you want to keep
    the keyboard focus on (any layer without this property will act normally).

    Revision 1.42  2002/08/05 19:20:08  lkehresman
    * Revamped the GUI for the DropDown to make it look cleaner
    * Added the function pg_resize_area() so page areas can be resized.  This
      allows the dropdown and datetime widgets to contain focus on their layer
      that extends down.  Previously it was very kludgy, now it works nicely
      by just extending the page area for that widget.
    * Reworked the dropdown widget to take advantage of the resize function
    * Added .mainlayer attributes to the editbox widget (new page functionaly
      requires .mainlayer properties soon to be standard in all widgets).

    Revision 1.41  2002/07/31 13:35:58  lkehresman
    * Made x.mainlayer always point to the top layer in dropdown
    * Fixed a netscape crash bug with the event stuff from the last revision of dropdown
    * Added a check to the page event stuff to make sure that pg_curkbdlayer is set
        before accessing the pg_curkbdlayer.getfocushandler() function. (was causing
        javascript errors before because of the special case of the dropdown widget)

    Revision 1.40  2002/07/30 16:09:05  pfinley
    Added Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove events to the
    radiobutton widget.

    Note: MouseOut does not use the MOUSEOUT event.  It had to be done manually
          since MouseOut's on the whole widget can not be distingushed if a widget
          has more than one layer visible at one time. For this I added a global
          variable: util_cur_mainlayer which is set to the mainlayer of the widget
          (the layer that the event connector functions are attached to) on a
          MouseOver event.  In order to use this in other widgets, each layer in
          the widget must:  1) have a .kind property, 2) have a .mainlayer property,
          3) set util_cur_mainlayer to the mainlayer upon a MouseOver event on the
          widget, and 4) set util_cur_mainlayer back to null upon a "MouseOut"
          event. (See code for example)

    I also:
    - changed the enabled/disabled to be properties of the mainlayer rather
      than of each layer in the widget.
    - changed toggle function to only toggle if selecting a new option (eliminated
      flicker and was needed for DataChange event).
    - removed the .parentPane pointer and replaced it with mainlayer.
    [ how about that for a log message :) ]

    Revision 1.39  2002/07/30 12:56:29  lkehresman
    Renamed the "Load" action and the "Page" parameter to be "LoadPage" and
    "Source", which is what is used in the "html" widget.  We should probably
    keep actions as similar and standard as possible.

    Revision 1.38  2002/07/29 18:21:52  pfinley
    Changed a static while loop conditional to be an if statement.  This was
    causing an infinite loop when e.target pointed to an object.

    Revision 1.37  2002/07/26 16:57:44  lkehresman
    a couple bugfixes to the scrollbar detection

    Revision 1.36  2002/07/26 14:42:04  lkehresman
    Added detection for scrollbars.  If they exist, set a timeout function
    to watch for scrolling activity.  If scroling happens, move the textarea
    layer (that we use to gain keypress input).

    Revision 1.35  2002/07/24 21:26:35  pfinley
    this is needed incase a page does not have any connectors.

    Revision 1.34  2002/07/23 13:45:32  lkehresman
    Added the "Load" action with the "Page" parameter to the page widget.  This
    enables us to load different applications with button clicks or other events.
    This will be useful for a program that spans several different app structure
    files. (i.e. Kardia)

    Revision 1.33  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.32  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.31  2002/07/19 14:54:22  pfinley
    - Modified the page mousedown & mouseover handlers so that the cursor ibeam
    "can't" be clicked on (only supports the global cursor).
    - Modified the editbox & textarea files to support the new global cursor.

    Revision 1.30  2002/07/18 20:12:40  lkehresman
    Added support for a loadstatus icon to be displayed, hiding the drawing
    of the visible windows.  This looks MUCH nicer when loading Kardia or
    any other large apps.  It is completely optional part of the page widget.
    To take advantage of it, put the parameter "loadstatus" equal to "true"
    in the page widget.

    Revision 1.29  2002/07/18 14:27:25  pfinley
    fixed another bug i created yesterday. cleaned up code a bit.

    Revision 1.28  2002/07/17 20:09:12  lkehresman
    Two changes to htdrv_page event handlers
      *  getfocushandler() function now is passed a reference to the current
         pg_area object as the 5th parameter.
      *  losefocushandler() is now an optional function (like the
         getfocushandler function is).  I think it was supposed to work like
         this originally, but someone forgot to put the check in there.

    Revision 1.27  2002/07/17 19:45:44  pfinley
    fixed a javascript error that i created

    Revision 1.26  2002/07/17 18:59:57  pfinley
    The flag parameter (f) of pg_addarea() was not being used, so I put it to use. If
    the flag is set to 0, it will not draw the box around the area when it has mouse
    focus; any other value and it will.

    Revision 1.25  2002/07/17 15:32:17  pfinley
    Changed losefocushandler so that it is only called when another layer other than itself is clicked (getfocushandler is still called).

    Revision 1.24  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.23  2002/07/16 16:12:07  pfinley
    added a script init that accidentally got deleted when converting to a js file.

    Revision 1.22  2002/07/15 22:41:02  lkehresman
    Whoops!  Got copy-happy and removed an event with all the functions.

    Revision 1.21  2002/07/15 21:58:02  lkehresman
    Split the page out into include scripts

    Revision 1.20  2002/07/08 23:21:38  jorupp
     * added a global object, cn_browser with two boolean properties -- netscape47 and mozilla
        The corresponding one will be set to true by the page
     * made one minor change to the form to get around the one .layers reference in the form (no .document references)
        It _should_ work, however I don't have a _simple_ form test to try it on, so it'll have to wait

    Revision 1.19  2002/07/07 00:21:46  jorupp
     * added Mozilla support for the page
       * BARELY WORKS -- hardly any events checked
       * ping works (after several days of trying)

    Revision 1.18  2002/06/24 19:45:35  pfinley
    eliminated the flicker in the border of an edit box when it is clicked when it already has keyboard focus..

    Revision 1.17  2002/06/19 21:21:50  lkehresman
    Bumped up the zIndex values so the hilights wouldn't be hidden.

    Revision 1.16  2002/06/19 18:35:25  pfinley
    fixed bug in edit box which didn't remove the focus border when clicking a place other than another edit box.

    Revision 1.15  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.14  2002/06/03 05:31:39  lkehresman
    Fixed some global variables that should have been local.

    Revision 1.13  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.12  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.11  2002/05/08 00:46:30  jorupp
     * added client-side support for the session watchdog.  The client will ping every timer/2 seconds (just to be safe)

    Revision 1.10  2002/04/28 03:19:53  gbeeley
    Fixed a bit of a bug in ht_render where it did not properly set the
    length on the StrValue structures when adding script functions.  This
    was basically causing some substantial heap corruption.

    Revision 1.9  2002/03/23 01:18:09  lkehresman
    Fixed focus detection and form notification on editbox and anything that
    uses keyboard input.

    Revision 1.8  2002/03/16 06:53:34  gbeeley
    Added modal-layer function at the page level.  Calling pg_setmodal(l)
    causes all mouse activity outside of layer l to be ignored.  Useful
    when you need to require the user to act on a certain window/etc.
    Call pg_setmodal(null) to clear the modal status.  Note: keep the
    modal layer simple!  The algorithm does quite a bit of poking around
    to figure out whether the activity is targeted within the given modal
    layer or not.  NOTE:  for modal windows, if you want to keep the user
    from clicking the 'x' on the window, set the window's mainLayer to be
    modal instead of the window itself, although the user will not be able
    to even drag the window in that case, which might be desirable.

    Revision 1.7  2002/03/15 22:40:47  gbeeley
    Modified key input logic in the page widget to improve key debouncing
    and make key repeat rate and delay a bit more natural and more
    consistent across different machines and platforms.

    Revision 1.6  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.5  2002/02/27 01:38:51  jheth
    Initial commit of object source

    Revision 1.4  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** htpageVerify - not written yet.
 ***/
int
htpageVerify()
    {
    return 0;
    }

typedef struct
    {
    char kbfocus1[64];	/* kb focus = 3d raised */
    char kbfocus2[64];
    char msfocus1[64];	/* ms focus = black rectangle */
    char msfocus2[64];
    char dtfocus1[64];	/* dt focus = navyblue rectangle */
    char dtfocus2[64];
    } HtPageStruct, *pHtPageStruct;

typedef struct
    {
    char bgstr[128];
    int show;
    } HtPageStatusStruct, *pHtPageStatusStruct;
HtPageStatusStruct htPageStatus;

/*** htpageRenderCommon - do everything common to both browsers
 ***/
int
htpageRenderCommon(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj,pHtPageStruct t,const char* layer)
    {
    char *ptr;
    char *nptr;
    char name[64];
    int attract = 0;

	strcpy(t->kbfocus1,"#ffffff");	/* kb focus = 3d raised */
	strcpy(t->kbfocus2,"#7a7a7a");
	strcpy(t->msfocus1,"#000000");	/* ms focus = black rectangle */
	strcpy(t->msfocus2,"#000000");
	strcpy(t->dtfocus1,"#000080");	/* dt focus = navyblue rectangle */
	strcpy(t->dtfocus2,"#000080");

    	/** If not at top-level, don't render the page. **/
	/** Z is set to 10 for the top-level page. **/
	if (z != 10) return 0;

    	/** Check for a title. **/
	if (objGetAttrValue(w_obj,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s, "    <TITLE>%s</TITLE>\n",ptr);
	    }

    	/** Check for page load status **/
	htPageStatus.show = 0;
	if (objGetAttrValue(w_obj,"loadstatus",DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"yes") || !strcmp(ptr,"true")))
	    {
	    htPageStatus.show = 1;
	    }

	strcpy(htPageStatus.bgstr, "");
	/** Check for bgcolor. **/
	if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    snprintf(htPageStatus.bgstr, 128, " BGCOLOR=%s", ptr);
	    htrAddBodyParam_va(s, " BGCOLOR=%s",ptr);
	    }
	if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    snprintf(htPageStatus.bgstr, 128, " BACKGROUND=%s", ptr);
	    htrAddBodyParam_va(s, " BACKGROUND=%s",ptr);
	    }

	/** Check for text color **/
	if (objGetAttrValue(w_obj,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " TEXT=%s",ptr);
	    }

	/** Keyboard Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"kbdfocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->kbfocus1,ptr,0,63);
	    t->kbfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"kbdfocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->kbfocus2,ptr,0,63);
	    t->kbfocus2[63]=0;
	    }

	/** Mouse Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"mousefocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->msfocus1,ptr,0,63);
	    t->msfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"mousefocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->msfocus2,ptr,0,63);
	    t->msfocus2[63]=0;
	    }

	/** Data Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"datafocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->dtfocus1,ptr,0,63);
	    t->dtfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"datafocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(t->dtfocus2,ptr,0,63);
	    t->dtfocus2[63]=0;
	    }
   
	/** Cx windows attract to browser edges? if so, by how much **/
	if (objGetAttrValue(w_obj,"attract",DATA_T_INTEGER,POD(&ptr)) == 0)
	    attract = (int)ptr;

	/** Add global for page metadata **/
	htrAddScriptGlobal(s, "page", "new Object()", 0);

	/** Add a list of highlightable areas **/
	htrAddScriptGlobal(s, "pg_arealist", "new Array()", 0);
	htrAddScriptGlobal(s, "pg_keylist", "new Array()", 0);
	htrAddScriptGlobal(s, "pg_curarea", "null", 0);
	htrAddScriptGlobal(s, "pg_curlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdarea", "null", 0);
	htrAddScriptGlobal(s, "pg_lastkey", "-1", 0);
	htrAddScriptGlobal(s, "pg_lastmodifiers", "null", 0);
	htrAddScriptGlobal(s, "pg_keytimeoutid", "null", 0);
	htrAddScriptGlobal(s, "pg_modallayer", "null", 0);
	htrAddScriptGlobal(s, "pg_attract", "null", 0);
	htrAddScriptGlobal(s, "pg_gshade", "null", 0);
	htrAddScriptGlobal(s, "pg_closetype", "null", 0);
	htrAddScriptGlobal(s, "fm_current", "null", 0);
	htrAddScriptGlobal(s, "osrc_current", "null", 0);
	htrAddScriptGlobal(s, "pg_insame", "false", 0);
	htrAddScriptGlobal(s, "cn_browser", "null", 0);
	htrAddScriptGlobal(s, "ibeam_current", "null", 0);
	htrAddScriptGlobal(s, "window_current","null",0);
	htrAddScriptGlobal(s, "util_cur_mainlayer", "null", 0);

	/** Add script include to get function declarations **/
	htrAddScriptInclude(s, "/sys/js/htdrv_page.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0);

	/** Write named global **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,'\0',63);
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	if(s->WidgetSet == HTR_UA_MOZILLA)
	    {
	    htrAddScriptInit_va(s, "    %s = pg_init(%s.getElementById('pgtop'),%d);\n", name, parentname, attract);
	    }
	else
	    {
	    htrAddScriptInit_va(s, "    %s = pg_init(%s.layers.pgtop,%d);\n", name, parentname, attract);
	    }

    return 0;
    }

/*** htpageRenderNtsp47xDefault - generate the HTML code for Netscape 4.7x default style
 ***/
int
htpageRenderNtsp47xDefault(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    pObject sub_w_obj;
    pObjQuery qy;
    int watchdogtimer;
    HtPageStruct t;

        htpageRenderCommon(s,w_obj,z,parentname,parentobj,&t,"DIV");

	/** Add focus box **/
	htrAddStylesheetItem(s, 
		"\t#pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:1; Z-INDEX:0; }\n"
		"\t#pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"\t#pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:20; Z-INDEX:20; }\n"
		"\t#pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; Z-INDEX:0; }\n");

	if (htPageStatus.show == 1)
	    {
	    htrAddStylesheetItem(s, "\t#pgstat { POSITION:absolute; VISIBILITY:visible; LEFT:0;TOP:0;WIDTH:100%;HEIGHT:99%; Z-INDEX:100000;}\n");
	    htrAddBodyItem_va(s, "<DIV ID=\"pgstat\"><BODY%s>", htPageStatus.bgstr);
	    htrAddBodyItem   (s, "<TABLE width=100\% height=100\% cellpadding=20><TR><TD valign=top><IMG src=/sys/images/loading.gif></TD></TR></TABLE></BODY></DIV>\n");
	    }
	htrAddBodyItem(s, "<DIV ID=\"pgtop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pglft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgtvl\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgktop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgklft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgping\"></DIV>\n");

	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	htrAddScriptInit_va(s,"    pg_ping_init(%s.layers.pgping,%i);\n",parentname,watchdogtimer/2*1000);
	//htrAddScriptInit_va(s,"    pg_ping_init(%s.layers.pgping,%i);\n",parentname,watchdogtimer/2*1000);

	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandler(s, "document", "MOUSEMOVE","pg",
		"    ly = (e.target.layer != null)?e.target.layer:e.target;\n"
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (pg_curlayer != null)\n"
		"        {\n"
		"        for(var i=0;i<pg_arealist.length;i++) if (pg_curlayer == pg_arealist[i].layer && e.x >= pg_arealist[i].x &&\n"
		"                e.y >= pg_arealist[i].y && e.x < pg_arealist[i].x+pg_arealist[i].width &&\n"
		"                e.y < pg_arealist[i].y+pg_arealist[i].height && pg_curarea != pg_arealist[i])\n"
		"            {\n"
		"            if (pg_curarea == pg_arealist[i]) break;\n"
		"            pg_curarea = pg_arealist[i];\n"
		"            if (pg_curarea.flags & 1)\n"
		"                {\n"
		"                var x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"                var y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"                var w = pg_curarea.width;\n"
		"                var h = pg_curarea.height;\n"
		"                pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft, page.mscolor1, page.mscolor2, document.layers.pgktop.zIndex-1);\n"
		"                }\n"
		"            break;\n"
		"            }\n"
		"        }\n"
		"    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer != pg_curarea.layer) || (e.target == pg_curarea.layer)))\n"
		"        {\n"
		"        if (pg_curarea.flags & 1) pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOUT", "pg",
		"    ly = (e.target.layer != null)?e.target.layer:e.target;\n"
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (ibeam_current && e.target == ibeam_current)\n"
		"        {\n"
		"        pg_curlayer = pg_curkbdlayer;\n"
		"        pg_curarea = pg_curkbdarea;\n"
		"        return false;\n"
		"        }\n" 
		"    if (e.target == pg_curlayer) pg_curlayer = null;\n"
		"    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer != pg_curarea.layer) || (e.target == pg_curarea.layer)))\n"
		"        {\n"
		"        if (pg_curarea.flags & 1) pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOVER", "pg",
		"    ly = (e.target.layer != null)?e.target.layer:e.target;\n"
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (ibeam_current && e.target == ibeam_current)\n"
		"        {\n"
		"        pg_curlayer = pg_curkbdlayer;\n"
		"        pg_curarea = pg_curkbdarea;\n"
		"        return false;\n"
		"        }\n" 
		"    if (e.target != null && e.target.pageX != null)\n"
		"        {\n"
		"        pg_curlayer = e.target;\n"
		"        if (pg_curlayer.mainlayer != null) pg_curlayer = pg_curlayer.mainlayer;\n"
		"        }\n" );

	/** CLICK event handler is for making mouse focus the keyboard focus **/
	htrAddEventHandler(s, "document", "MOUSEDOWN", "pg",
		"    ly = (e.target.layer != null)?e.target.layer:e.target;\n"
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		//"    if (pg_curlayer) alert('cur layer kind = ' + ly.kind + ' ' + ly.id);\n"
		"    if (ibeam_current && e.target.layer == ibeam_current) return false;\n"
		"    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer != pg_curarea.layer) || (e.target == pg_curarea.layer)))\n"
		"        {\n"
		"        if (pg_curarea.flags & 1) pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n"
		"    if (pg_curarea != null)\n"
		"        {\n"
		"        var x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"        var y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"        var w = pg_curarea.width;\n"
		"        var h = pg_curarea.height;\n"
		"        if (pg_curkbdlayer && pg_curkbdlayer.losefocushandler && pg_curlayer != pg_curkbdlayer)\n"
		"            {\n"
		"            if (!pg_curkbdlayer.losefocushandler()) return true;\n"
		"            pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
		"            }\n"
		"        var prevLayer = pg_curkbdlayer;\n"
		"        pg_curkbdarea = pg_curarea;\n"
		"        pg_curkbdlayer = pg_curlayer;\n"
		"        if (pg_curkbdlayer && pg_curkbdlayer.getfocushandler)\n"
		"            {\n"
		"            var v=pg_curkbdlayer.getfocushandler(e.pageX-pg_curarea.layer.pageX,e.pageY-pg_curarea.layer.pageY,pg_curarea.layer,pg_curarea.cls,pg_curarea.name,pg_curarea);\n"
		"            if (v & 1)\n"
		"                {\n"
		"                if (prevLayer != pg_curlayer)\n"
		"                    pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
		"                }\n"
		"            if (v & 2)\n"
		"                {\n"
		"                if (pg_curlayer.pg_dttop != null)\n"
		"                    {\n"
		"                    pg_hidebox(pg_curlayer.pg_dttop,pg_curlayer.pg_dtbtm,pg_curlayer.pg_dtrgt,pg_curlayer.pg_dtlft);\n"
		"                    }\n"
		"                else\n"
		"                    {\n"
		"                    pg_curlayer.pg_dttop = new Layer(1152);\n"
		"                    //pg_curlayer.pg_dttop.document.write('<IMG SRC=/sys/images/trans_1.gif width=1152 height=1>');\n"
		"                    //pg_curlayer.pg_dttop.document.close();\n"
		"                    pg_curlayer.pg_dtbtm = new Layer(1152);\n"
		"                    //pg_curlayer.pg_dtbtm.document.write('<IMG SRC=/sys/images/trans_1.gif width=1152 height=1>');\n"
		"                    //pg_curlayer.pg_dtbtm.document.close();\n"
		"                    pg_curlayer.pg_dtrgt = new Layer(2);\n"
		"                    //pg_curlayer.pg_dtrgt.document.write('<IMG SRC=/sys/images/trans_1.gif height=864 width=1>');\n"
		"                    //pg_curlayer.pg_dtrgt.document.close();\n"
		"                    pg_curlayer.pg_dtlft = new Layer(2);\n"
		"                    //pg_curlayer.pg_dtlft.document.write('<IMG SRC=/sys/images/trans_1.gif height=864 width=1>');\n"
		"                    //pg_curlayer.pg_dtlft.document.close();\n"
		"                    }\n"
		"                pg_mkbox(pg_curlayer,x-1,y-1,w+2,h+2, 1,  pg_curlayer.pg_dttop,pg_curlayer.pg_dtbtm,pg_curlayer.pg_dtrgt,pg_curlayer.pg_dtlft, page.dtcolor1, page.dtcolor2, document.layers.pgtop.zIndex+100);\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"    else if (pg_curkbdlayer != null && !ly.keep_kbd_focus)\n"
		"        {\n"
		"        pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
		"        if (pg_curkbdlayer.losefocushandler && !pg_curkbdlayer.losefocushandler()) return true;\n"
		"        pg_curkbdarea = null;\n"
		"        pg_curkbdlayer = null;\n"
		"        }\n");

	/** This resets the keyboard focus. **/
	htrAddEventHandler(s, "document", "MOUSEUP", "pg",
		"    ly = (e.target.layer != null)?e.target.layer:e.target;\n"
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    setTimeout('document.layers.pginpt.document.tmpform.x.focus()',10);\n");

	/** Set colors for the focus layers **/
	htrAddScriptInit_va(s, "    page.kbcolor1 = '%s';\n    page.kbcolor2 = '%s';\n",t.kbfocus1,t.kbfocus2);
	htrAddScriptInit_va(s, "    page.mscolor1 = '%s';\n    page.mscolor2 = '%s';\n",t.msfocus1,t.msfocus2);
	htrAddScriptInit_va(s, "    page.dtcolor1 = '%s';\n    page.dtcolor2 = '%s';\n",t.dtfocus1,t.dtfocus2);
	htrAddScriptInit(s, "    document.LSParent = null;\n");

	/** Start cursor blinking **/
	htrAddScriptInit(s, "    pg_togglecursor();\n");

	htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");

	htrAddEventHandler(s, "document", "KEYDOWN", "pg",
		"    k = e.which;\n"
		"    if (k > 65280) k -= 65280;\n"
		"    if (k >= 128) k -= 128;\n"
		"    if (k == pg_lastkey) return false;\n"
		"    pg_lastkey = k;\n"
		"    /*pg_togglecursor();*/\n"
		"    if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);\n"
		"    pg_keytimeoutid = setTimeout(pg_keytimeout, 200);\n"
		"    return pg_keyhandler(k, e.modifiers, e);\n");

	htrAddEventHandler(s, "document", "KEYUP", "pg",
		"    k = e.which;\n"
		"    if (k > 65280) k -= 65280;\n"
		"    if (k >= 128) k -= 128;\n"
		"    if (k == pg_lastkey) pg_lastkey = -1;\n"
		"    if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);\n"
		"    pg_keytimeoutid = null;\n");

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+1, parentname, "document");
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	htrAddScriptInit(s,
		"    setTimeout(pg_mvpginpt, 1, document.layers.pginpt);\n"
		"    document.layers.pginpt.moveTo(window.innerWidth-2, 20);\n"
		"    document.layers.pginpt.visibility = 'inherit';\n");
	htrAddScriptInit(s,"    document.layers.pginpt.document.tmpform.x.focus();\n");

    return 0;
    }

#include "htdrv_page_moz.c"

int
htpageRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    if (s->WidgetSet == HTR_UA_NETSCAPE_47)
	return htpageRenderNtsp47xDefault(s,w_obj,z,parentname,parentobj);
    else
	return htpageRenderMozDefault(s,w_obj,z,parentname,parentobj);
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
	drv->Verify = htpageVerify;
	htrAddSupport(drv, HTR_UA_NETSCAPE_47);
	htrAddSupport(drv, HTR_UA_MOZILLA);

	/** Actions **/
	htrAddAction(drv, "LoadPage");
	htrAddParam(drv, "LoadPage", "Source", DATA_T_STRING);
	
	/** Register. **/
	htrRegisterDriver(drv);

    return 0;
    }
