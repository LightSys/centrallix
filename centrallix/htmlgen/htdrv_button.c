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


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTBTN;


/*** htbtnRender - generate the HTML code for the page.
 ***/
int
htbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char type[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    char bgstyle[128];
    char disable_color[64];
    char n_img[128];
    char p_img[128];
    char c_img[128];
    char d_img[128];
    int x,y,w,h,spacing;
    int id, i;
    int is_ts = 1;
    char* dptr;
    int is_enabled = 1;
    pExpression code;
    int box_offset;
    int clip_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTBTN","Netscape DOM or (W3C DOM1 HTML and W3C DOM2 CSS) support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTBTN","Button widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTBTN","Button widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTBTN","Button widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));
	
	if (wgtrGetPropertyValue(tree,"type",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(type,ptr,sizeof(type));

	/* if not a text only button */
	if(strcmp(type,"text"))
	    {
	    if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	    strtcpy(n_img,ptr,sizeof(n_img));
	    }

        if (wgtrGetPropertyValue(tree,"pointimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(p_img,ptr,sizeof(p_img));
        else
            strcpy(p_img, n_img);

        if (wgtrGetPropertyValue(tree,"clickimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(c_img,ptr,sizeof(c_img));
        else
            strcpy(c_img, n_img);

        if (wgtrGetPropertyValue(tree,"disabledimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(d_img,ptr,sizeof(d_img));
        else
            strcpy(d_img, n_img);
	
		/** Threestate button or twostate? **/
		if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

		if(strcmp(type,"image"))
		    if (wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
			{
			mssError(1,"HTBTN","Button widget must have a 'text' property");
			return -1;
			}
		strtcpy(text,ptr,sizeof(text));

		/** Get fgnd colors 1,2, and background color **/
		htrGetBackground(tree, NULL, 0, bgcolor, sizeof(bgcolor));
		htrGetBackground(tree, NULL, 1, bgstyle, sizeof(bgstyle));

		if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
		else
		    strcpy(fgcolor1,"white");
		if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
		else
		    strcpy(fgcolor2,"black");
		if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(disable_color,ptr,sizeof(disable_color));
		else
		    strcpy(disable_color,"#808080");

	/* spacing between image and text if both are supplied */
	if (wgtrGetPropertyValue(tree,"spacing",DATA_T_INTEGER,POD(&spacing)) != 0)  spacing=5;
	
	/** User requesting expression for enabled? **/
		if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
		    {
		    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
		    is_enabled = 0;
		    htrAddExpression(s, name, "enabled", code);
		    }
	    
	/* image only button - based on imagebutton */
	
	if (!strcmp(type,"image") || !strcmp(type,"textoverimage"))
		{
		htrAddStylesheetItem_va(s,"\t#gb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);

		htrAddScriptGlobal(s, "gb_cur_img", "null", 0);
		htrAddScriptGlobal(s, "gb_current", "null", 0);
		htrAddWgtrObjLinkage_va(s, tree, "gb%POSpane", id);

		/** User requesting expression for enabled? **/
		if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
		    {
		    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
		    is_enabled = 0;
		    htrAddExpression(s, name, "enabled", code);
		    }
		/** Widget Tree Stuff **/
		dptr = wgtrGetDName(tree);
		htrAddScriptInit_va(s, "    %STR&SYM = wgtrGetNodeRef(ns,'%STR&SYM');\n", dptr, name);

		if(!strcmp(type,"image")) htrAddScriptInit_va(s,"    gb_init({layer:%STR&SYM, n:'%STR&JSSTR', p:'%STR&JSSTR', c:'%STR&JSSTR', d:'%STR&JSSTR', width:%INT, height:%INT, name:'%STR&SYM', enable:%INT, type:'%STR&JSSTR', text:'%STR&JSSTR'});\n", dptr, n_img, p_img, c_img, d_img, w, h, name,is_enabled,type,text);
		/* text over image needs second layer */
		else htrAddScriptInit_va(s,"    gb_init({layer:\"%STR&SYM\", layer2:htr_subel(%STR&SYM, \"gb%POSpane2\"), n:'%STR&JSSTR', p:'%STR&JSSTR', c:'%STR&JSSTR', d:'%STR&JSSTR', width:%INT, height:%INT, name:'%STR&SYM', enable:%INT, type:'%STR&JSSTR', text:'%STR&JSSTR'});\n", dptr, dptr, id, n_img, p_img, c_img, d_img, w, h, name,is_enabled,type,text);

		/** Include the javascript code for the button **/
		htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
		htrAddScriptInclude(s, "/sys/js/htdrv_button.js", 0);

		/** HTML body <DIV> elements for the layers. **/
		if(!strcmp(type,"image")||!strcmp(type,"textoverimage"))
		    {
		    if (h < 0)
			if(is_enabled)
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\">\n",id,n_img);
			else
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\">\n",id,d_img);
		    else
			if(is_enabled)
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\" width=\"%POS\" height=\"%POS\">\n",id,n_img,w,h);
			else
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\" width=\"%POS\" height=\"%POS\">\n",id,d_img,w,h);
		    }
		/* text over image */
		if(!strcmp(type,"textoverimage"))
		    	{
		    	htrAddStylesheetItem_va(s,"\t#gb%POSpane2 { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,0,0,w,z+1);
		    	htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellpadding=0 height=%INT><tr><td valign=\"center\" align=\"center\"><font color='%STR&HTE'><b>%STR&HTE</b></font></td></tr></table></center></DIV></DIV>\n",id,h,fgcolor1,text);
		    	}
		else
			htrAddBodyItem_va(s,"</DIV>");
		}

	/* other types of buttons - based on textbutton */

	else
	
		{
		/** box adjustment... arrgh **/
		if (s->Capabilities.CSSBox)
		    box_offset = 1;
		else
		    box_offset = 0;
		clip_offset = s->Capabilities.CSSClip?1:0;

		htrAddScriptGlobal(s, "gb_current", "null", 0);

		/** DOM Linkages **/
		htrAddWgtrObjLinkage_va(s, tree, "gb%POSpane",id);
		
		/** Include the javascript code for the button **/
		htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
		htrAddScriptInclude(s, "/sys/js/htdrv_button.js", 0);

		dptr = wgtrGetDName(tree);
		htrAddScriptInit_va(s, "    %STR&SYM = wgtrGetNodeRef(ns,'%STR&SYM');\n", dptr, name);

		if(s->Capabilities.Dom0NS)
		    {
		    /** Ok, write the style header items. **/
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane2 { POSITION:absolute; VISIBILITY:%STR; LEFT:-1; TOP:-1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"inherit":"hidden",w-1,z+1);
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane3 { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"hidden":"inherit",w-1,z+1);
		    htrAddStylesheetItem_va(s,"\t#gb%POStop { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
		    htrAddStylesheetItem_va(s,"\t#gb%POSbtm { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
		    htrAddStylesheetItem_va(s,"\t#gb%POSrgt { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);
		    htrAddStylesheetItem_va(s,"\t#gb%POSlft { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);

		    /** Script initialization call. **/
		    htrAddScriptInit_va(s, "    gb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"gb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"gb%POSpane3\"), top:htr_subel(%STR&SYM, \"gb%POStop\"), bottom:htr_subel(%STR&SYM, \"gb%POSbtm\"), right:htr_subel(%STR&SYM, \"gb%POSrgt\"), left:htr_subel(%STR&SYM, \"gb%POSlft\"), width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR', n:\"%STR&JSSTR\", p:\"%STR&JSSTR\", c:\"%STR&JSSTR\", d:\"%STR&JSSTR\", type:\"%STR&JSSTR\"});\n",
			    dptr, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, w, h, is_ts, name, text,n_img,p_img,c_img,d_img,type);

		    /** HTML body <DIV> elements for the layers. **/
		    if (h >= 0)
			{
			if(!strcmp(type,"text"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,h);
			    }
			else if(!strcmp(type,"image"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,n_img,w,h,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,w,h,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,w,h,h);
			    }
			else if(!strcmp(type,"topimage"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\"><br><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,n_img,fgcolor2,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\"><br><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,fgcolor1,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\"><br><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,disable_color,text,h);
			    }
			else if(!strcmp(type,"bottomimage"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font><br><IMG SRC=\"%STR&HTE\"></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,n_img,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font><br><IMG SRC=\"%STR&HTE\"></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,n_img,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font><br><IMG SRC=\"%STR&HTE\"></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,n_img,h);
			    }
			else if(!strcmp(type,"leftimage"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=%POS cellpadding=0 %STR width=%POS><TR><TD align=right valign=middle><IMG SRC=\"%STR&HTE\"></td><TD align=left valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,spacing,bgcolor,w,n_img,fgcolor2,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=%POS cellpadding=0 width=%POS><TR><TD align=right valign=middle><IMG SRC=\"%STR&HTE\"></td><TD align=left valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,spacing,w,n_img,fgcolor1,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=%POS cellpadding=0 width=%POS><TR><TD align=right valign=middle><IMG SRC=\"%STR&HTE\"></td><TD align=left valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,spacing,w,n_img,disable_color,text,h);
			    }
			else if(!strcmp(type,"rightimage"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=%POS cellpadding=0 %STR width=%POS><TR><TD align=right valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD align=left valign=middle><IMG SRC=\"%STR&HTE\"></td><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,spacing,bgcolor,w,fgcolor2,text,n_img,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=%POS cellpadding=0 width=%POS><TR><TD align=right valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD align=left valign=middle><IMG SRC=\"%STR&HTE\"></td><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,spacing,w,fgcolor1,text,n_img,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=%POS cellpadding=0 width=%POS><TR><TD align=right valign=middle><font color='%STR&HTE'><b>%STR&HTE</b></font></TD><TD align=left valign=middle><IMG SRC=\"%STR&HTE\"></td><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,spacing,w,disable_color,text,n_img,h);
			    }
			else if(!strcmp(type,"textoverimage"))
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,n_img,w,h,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,w,h,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,n_img,w,h,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane4\"><p><font color='%STR&HTE'>%STR&HTE</font></p></DIV>",id,fgcolor1,text);
			    }
			else
			    {
			    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
			    htrAddBodyItem_va(s, "<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,h);
			    }
			}
		    else
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=3 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text);
			}
		    htrAddBodyItem_va(s,"<DIV ID=\"gb%POStop\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
		    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSbtm\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
		    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSrgt\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
		    htrAddBodyItem_va(s,"<DIV ID=\"gb%POSlft\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
		    htrAddBodyItem(s,   "</DIV>\n");
		    }
		else if(s->Capabilities.CSS2)
		    {
		    if(h >=0 )
			{
			htrAddStylesheetItem_va(s,"\t#gb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx %INTpx %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,h-1-2*box_offset+2*clip_offset,0);
			htrAddStylesheetItem_va(s,"\t#gb%POSpane2, #gb%POSpane3 { height: %POSpx;}\n",id,id,h-3);
			htrAddStylesheetItem_va(s,"\t#gb%POSpane { height: %POSpx;}\n",id,h-1-2*box_offset);
			}
		    else
			{
			htrAddStylesheetItem_va(s,"\t#gb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx auto %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,0);
			}
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane, #gb%POSpane2, #gb%POSpane3 { cursor:default; text-align: center; }\n",id,id,id);
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane { %STR border-width: 1px; border-style: solid; border-color: white gray gray white; }\n",id,bgstyle);
		    /*htrAddStylesheetItem_va(s,"\t#gb%dpane { color: %s; }\n",id,fgcolor2);*/
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane2 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:-1px; top: -1px; width:%POSpx; }\n",id,is_enabled?"inherit":"hidden",z+1,w-3);
		    htrAddStylesheetItem_va(s,"\t#gb%POSpane3 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:0px; top: 0px; width:%POSpx; }\n",id,is_enabled?"hidden":"inherit",z+1,w-3);

		    if(!strcmp(type,"text"))
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center>\n",id,h-3,fgcolor2,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,fgcolor1,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,disable_color,text);
			}
		    else if(!strcmp(type,"image"))
			{
	/* working image		htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></center>\n",id,h-3,n_img,w,h-3);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></center></DIV>\n",id,h-3,n_img,w,h-3);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></center></DIV>\n",id,h-3,n_img,w,h-3); */
			}
		    else if(!strcmp(type,"topimage"))
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center>\n",id,h-3,n_img,fgcolor2,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,n_img,fgcolor1,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,n_img,disable_color,text);
			}
		    else if(!strcmp(type,"bottomimage"))
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font><IMG SRC=\"%STR&HTE\"></td></tr></table></center>\n",id,h-3,fgcolor2,text,n_img);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font><IMG SRC=\"%STR&HTE\"></td></tr></table></center></DIV>\n",id,h-3,fgcolor1,text,n_img);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font><IMG SRC=\"%STR&HTE\"></td></tr></table></center></DIV>\n",id,h-3,disable_color,text,n_img);
			}
		    else if(!strcmp(type,"leftimage"))
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><img src=\"%STR&HTE\"></td><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center>\n",id,spacing,h-3,n_img,h-3,fgcolor2,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><img src=\"%STR&HTE\"></td><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,spacing,h-3,n_img,h-3,fgcolor1,text);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><img src=\"%STR&HTE\"></td><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,spacing,h-3,n_img,h-3,disable_color,text);
			}
		    else if(!strcmp(type,"rightimage"))
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"></td></tr></table></center>\n",id,spacing,h-3,fgcolor2,text,h-3,n_img);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"></td></tr></table></center></DIV>\n",id,spacing,h-3,fgcolor1,text,h-3,n_img);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=%POS cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\"></td></tr></table></center></DIV>\n",id,spacing,h-3,disable_color,text,h-3,n_img);
			}
		    else if(!strcmp(type,"textoverimage"))
			{
			//htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><DIV style=\"zindex:50; position:absolute; x=30;y=30;\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></DIV><DIV style=\"zindex:49; position:asbolute; x=0; y=0;\"><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></DIV></center>\n",id,fgcolor2,text,h-3,n_img,w,h-3);
			//htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><DIV style=\"zindex:50; position:absolute; x=30;y=30;\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></DIV><DIV style=\"zindex:49; position:asbolute; x=0; y=0;\"><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></DIV></center>\n",id,fgcolor2,text,h-3,n_img,w,h-3);
			//htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><DIV style=\"zindex:50; position:absolute; x=30;y=30;\"><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></DIV><DIV style=\"zindex:49; position:asbolute; x=0; y=0;\"><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><IMG SRC=\"%STR&HTE\" width=%POS height=%POS></td></tr></table></DIV></center>\n",id,fgcolor2,text,h-3,n_img,w,h-3);
			}
		    else
			{
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>default text</b></font></td></tr></table></center>\n",id,h-3,fgcolor2);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>default text</b></font></td></tr></table></center></DIV>\n",id,h-3,fgcolor1);
			htrAddBodyItem_va(s,"<DIV ID=\"gb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>default text</b></font></td></tr></table></center></DIV>\n",id,h-3,disable_color);
			}
		    htrAddBodyItem(s,   "</DIV>");

		    /** Script initialization call. **/
		    htrAddScriptInit_va(s, "    gb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"gb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"gb%POSpane3\"), top:null, bottom:null, right:null, left:null, width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR', n:'%STR&JSSTR', p:'%STR&JSSTR', c:'%STR&JSSTR', d:'%STR&JSSTR', type:'%STR&JSSTR'});\n",
			    dptr, dptr, id, dptr, id, w, h, is_ts, name, text, n_img, p_img, c_img, d_img,type);
		    }
		else
		    {
		    mssError(0,"HTBTN","Unable to render for this browser");
		    return -1;
		    }
		}

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "gb", "gb_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "gb", "gb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "gb", "gb_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "gb", "gb_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "gb", "gb_mousemove");

	/** IE handles dblclick strangely **/
	if (s->Capabilities.Dom0IE)
	    htrAddEventHandlerFunction(s, "document", "DBLCLICK", "gb", "gb_dblclick");

	/** Check for more sub-widgets within the button. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

    return 0;
    }


/*** htbtnInitialize - register with the ht_render module.
 ***/
int
htbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Button Widget Driver");
	strcpy(drv->WidgetName,"button");
	drv->Render = htbtnRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTBTN.idcnt = 0;

    return 0;
    }
