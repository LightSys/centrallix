
/* 
   htcbMozDefRender - generate the HTML code for the page (Mozilla:Default).
*/
int htcbMozDefRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char fieldname[HT_FIELDNAME_SIZE];
   int x=-1,y=-1,checked=0;
   int id;
   char *ptr;

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
      {
      strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   if (objGetAttrValue(w_obj,"checked",POD(&ptr)) != 0)
      { 
      checked = 0;
      } 
   else
      { 
      checked = 1;
      } 

   /** Ok, write the style header items. **/
   htrAddHeaderItem_va(s,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem_va(s,"\t#cb%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; HEIGHT:13px; WIDTH:13pX; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem_va(s,"    </STYLE>\n");

   /** Get value function **/
   htrAddScriptFunction(s, "checkbox_getvalue", "\n"
      "function checkbox_getvalue()\n"
      "    {\n"
      "    return this.document.images[0].is_checked;\n" /* not sure why, but it works - JDR */
      "    }\n",0);

   /** Set value function **/
   htrAddScriptFunction(s, "checkbox_setvalue", "\n"
      "function checkbox_setvalue(v)\n"
      "    {\n"
      "    if (v)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        this.document.images[0].is_checked = 1;\n"
      "        this.is_checked = 1;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        this.document.images[0].is_checked = 0;\n"
      "        this.is_checked = 0;\n"
      "        }\n"
      "    }\n",0);

   /** Clear function **/
   htrAddScriptFunction(s, "checkbox_clearvalue", "\n"
      "function checkbox_clearvalue()\n"
      "    {\n"
      "    this.setvalue(false);\n"
      "    }\n", 0);

   /** reset value function **/
   htrAddScriptFunction(s, "checkbox_resetvalue", "\n"
      "function checkbox_resetvalue()\n"
      "    {\n"
      "    this.setvalue(false);\n"
      "    }\n",0);

   /** enable function **/
   htrAddScriptFunction(s, "checkbox_enable", "\n"
      "function checkbox_enable()\n"
      "    {\n"
      "    this.enabled = true;\n"
      "    this.document.images[0].uncheckedImage.enabled = this.enabled;\n"
      "    this.document.images[0].checkedImage.enabled = this.enabled;\n"
      "    this.document.images[0].enabled = this.enabled;\n"
      "    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked.gif';\n"
      "    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked.gif';\n"
      "    if (this.is_checked)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        }\n"
      "    }\n",0);

   /** read-only function - marks the widget as "readonly" **/
   htrAddScriptFunction(s, "checkbox_readonly", "\n"
      "function checkbox_readonly()\n"
      "    {\n"
      "    this.enabled = false;\n"
      "    this.document.images[0].uncheckedImage.enabled = this.enabled;\n"
      "    this.document.images[0].checkedImage.enabled = this.enabled;\n"
      "    this.document.images[0].enabled = this.enabled;\n"
      "    }\n",0);

   /** disable function - disables the widget completely (visually too) **/
   htrAddScriptFunction(s, "checkbox_disable", "\n"
      "function checkbox_disable()\n"
      "    {\n"
      "    this.readonly();\n"
      "    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked_dis.gif';\n"
      "    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked_dis.gif';\n"
      "    if (this.is_checked)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        }\n"
      "    }\n",0);

   /** Checkbox initializer **/
   htrAddScriptFunction(s, "checkbox_init", "\n"
      "function checkbox_init(l,fieldname,checked) {\n"
      "   l.kind = 'checkbox';\n"
      "   l.fieldname = fieldname;\n"
      "   l.is_checked = checked;\n"
      "   l.enabled = true;\n"
      "   l.form = fm_current;\n"
      "   var img=l.getElementsByTagName('img')[0];\n"
      "   img.kind = 'checkbox';\n"
      "   img.form = l.form;\n"
      "   img.parentLayer = l;\n"
      "   img.is_checked = l.is_checked;\n"
      "   img.enabled = l.enabled;\n"
      "   img.uncheckedImage = new Image();\n"
      "   img.uncheckedImage.kind = 'checkbox';\n"
      "   img.uncheckedImage.src = \"/sys/images/checkbox_unchecked.gif\";\n"
      "   img.uncheckedImage.is_checked = l.is_checked;\n"
      "   img.uncheckedImage.enabled = l.enabled;\n"
      "   img.checkedImage = new Image();\n"
      "   img.checkedImage.kind = 'checkbox';\n"
      "   img.checkedImage.src = \"/sys/images/checkbox_checked.gif\";\n"
      "   img.checkedImage.is_checked = l.is_checked;\n"
      "   img.checkedImage.enabled = l.enabled;\n"
      "   l.setvalue   = checkbox_setvalue;\n"
      "   l.getvalue   = checkbox_getvalue;\n"
      "   l.clearvalue = checkbox_clearvalue;\n"
      "   l.resetvalue = checkbox_resetvalue;\n"
      "   l.enable     = checkbox_enable;\n"
      "   l.readonly   = checkbox_readonly;\n"
      "   l.disable    = checkbox_disable;\n"
      "   if (fm_current) fm_current.Register(l);\n"
      "}\n", 0);

   /** Checkbox toggle mode function **/
   htrAddScriptFunction(s, "checkbox_toggleMode", "\n"
      "function checkbox_toggleMode(layer) {\n"
      "   if (layer.form) {\n"
      "       if (layer.parentLayer) {\n"
      "           layer.form.DataNotify(layer.parentLayer);\n"
      "       } else {\n"
      "           layer.form.DataNotify(layer);\n"
      "       }\n"
      "   }\n"
      "   if (layer.is_checked) {\n"
      "       layer.src = layer.uncheckedImage.src;\n"
      "       layer.is_checked = 0;\n"
      "   } else {\n"
      "       layer.src = layer.checkedImage.src;\n"
      "       layer.is_checked = 1;\n"
      "   }\n"
      "}\n", 0);

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
