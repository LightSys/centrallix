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

    $Id: htdrv_radiobutton.c,v 1.9 2002/06/06 17:12:22 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_radiobutton.c,v $

    $Log: htdrv_radiobutton.c,v $
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
   char sbuf[1024];
   char sbuf2[200];
   char* nptr;
   //char bigbuf[4096];
   char textcolor[32];
   char main_bgcolor[32];
   char main_background[128];
   char outline_bg[64];
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

   

   /** Ok, write the style header items. **/
   snprintf(sbuf,1024,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanel%dparentpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,x,y,w,h,z,w,h);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanel%dborderpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,3,12,w-(2*3),h-(12+3),z+1,w-(2*3),h-(12+3));
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanel%dcoverpane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,1,1,w-(2*3 +2),h-(12+3 +2),z+2,w-(2*3 +2),h-(12+3 +2));
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanel%dtitlepane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",
           id,10,1,w/2,17,z+3);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanelbuttonsetpane   { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanelbuttonunsetpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanelvaluepane       { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,1024,"\t#radiobuttonpanellabelpane       { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           27,2,w-(2*3 +2+27+1),24,z+2,w-(2*3 +2+27+1),24);
   htrAddHeaderItem(s,sbuf);
   
   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);
   htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

   /*
      Now lets loop through and create a style sheet for each optionpane on the radiobuttonpanel
   */   
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((radiobutton_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(radiobutton_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            snprintf(sbuf,1024,"\t#radiobuttonpanel%doption%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx, %dpx); }\n",
                    id,i,7,10+((i-1)*25)+3,w-(2*3 +2+7),25,z+2,w-(2*3 +2+7),25);
            htrAddHeaderItem(s,sbuf);
            i++;
         }
         objClose(radiobutton_obj);
      }
   }
   objQueryClose(qy);
   
   snprintf(sbuf, 1024, "    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   htrAddScriptFunction(s, "rb_getvalue", "\n"
      "   function rb_getvalue() {\n"
      "       if (this.selectedOption)\n"	/* return "" if nothing is selected */
      "           {\n"
      "           return this.selectedOption.layers.radiobuttonpanelvaluepane.document.anchors[0].name;\n"
      "           }\n"
      "       else\n"
      "           {\n"
      "           return \"\";\n"
      "           }\n"
      "   }\n",0);

   /***
    ***  Luke (03/04/02) -
    ***  The behavior of this is in direct combination with the clearvalue function.
    ***  if set_value() gets called with a value that _is_ in the list of values, then
    ***  obviously that button gets marked.  However, if the called parameter is not
    ***  in the list, an alert message pops up.  SEE ALSO:  Note on clearvalue()
    ***/
   htrAddScriptFunction(s, "rb_setvalue", "\n"
      "   function rb_setvalue(v) {\n"
      "       optsLayerArray = this.layers[0].document.layers[0].document.layers;\n"
      "       for (var i=0; i < optsLayerArray.length; i++) {\n"
      "           if (optsLayerArray[i].layers.radiobuttonpanelvaluepane.document.anchors[0].name == v) {\n"
      "               radiobutton_toggle(optsLayerArray[i]);\n"
      "               return;\n"
      "           }\n"
      "       }\n"
      "       alert('Warning: \"'+v+'\" is not in the radio button list.');\n"
      "   }\n",0);

   /** Clear function **/
   /*  Luke (03/04/02)  This unchecks ALL radio buttons.  As such, nothing is selected. */
   htrAddScriptFunction(s, "rb_clearvalue", "\n"
      "   function rb_clearvalue() {\n"
      "      if (this.selectedOption) {\n"
      "         this.selectedOption.unsetPane.visibility = 'inherit';\n"
      "         this.selectedOption.setPane.visibility = 'hidden';\n"
      "         this.selectedOption = null;\n"
      "      }\n"
      "   }\n", 0);

   /** reset value function **/
   htrAddScriptFunction(s, "rb_resetvalue", "\n"
      "   function rb_resetvalue() {\n"
      "      if (this.selectedOption != this.defaultSelectedOption) {\n"
      "         radiobutton_toggle(this.defaultSelectedOption);\n"
      "      }\n"
      "   }\n", 0);

   /** enable function **/
   htrAddScriptFunction(s, "rb_enable", "\n"
      "   function rb_enable() {\n"
      "      this.enabled = true;\n"
      "      for (i=0;i<this.buttonList.length;i++) {\n"
      "         this.buttonList[i].enabled = this.enabled;;\n"
      "         this.buttonList[i].setPane.enabled = this.enabled;\n"
      "         this.buttonList[i].unsetPane.enabled = this.enabled;\n"
      "         this.buttonList[i].layers.radiobuttonpanellabelpane.enabled = this.enabled;\n"
      "         this.buttonList[i].setPane.document.images[0].src = '/sys/images/radiobutton_set.gif';\n"
      "         this.buttonList[i].unsetPane.document.images[0].src = '/sys/images/radiobutton_unset.gif';\n"
      "      }\n"
      "   }\n", 0);
   
   /** readonly function **/
   htrAddScriptFunction(s, "rb_readonly", "\n"
      "   function rb_readonly() {\n"
      "      this.enabled = false;\n"
      "      for (i=0;i<this.buttonList.length;i++) {\n"
      "         this.buttonList[i].enabled = this.enabled;\n"
      "         this.buttonList[i].setPane.enabled = this.enabled;\n"
      "         this.buttonList[i].unsetPane.enabled = this.enabled;\n"
      "         this.buttonList[i].layers.radiobuttonpanellabelpane.enabled = this.enabled;\n"
      "      }\n"
      "   }\n", 0);
   
   /** disable function **/
   htrAddScriptFunction(s, "rb_disable", "\n"
      "   function rb_disable() {\n"
      "      this.enabled = false;\n"
      "      for (i=0;i<this.buttonList.length;i++) {\n"
      "         this.buttonList[i].enabled = this.enabled;\n"
      "         this.buttonList[i].setPane.enabled = this.enabled;\n"
      "         this.buttonList[i].unsetPane.enabled = this.enabled;\n"
      "         this.buttonList[i].layers.radiobuttonpanellabelpane.enabled = this.enabled;\n"
      "         this.buttonList[i].setPane.document.images[0].src = '/sys/images/radiobutton_set_dis.gif';\n"
      "         this.buttonList[i].unsetPane.document.images[0].src = '/sys/images/radiobutton_unset_dis.gif';\n"
      "      }\n"
      "   }\n", 0);

   htrAddScriptFunction(s, "add_radiobutton", "\n"
      "   function add_radiobutton(optionPane, parentPane, selected) {\n"
      "      optionPane.kind = 'radiobutton';\n"
      "      optionPane.enabled = true;\n"
      "      optionPane.parentPane = parentPane;\n"
      "      optionPane.optionPane = optionPane;\n"
      "      optionPane.setPane = optionPane.layers.radiobuttonpanelbuttonsetpane;\n"
      "      optionPane.unsetPane = optionPane.layers.radiobuttonpanelbuttonunsetpane;\n"
      "      optionPane.document.layer = optionPane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.kind = 'radiobutton';\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.optionPane = optionPane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.enabled = optionPane.enabled;\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonsetpane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].kind = 'radiobutton';\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonsetpane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonsetpane.document.images[0].enabled = optionPane.enabled;\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.kind = 'radiobutton';\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.optionPane = optionPane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.enabled = optionPane.enabled;\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.document.layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].kind = 'radiobutton';\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].layer = optionPane.layers.radiobuttonpanelbuttonunsetpane;\n"
      "      optionPane.layers.radiobuttonpanelbuttonunsetpane.document.images[0].enabled = optionPane.enabled;\n"
      "      optionPane.layers.radiobuttonpanellabelpane.kind = 'radiobutton';\n"
      "      optionPane.layers.radiobuttonpanellabelpane.optionPane = optionPane;\n"
      "      optionPane.layers.radiobuttonpanellabelpane.enabled = optionPane.enabled;\n"
      "      optionPane.layers.radiobuttonpanellabelpane.document.layer = optionPane.layers.radiobuttonpanellabelpane;\n"
      "      parentPane.buttonList.push(optionPane);\n"
      "      if (selected) {\n"
      "         optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'inherit';\n"
      "         optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'hidden';\n"
      "         parentPane.selectedOption = optionPane;\n"
      "         parentPane.defaultSelectedOption = optionPane;\n"
      "      } else {\n"
      "         optionPane.layers.radiobuttonpanelbuttonsetpane.visibility = 'hidden';\n"
      "         optionPane.layers.radiobuttonpanelbuttonunsetpane.visibility = 'inherit';\n"
      "      }\n"
      "   }\n", 0);

   htrAddScriptFunction(s, "radiobuttonpanel_init",
         "   function radiobuttonpanel_init(parentPane,fieldname,flag,borderpane,coverpane,titlepane,main_bg,outline_bg) {\n"
         "      if(flag==1)\n"
         "          {\n"
         "          parentPane.bgColor=main_bg;\n"
         "          borderpane.bgColor=outline_bg;\n"
         "          coverpane.bgColor=main_bg;\n"
         "          titlepane.bgColor=main_bg;\n"
         "          }\n"
         "      if(flag==2)\n"
         "          {\n"
         "          parentPane.background.src=main_bg;\n"
         "          borderpane.background.src=outline_bg;\n"
         "          coverpane.background.src=main_bg;\n"
         "          titlepane.background.src=main_bg;\n"
         "          }\n"
	 "      parentPane.buttonList = new Array();\n"
	 "      parentPane.setvalue = rb_setvalue;\n"
	 "      parentPane.getvalue = rb_getvalue;\n"
	 "      parentPane.clearvalue = rb_clearvalue;\n"
	 "      parentPane.resetvalue = rb_resetvalue;\n"
	 "      parentPane.enable = rb_enable;\n"
	 "      parentPane.disable = rb_disable;\n"
	 "      parentPane.readonly = rb_readonly;\n"
	 "      parentPane.kind = 'radiobutton';\n"
	 "      parentPane.fieldname = fieldname;\n"
	 "      parentPane.form = fm_current;\n"
	 "      if (fm_current) fm_current.Register(parentPane);\n"
	 "      return parentPane;\n"
         "   }\n",0);

   /** Script initialization call. **/
   if (strlen(main_bgcolor) > 0) {
      snprintf(sbuf,1024,"    %s = radiobuttonpanel_init(
            %s.layers.radiobuttonpanel%dparentpane,\"%s\",1,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,
	    \"%s\",\"%s\");", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_bgcolor, outline_bg);
   } else if (strlen(main_background) > 0) {
      snprintf(sbuf,1024,"    %s = radiobuttonpanel_init(
            %s.layers.radiobuttonpanel%dparentpane,\"%s\",2,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,
            %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,
	    \"%s\",\"%s\");", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_background, outline_bg);
   } else {
      snprintf(sbuf,1024,"    %s = radiobuttonpanel_init(%s.layers.radiobuttonpanel%dparentpane,\"%s\",0,0,0,0,0,0);\n", nptr, parentname, id,fieldname);
   }
   htrAddScriptInit(s, sbuf);

   htrAddScriptFunction(s, "radiobutton_toggle", "\n"
      "   function radiobutton_toggle(layer) {\n"
      "      if(!layer) return;\n"
      "      if(layer.optionPane.parentPane.form)\n"
      "          layer.optionPane.parentPane.form.DataNotify(layer.optionPane.parentPane);\n"
      "      if (layer.optionPane.parentPane.selectedOption) {\n"
      "          layer.optionPane.parentPane.selectedOption.unsetPane.visibility = 'inherit';\n"
      "          layer.optionPane.parentPane.selectedOption.setPane.visibility = 'hidden';\n"
      "      }\n"
      "      layer.optionPane.setPane.visibility = 'inherit';\n"
      "      layer.optionPane.unsetPane.visibility = 'hidden';\n"
      "      layer.optionPane.parentPane.selectedOption = layer.optionPane;\n"
      "   }\n", 0);

   htrAddEventHandler(s, "document", "MOUSEUP", "radiobutton", "\n"
      "   targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
      "   if (targetLayer != null && targetLayer.kind == 'radiobutton') {\n"
      "      if(layer.optionPane.parentPane.form)\n"
      "          targetLayer.optionPane.parentPane.form.FocusNotify(targetLayer.optionPane.parentPane);\n"
      "      if (targetLayer.enabled) {\n"
      "         radiobutton_toggle(targetLayer);\n"
      "      }\n"
      "   }\n");


   /*
      Now lets loop through and add each radiobutton
   */
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((radiobutton_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(radiobutton_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            if (objGetAttrValue(radiobutton_obj,"selected",POD(&ptr)) != 0)
               strcpy(sbuf2,"false");
            else {
               memccpy(sbuf2,ptr,0,199);
	       sbuf2[199] = 0;
	    }

            snprintf(sbuf,1024,"    add_radiobutton(%s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane.layers.radiobuttonpanel%doption%dpane, %s.layers.radiobuttonpanel%dparentpane, %s);\n",
               parentname, id, id, id, id, i, parentname, id, sbuf2);
            htrAddScriptInit(s, sbuf);
            i++;
         }
         objClose(radiobutton_obj);
      }
   }
   objQueryClose(qy);

   /*
      Do the HTML layers
   */
   snprintf(sbuf,1024,"   <DIV ID=\"radiobuttonpanel%dparentpane\">\n", id);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,1024,"      <DIV ID=\"radiobuttonpanel%dborderpane\">\n", id);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,1024,"         <DIV ID=\"radiobuttonpanel%dcoverpane\">\n", id);
   htrAddBodyItem(s, sbuf);

   /* Loop through each radio button and do the option pane and sub layers */
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      int i = 1;
      while((radiobutton_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(radiobutton_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/radiobutton")) {
            snprintf(sbuf,1024,"            <DIV ID=\"radiobuttonpanel%doption%dpane\">\n", id, i);
            htrAddBodyItem(s, sbuf);
            snprintf(sbuf,1024,"               <DIV ID=\"radiobuttonpanelbuttonsetpane\"><IMG SRC=\"/sys/images/radiobutton_set.gif\"></DIV>\n");
            htrAddBodyItem(s, sbuf);
            snprintf(sbuf,1024,"               <DIV ID=\"radiobuttonpanelbuttonunsetpane\"><IMG SRC=\"/sys/images/radiobutton_unset.gif\"></DIV>\n");
            htrAddBodyItem(s, sbuf);

	    
            objGetAttrValue(radiobutton_obj,"label",POD(&ptr));
            memccpy(sbuf2,ptr,0,199);
	    sbuf2[199]=0;
            snprintf(sbuf,1024,"               <DIV ID=\"radiobuttonpanellabelpane\" NOWRAP><FONT COLOR=\"%s\">%s</FONT></DIV>\n", textcolor, sbuf2);
	    htrAddBodyItem(s, sbuf);

	    /* use label (from above) as default value if no value given */
	    if(objGetAttrValue(radiobutton_obj,"value",POD(&ptr))==0)
		{
		memccpy(sbuf2,ptr,0,199);
		sbuf2[199] = 0;
		}

            snprintf(sbuf,1024,"               <DIV ID=\"radiobuttonpanelvaluepane\" VISIBILITY=\"hidden\"><A NAME=\"%s\"></A></DIV>\n", sbuf2);
            htrAddBodyItem(s, sbuf);
            
            snprintf(sbuf,1024,"            </DIV>\n");
            htrAddBodyItem(s, sbuf);
            i++;
         }
         objClose(radiobutton_obj);
      }
   }
   objQueryClose(qy);
   
   snprintf(sbuf,1024,"         </DIV>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,1024,"      </DIV>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,1024,"      <DIV ID=\"radiobuttonpanel%dtitlepane\"><TABLE><TR><TD NOWRAP><FONT COLOR=\"%s\">%s</FONT></TD></TR></TABLE></DIV>\n", id, textcolor, title);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,1024,"   </DIV>\n");
   htrAddBodyItem(s, sbuf);

   return 0;
}


/** htrbInitialize - register with the ht_render module.  **/
int htrbInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML RadioButton Driver");
   strcpy(drv->WidgetName,"radiobuttonpanel");
   drv->Render = htrbRender;
   drv->Verify = htrbVerify;
   xaInit(&(drv->PosParams),16);
   xaInit(&(drv->Properties),16);
   xaInit(&(drv->Events),16);
   xaInit(&(drv->Actions),16);

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

   /** Register. **/
   htrRegisterDriver(drv);

   HTRB.idcnt = 0;

   return 0;
}
