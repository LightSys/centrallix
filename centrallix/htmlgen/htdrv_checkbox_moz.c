
/* 
   htcbMozDefRender - generate the HTML code for the page (Mozilla:Default).
*/
int htcbMozDefRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj) {
   char fieldname[HT_FIELDNAME_SIZE];
   int x=-1,y=-1,checked=0;
   int id;
   char *ptr;

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      {
      strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   if (wgtrGetPropertyValue(tree,"checked",DATA_T_STRING,POD(&ptr)) != 0)
      { 
      checked = 0;
      } 
   else
      { 
      checked = 1;
      } 

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#cb%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; HEIGHT:13px; WIDTH:13pX; Z-INDEX:%d; }\n",id,x,y,z);

   htrAddScriptInclude(s,"/sys/js/htdrv_checkbox_moz.js",0);

   htrAddEventHandler(s, "document","MOUSEDOWN", "checkbox", 
      "\n"
      "   if (e.target != null && e.target.kind == 'checkbox') {\n"
      "      if (e.target.layer != null)\n"
      "         layer = e.target.layer;\n"
      "      else\n"
      "         layer = e.target;\n"
      "      if (layer.enabled)\n"
      "         checkbox_toggleMode(layer);\n"
      "   }\n"
      "\n");

   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    checkbox_init(%s.getElementById('cb%dmain'),\"%s\",%d);\n", parentname, id,fieldname,checked);

   /** HTML body <DIV> element for the layers. **/
   htrAddBodyItem_va(s,"   <DIV ID=\"cb%dmain\">\n",id);
   if (checked)
      {
      htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_checked.gif\"/>\n");
      }
   else
      {
      htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_unchecked.gif\"/>\n");
      }
   htrAddBodyItem_va(s,"   </DIV>\n");
   return 0;
}
