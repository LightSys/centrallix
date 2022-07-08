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


/** globals **/
static struct {
   int   idcnt;
} HTRB;

int htrbSetup(pHtSession s)
	{
	htrAddStylesheetItem_va(s,"\t.posAbs\t{ POSITION:absolute; }\n");
	htrAddStylesheetItem_va(s,"\t.visInh\t{ VISIBILITY:inherit; }\n");
	htrAddStylesheetItem_va(s,"\t.visHid\t{ VISIBILITY:hidden; }\n");
	return 0;	
	}

/** htrbRender - generate the HTML code for the page.  **/
int htrbRender(pHtSession s, pWgtrNode tree, int z) {
   char* ptr;
   char name[64];
   char title[64];
   char sbuf2[200];
   //char bigbuf[4096];
   char textcolor[32];
   char main_background[128];
   char outline_background[128];
   char form[64];
   pWgtrNode radiobutton_obj, sub_tree;
   int x=-1,y=-1,w,h;
   int top_offset;
   int cover_height, cover_width;
   int item_spacing;
   int id, i, j;
   int is_selected;
   int rb_cnt;
   int cover_margin;
   char fieldname[32];
   char value[64];
   char label[64];

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTRB","Netscape 4.x or W3C DOM support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTRB.idcnt++);

   /** Get x,y,w,h of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'width' property");
      return -1;
   }
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'height' property");
      return -1;
   }

   /** Background color/image? **/
   htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_background,sizeof(main_background));

   /** Text color? **/
   if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
      strtcpy(textcolor,ptr,sizeof(textcolor));
   else
      strcpy(textcolor,"black");

   /** Outline color? **/
   htrGetBackground(tree,"outline",!s->Capabilities.Dom0NS,outline_background,sizeof(outline_background));

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(name,ptr,sizeof(name));

   /** Get title **/
   if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(title,ptr,sizeof(title));

   /** User requesting expression for selected tab? **/
   htrCheckAddExpression(s, tree, name, "value");

   /** User requesting expression for selected tab using integer index value? **/
   htrCheckAddExpression(s, tree, name, "value_index");

   /** Get fieldname **/
   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      {
      strtcpy(fieldname,ptr,sizeof(fieldname));
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
      strtcpy(form,ptr,sizeof(form));
   else
      form[0]='\0';

   htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

   /** Ok, write the style header items. **/
   top_offset = s->ClientInfo->ParagraphHeight*3/4+1;
   cover_height = h-(top_offset+3+2);
   cover_width = w-(2*3 +2);
   htrAddStylesheetItem_va(s,"\t#rb%POSparent    { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,x,y,w,h,z,w,h);
   htrAddStylesheetItem_va(s,"\t#rb%POSborder    { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,3,top_offset,w-(2*3),h-(top_offset+3),z+1,w-(2*3),h-(top_offset+3));
   htrAddStylesheetItem_va(s,"\t#rb%POScover     { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,1,1,cover_width,cover_height,z+2,cover_width,cover_height);
   htrAddStylesheetItem_va(s,"\t#rb%POStitle     { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",
           id,10,1,w/2,s->ClientInfo->ParagraphHeight,z+3);
   
   htrAddScriptGlobal(s, "radiobutton", "null", 0);

   /** DOM linkages **/
   htrAddWgtrObjLinkage_va(s, tree, "rb%POSparent",id);
   htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(htr_subel(_obj,\"rb%POSborder\"),\"rb%POScover\")",id,id);

    /** Loop through each radiobutton and flag it NOOBJECT **/
    rb_cnt = 0;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
	radiobutton_obj->RenderFlags |= HT_WGTF_NOOBJECT;
	wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	if (!strcmp(ptr,"widget/radiobutton"))
	    {
	    rb_cnt++;
	    }
	}
   /*
      Now lets loop through and create a style sheet for each optionpane on the
      radiobuttonpanel
   */   
    item_spacing = 12 + s->ClientInfo->ParagraphHeight;
    cover_margin = 10;
    if (item_spacing*rb_cnt+2*cover_margin > cover_height)
	item_spacing = (cover_height-2*cover_margin)/rb_cnt;
    if (item_spacing*rb_cnt+2*cover_margin > cover_height)
	cover_margin = (cover_height-(item_spacing*rb_cnt))/2;
    if (cover_margin < 2) cover_margin = 2;
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
	wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	if (!strcmp(ptr,"widget/radiobutton"))
	    {
	    htrAddStylesheetItem_va(s,"\t#rb%POSoption%POS { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px, %POSpx, %POSpx, 0px); }\n",
		    id,i,7,cover_margin+((i-1)*item_spacing)+3,cover_width-7,item_spacing,z+2,cover_width-7,item_spacing);
	    i++;
	    }
	}

   /** Script initialization call. **/
   if (strlen(main_background) > 0) {
      htrAddScriptInit_va(s,
        "    var rb = wgtrGetNodeRef(ns, \"%STR&SYM\");\n"
        "    radiobuttonpanel_init({\n"
        "	parentPane:rb, fieldname:\"%STR&JSSTR\",\n"
        "	borderPane:htr_subel(rb,\"rb%POSborder\"),\n"
        "	coverPane:htr_subel(htr_subel(rb,\"rb%POSborder\"),\"rb%POScover\"),\n"
        "	titlePane:htr_subel(rb,\"rb%POStitle\"),\n"
	"	mainBackground:\"%STR&JSSTR\", outlineBackground:\"%STR&JSSTR\", form:\"%STR&JSSTR\"});\n",
	    name, fieldname, id, id,id, id, main_background, outline_background, form);
   } else {
      htrAddScriptInit_va(s,"    radiobuttonpanel_init({parentPane:wgtrGetNodeRef(ns,\"%STR&SYM\"), fieldname:\"%STR&JSSTR\", borderPane:0, coverPane:0, titlePane:0, mainBackground:0, outlineBackground:0, form:\"%STR&JSSTR\"});\n", name, fieldname, form);
   }

   htrAddEventHandlerFunction(s, "document", "MOUSEUP", "radiobutton", "radiobutton_mouseup");
   htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "radiobutton", "radiobutton_mousedown");
   htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "radiobutton", "radiobutton_mouseover");
   htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "radiobutton", "radiobutton_mousemove");

   /*
      Now lets loop through and add each radiobutton
   */
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	sub_tree = xaGetItem(&(tree->Children), j);
	wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
        if (!strcmp(ptr,"widget/radiobutton")) 
	    {
	    if (wgtrGetPropertyValue(sub_tree,"value",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(value, ptr, sizeof(value));
	    else
		value[0] = '\0';
	    if (wgtrGetPropertyValue(sub_tree,"label",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(label, ptr, sizeof(label));
	    else
		label[0] = '\0';
	    is_selected = htrGetBoolean(sub_tree, "selected", 0);
	    if (is_selected < 0) is_selected = 0;
	    htrAddWgtrObjLinkage_va(s,sub_tree,"rb%POSoption%POS",id,i);
	    wgtrGetPropertyValue(sub_tree,"name",DATA_T_STRING,POD(&ptr));
            htrAddScriptInit_va(s,
		    "    var rbitem = wgtrGetNodeRef('%STR&SYM', '%STR&SYM');\n"
		    "    add_radiobutton(rbitem, {selected:%INT, buttonset:htr_subel(rbitem, \"rb%POSbuttonset%POS\"), buttonunset:htr_subel(rbitem, \"rb%POSbuttonunset%POS\"), value:htr_subel(rbitem, \"rb%POSvalue%POS\"), label:htr_subel(rbitem, \"rb%POSlabel%POS\"), valuestr:\"%STR&JSSTR\", labelstr:\"%STR&JSSTR\"});\n", 
		    wgtrGetNamespace(sub_tree), ptr,
		    is_selected,
		    id, i, id, i,
		    id, i, id, i,
		    value, label);
            i++;
	    }
	 else 
	    {
	    htrRenderWidget(s, sub_tree, z+1);
	    }
	}

   /*
      Do the HTML layers
   */
   htrAddBodyItem_va(s,"   <DIV ID=\"rb%POSparent\" class=\"visInh posAbs\">\n", id);
   htrAddBodyItem_va(s,"      <DIV ID=\"rb%POSborder\" class=\"visInh posAbs\">\n", id);
   htrAddBodyItem_va(s,"         <DIV ID=\"rb%POScover\" class=\"visInh posAbs\">\n", id);

   /* Loop through each radio button and do the option pane and sub layers */
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
        wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
        if (!strcmp(ptr,"widget/radiobutton")) 
	    {
	    /** CSS layers **/
	    htrAddStylesheetItem_va(s,"\t#rb%POSbuttonset%POS\t{ LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",id,i,5,2+(s->ClientInfo->ParagraphHeight-12)/2,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSbuttonunset%POS { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",id,i,5,2+(s->ClientInfo->ParagraphHeight-12)/2,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSvalue%POS\t{ LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",id,i,5,5,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSlabel%POS\t{ LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",id,i,27,2,cover_width-(27+1),item_spacing-1,z+2,cover_width-(27+1),item_spacing-1);

	    /** Body layers **/
            htrAddBodyItem_va(s,"            <DIV ID=\"rb%POSoption%POS\" class=\"posAbs visInh\">\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSbuttonset%POS\" class=\"posAbs visHid\"><IMG SRC=\"/sys/images/radiobutton_set.gif\"></DIV>\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSbuttonunset%POS\" class=\"posAbs visInh\"><IMG SRC=\"/sys/images/radiobutton_unset.gif\"></DIV>\n", id, i);
 
            wgtrGetPropertyValue(radiobutton_obj,"label",DATA_T_STRING,POD(&ptr));
	    strtcpy(sbuf2,ptr,sizeof(sbuf2));
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSlabel%POS\" class=\"posAbs visInh\" NOWRAP><FONT COLOR=\"%STR&HTE\">%STR&HTE</FONT></DIV>\n", 
		    id, i, textcolor, sbuf2);

	    /* use label (from above) as default value if no value given */
	    if(wgtrGetPropertyValue(radiobutton_obj,"value",DATA_T_STRING,POD(&ptr))==0)
		{
		strtcpy(sbuf2,ptr,sizeof(sbuf2));
		}

            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSvalue%POS\" class=\"posAbs visHid\"><A HREF=\".\">%STR&HTE</A></DIV>\n",
		    id, i, sbuf2);
            htrAddBodyItem(s,   "            </DIV>\n");
            i++;
	    }
	}
   
   htrAddBodyItem(s,   "         </DIV>\n");
   htrAddBodyItem(s,   "      </DIV>\n");
   htrAddBodyItem_va(s,"      <DIV ID=\"rb%POStitle\" class=\"posAbs visInh\"><TABLE><TR><TD NOWRAP><FONT COLOR=\"%STR&HTE\">%STR&HTE</FONT></TD></TR></TABLE></DIV>\n", id, textcolor, title);
   htrAddBodyItem(s,   "   </DIV>\n");

   return 0;
}


/** htrbInitialize - register with the ht_render module.  **/
int htrbInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML RadioButton Driver");
   strcpy(drv->WidgetName,"radiobuttonpanel");
   drv->Render = htrbRender;
   drv->Setup = htrbSetup;

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

   htrAddSupport(drv, "dhtml");

   HTRB.idcnt = 0;

   return 0;
}
