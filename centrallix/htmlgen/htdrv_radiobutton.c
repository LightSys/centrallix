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
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_radiobutton.c                                     */
/* Author:      Nathan Ehresman (NRE)                                   */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for a radiobutton panel and          */
/*              radio button.                                           */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_radiobutton.c,v 1.17 2002/07/30 16:09:05 pfinley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_radiobutton.c,v $

    $Log: htdrv_radiobutton.c,v $
    Revision 1.17  2002/07/30 16:09:05  pfinley
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

    Revision 1.16  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.15  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.14  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.13  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.12  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.11  2002/06/19 14:57:52  lkehresman
    Fixed the bug that broke radio buttons.  It was just a simple typo.

    Revision 1.10  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.9  2002/06/06 17:12:22  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.8  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.7  2002/03/09 07:46:04  lkehresman
    Brought radiobutton widget up to date:
      * enable(), disable(), readonly()
      * FocusNotify() callback issued
      * Added disabled images for radio buttons

    Revision 1.6  2002/03/05 01:55:09  lkehresman
    Added "clearvalue" method to form widgets

    Revision 1.5  2002/03/05 00:46:34  jorupp
    * Fix a problem in Luke's radiobutton fix
    * Add the corresponding checks in the form

    Revision 1.4  2002/03/05 00:31:40  lkehresman
    Implemented DataNotify form method in the radiobutton and checkbox widgets

    Revision 1.3  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.2  2002/02/23 19:35:28  lkehresman
    * Radio button widget is now forms aware.
    * Fixes a couple of oddities in the checkbox.
    * Fixed some formatting issues in the form.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:57  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int   idcnt;
} HTRB;


/** htrbVerify - not written yet.  **/
int htrbVerify() {
   return 0;
}


/** htrbRender - generate the HTML code for the page.  **/
int htrbRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char* ptr;
   char name[64];
   char title[64];
   char sbuf2[200];
   char* nptr;
   //char bigbuf[4096];
   char textcolor[32];
   char main_bgcolor[32];
   char main_background[128];
   char outline_bg[64];
   pObject sub_w_obj;
   pObject radiobutton_obj;
   pObjQuery qy;
   int x=-1,y=-1,w,h;
   int id;
   char fieldname[32];

   /** Get an id for this. **/
   id = (HTRB.idcnt++);

   /** Get x,y,w,h of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'width' property");
      return -1;
   }
   if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'height' property");
      return -1;
   }

   /** Background color/image? **/
   if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
      strncpy(main_bgcolor,ptr,31);
   else 
      strcpy(main_bgcolor,"");

   if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
      strncpy(main_background,ptr,127);
   else
      strcpy(main_background,"");

   /** Text color? **/
   if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
      snprintf(textcolor,32,"%s",ptr);
   else
      strcpy(textcolor,"black");

   /** Outline color? **/
   if (objGetAttrValue(w_obj,"outlinecolor",POD(&ptr)) == 0)
      snprintf(outline_bg,64,"%s",ptr);
   else
      strcpy(outline_bg,"black");

   /** Get name **/
   if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,63);
   name[63]=0;

   /** Get title **/
   if (objGetAttrValue(w_obj,"title",POD(&ptr)) != 0) return -1;
   memccpy(title,ptr,0,63);
   title[63] = 0;

   /** Get fieldname **/
   if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
      {
      strncpy(fieldname,ptr,30);
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dparentpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,x,y,w,h,z,w,h);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dborderpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,3,12,w-(2*3),h-(12+3),z+1,w-(2*3),h-(12+3));
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dcoverpane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,1,1,w-(2*3 +2),h-(12+3 +2),z+2,w-(2*3 +2),h-(12+3 +2));
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dtitlepane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",
           id,10,1,w/2,17,z+3);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelbuttonsetpane   { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelbuttonunsetpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelvaluepane       { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanellabelpane       { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           27,2,w-(2*3 +2+27+1),24,z+2,w-(2*3 +2+27+1),24);
   
   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);
   htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
   htrAddScriptGlobal(s, "radiobutton", "null", 0);

   /*
      Now lets loop through and create a style sheet for each optionpane on the
      radiobuttonpanel
   */   
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((radiobutton_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(radiobutton_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%doption%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx, %dpx); }\n",
                    id,i,7,10+((i-1)*25)+3,w-(2*3 +2+7),25,z+2,w-(2*3 +2+7),25);
            i++;
         }
         objClose(radiobutton_obj);
      }
   }
   objQueryClose(qy);
   


   /** Script initialization call. **/
   if (strlen(main_bgcolor) > 0) {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(
            %s.layers.radiobuttonpanel%dparentpane,\"%s\",1,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,
	    \"%s\",\"%s\");\n", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_bgcolor, outline_bg);
   } else if (strlen(main_background) > 0) {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(
            %s.layers.radiobuttonpanel%dparentpane,\"%s\",2,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,
	    \"%s\",\"%s\");\n", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_background, outline_bg);
   } else {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(%s.layers.radiobuttonpanel%dparentpane,\"%s\",0,0,0,0,0,0);\n", nptr, parentname, id,fieldname);
   }

   htrAddEventHandler(s, "document", "MOUSEUP", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) {\n"
      "          if(ly.optionPane) {\n"
      "             if (ly.mainlayer.form) ly.mainlayer.form.FocusNotify(ly.mainlayer);\n"
      "             radiobutton_toggle(ly);\n"
      "          }\n"
      "          cn_activate(ly.mainlayer, 'Click');\n"
      "          cn_activate(ly.mainlayer, 'MouseUp');\n"
      "      }\n"
      "   }\n");

   htrAddEventHandler(s, "document", "MOUSEDOWN", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) cn_activate(ly.mainlayer, 'MouseDown');\n"
      "   }\n");
   
   htrAddEventHandler(s, "document", "MOUSEOVER", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled && !util_cur_mainlayer) {\n"
      "          cn_activate(ly.mainlayer, 'MouseOver');\n"
      "          util_cur_mainlayer = ly.mainlayer;\n"
      "      }\n"
      "   }\n");

//   htrAddEventHandler(s, "document", "MOUSEOUT", "radiobutton", "\n"
//      "   }\n");
   
   htrAddEventHandler(s, "document", "MOUSEMOVE", "radiobutton", "\n"
      "   if (util_cur_mainlayer && ly.kind != 'radiobutton') {\n"						// 
      "      if (util_cur_mainlayer.mainlayer.enabled) cn_activate(util_cur_mainlayer.mainlayer, 'MouseOut');\n"	// This is MouseOut Detection!
      "      util_cur_mainlayer = null;\n"									// 
      "   }\n"												// 
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) cn_activate(ly.mainlayer, 'MouseMove');\n"
      "   }\n");
   
   /*
      Now lets loop through and add each radiobutton
   */
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((sub_w_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(sub_w_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            if (objGetAttrValue(sub_w_obj,"selected",POD(&ptr)) != 0)
               strcpy(sbuf2,"false");
            else {
               memccpy(sbuf2,ptr,0,199);
	       sbuf2[199] = 0;
	    }

            htrAddScriptInit_va(s,"    add_radiobutton(%s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane.layers.radiobuttonpanel%doption%dpane, %s.layers.radiobuttonpanel%dparentpane, %s, %s);\n",
               parentname, id, id, id, id, i, parentname, id, sbuf2, nptr);
            i++;
         }
	 else {
	    htrRenderWidget(s, sub_w_obj, z+1, parentname, nptr);
	 }
         objClose(sub_w_obj);
      }
   }
   objQueryClose(qy);

   /*
      Do the HTML layers
   */
   htrAddBodyItem_va(s,"   <DIV ID=\"radiobuttonpanel%dparentpane\">\n", id);
   htrAddBodyItem_va(s,"      <DIV ID=\"radiobuttonpanel%dborderpane\">\n", id);
   htrAddBodyItem_va(s,"         <DIV ID=\"radiobuttonpanel%dcoverpane\">\n", id);

   /* Loop through each radio button and do the option pane and sub layers */
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((radiobutton_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(radiobutton_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            htrAddBodyItem_va(s,"            <DIV ID=\"radiobuttonpanel%doption%dpane\">\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelbuttonsetpane\"><IMG SRC=\"/sys/images/radiobutton_set.gif\"></DIV>\n");
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelbuttonunsetpane\"><IMG SRC=\"/sys/images/radiobutton_unset.gif\"></DIV>\n");

	    
            objGetAttrValue(radiobutton_obj,"label",POD(&ptr));
            memccpy(sbuf2,ptr,0,199);
	    sbuf2[199]=0;
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanellabelpane\" NOWRAP><FONT COLOR=\"%s\">%s</FONT></DIV>\n", textcolor, sbuf2);

	    /* use label (from above) as default value if no value given */
	    if(objGetAttrValue(radiobutton_obj,"value",POD(&ptr))==0)
		{
		memccpy(sbuf2,ptr,0,199);
		sbuf2[199] = 0;
		}

            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelvaluepane\" VISIBILITY=\"hidden\"><A NAME=\"%s\"></A></DIV>\n", sbuf2);
            htrAddBodyItem_va(s,"            </DIV>\n");
            i++;
         }
         objClose(radiobutton_obj);
      }
   }
   objQueryClose(qy);
   
   htrAddBodyItem_va(s,"         </DIV>\n");
   htrAddBodyItem_va(s,"      </DIV>\n");
   htrAddBodyItem_va(s,"      <DIV ID=\"radiobuttonpanel%dtitlepane\"><TABLE><TR><TD NOWRAP><FONT COLOR=\"%s\">%s</FONT></TD></TR></TABLE></DIV>\n", id, textcolor, title);
   htrAddBodyItem_va(s,"   </DIV>\n");

   return 0;
}


/** htrbInitialize - register with the ht_render module.  **/
int htrbInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML RadioButton Driver");
   strcpy(drv->WidgetName,"radiobuttonpanel");
   drv->Render = htrbRender;
   drv->Verify = htrbVerify;
   strcpy(drv->Target, "Netscape47x:default");

#if 00
   /** Add the 'load page' action **/
   action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
   strcpy(action->Name,"LoadPage");
   xaInit(&action->Parameters,16);
   param = (pHtParam)nmSysMalloc(sizeof(HtParam));
   strcpy(param->ParamName,"Source");
   param->DataType = DATA_T_STRING;
   xaAddItem(&action->Parameters,(void*)param);
   xaAddItem(&drv->Actions,(void*)action);
#endif

   /** Events **/ 
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");

   /** Register. **/
   htrRegisterDriver(drv);

   HTRB.idcnt = 0;

   return 0;
}
