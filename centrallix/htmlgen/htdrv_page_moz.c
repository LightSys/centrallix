
/*** htpageRenderMozDefault - generate the HTML code for Mozilla default style
 ***/
int
htpageRenderMozDefault(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    pObject sub_w_obj;
    pObjQuery qy;
    int watchdogtimer;
    HtPageStruct t;

	htpageRenderCommon(s,w_obj,z,parentname,parentobj,&t,"IFRAME");

	/** set variable so javascript can run alternate code for a different browser **/    
	htrAddScriptInit(s,
		"    cn_browser=new Object();\n"
		"    cn_browser.netscape47=false;\n"
		"    cn_browser.mozilla=true;\n");

	/** Add focus box **/
	htrAddHeaderItem(s, 
	        "    <STYLE TYPE=\"text/css\">\n"
		"        #pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:1; Z-INDEX:0; }\n"
		"        #pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n"
		"        #pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:20; Z-INDEX:20; }\n"
		"        #pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; WIDTH:0; HEIGHT:0; Z-INDEX:0;}\n"
		"    </STYLE>\n" );

	htrAddBodyItem(s, "<DIV ID=\"pgtop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pglft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgtvl\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgktop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgklft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<IFRAME ID=\"pgping\"></IFRAME>\n");

	htrAddScriptFunction(s, "pg_ping_init","\n"
		"function pg_ping_init(l,i)\n"
		"    {\n"
		"    l.tid=setInterval(pg_ping_send,i,l);\n"
		"    }\n"
		"\n",0);
	
	htrAddScriptFunction(s, "pg_ping_recieve","\n"
		"function pg_ping_recieve()\n"
		"    {\n"
		"    this.removeEventListener('load',pg_ping_recieve,false);\n"
		"    if(this.contentDocument.getElementsByTagName('a')[0].target!=='OK')\n"
		"        {\n"
		"        clearInterval(this.tid);\n"
		"        confirm('you have been disconnected from the server');\n"
		"        }\n"
		"    }\n"
		"\n",0);

	htrAddScriptFunction(s, "pg_ping_send","\n"
		"function pg_ping_send(p)\n"
		"    {\n"
		"    if(p.addEventListener)\n"
		"        p.addEventListener('load',pg_ping_recieve,false);\n"
		"    //else\n"
		"    //    p.onload=pg_ping_recieve;\n"
		"    p.src='/INTERNAL/ping';\n"
		"    }\n"
		"\n",0);

	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	htrAddScriptInit_va(s,"    pg_ping_init(%s.getElementById('pgping'),%i);\n",parentname,watchdogtimer/2*1000);

	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandler(s, "document", "MOUSEMOVE","pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
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
		"            var x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"            var y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"            var w = pg_curarea.width;\n"
		"            var h = pg_curarea.height;\n"
		"            pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft, page.mscolor1, page.mscolor2, document.layers.pgktop.zIndex-1);\n"
		"            break;\n"
		"            }\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOUT", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (e.target == pg_curlayer) pg_curlayer = null;\n"
		"    if (e.target != null && pg_curarea != null && e.target == pg_curarea.layer)\n"
		"        {\n"
		"        pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOVER", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (e.target != null && e.target.pageX != null)\n"
		"        {\n"
		"        pg_curlayer = e.target;\n"
		"        while(pg_curlayer.mainlayer != null) pg_curlayer = pg_curlayer.mainlayer;\n"
		"        }\n" );

	/** CLICK event handler is for making mouse focus the keyboard focus **/
	htrAddEventHandler(s, "document", "MOUSEDOWN", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (pg_curarea != null)\n"
		"        {\n"
		"        var x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"        var y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"        var w = pg_curarea.width;\n"
		"        var h = pg_curarea.height;\n"
		"        if (pg_curkbdlayer && pg_curkbdlayer.losefocushandler)\n"
		"            {\n"
		"            if (!pg_curkbdlayer.losefocushandler()) return true;\n"
		"            if(pg_curkbdlayer != pg_curlayer)\n"
		"                {\n"
		"                pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
		"                pg_insame = false;\n"
		"                }\n"
		"            else pg_insame = true;\n"
		"            }\n"
		"        pg_curkbdarea = pg_curarea;\n"
		"        pg_curkbdlayer = pg_curlayer;\n"
		"        if (pg_curkbdlayer.getfocushandler)\n"
		"            {\n"
		"            var v=pg_curkbdlayer.getfocushandler(e.pageX-pg_curarea.layer.pageX,e.pageY-pg_curarea.layer.pageY,pg_curarea.layer,pg_curarea.cls,pg_curarea.name);\n"
		"            if (v & 1)\n"
		"                {\n"
		"                if (!pg_insame) pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
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
		"    else if (pg_curkbdlayer != null)\n"
		"        {\n"
		"        pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);\n"
		"        if (!pg_curkbdlayer.losefocushandler()) return true;\n"
		"        pg_curkbdarea = null;\n"
		"        pg_curkbdlayer = null;\n"
		"        pg_insame = false;\n"
		"        }\n");

	/** This resets the keyboard focus. **/
	htrAddEventHandler(s, "document", "MOUSEUP", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    //setTimeout(\"document.getElementById('pginpt').getElementsByTagName('form')['tmpform'].getElementsByTagName('textarea')['x'].focus()\",10);\n");

	/** Set colors for the focus layers **/
	htrAddScriptInit_va(s, "    page.kbcolor1 = '%s';\n    page.kbcolor2 = '%s';\n",t.kbfocus1,t.kbfocus2);
	htrAddScriptInit_va(s, "    page.mscolor1 = '%s';\n    page.mscolor2 = '%s';\n",t.msfocus1,t.msfocus2);
	htrAddScriptInit_va(s, "    page.dtcolor1 = '%s';\n    page.dtcolor2 = '%s';\n",t.dtfocus1,t.dtfocus2);
	htrAddScriptInit(s, "    document.LSParent = null;\n");

	/** Function to set modal mode to a layer. **/
	htrAddScriptFunction(s, "pg_setmodal", "\n"
		"function pg_setmodal(l)\n"
		"    {\n"
		"    pg_modallayer = l;\n"
		"    }\n", 0);

	/** Function to find out whether image or layer is in a layer **/
	htrAddScriptFunction(s, "pg_isinlayer", "\n"
		"function pg_isinlayer(outer,inner)\n"
		"    {\n"
		"    if (inner == outer) return true;\n"
		"    if(!outer) return true;\n"
		"    if(!inner) return false;\n"
		"    var i = 0;\n"
		"    var olist = outer.getElementsByTagName('iframe');\n"
		"    for(i=0;i<olist.length;i++)\n"
		"        {\n"
		"        if (olist[i] == inner) return true;\n"
		"        if (pg_isinlayer(olist[i], inner)) return true;\n"
		"        }\n"	// might need to check document/contentDocument as well....
		"    olist = outer.getElementsByTagName('img');\n"
		"    for(i=0;i<olist.length;i++)\n"
		"        {\n"
		"        if (olist[i] == inner) return true;\n"
		"        }\n"
		"    return false;\n"
		"    }\n", 0);

	/** Function to make four layers into a box **/
	htrAddScriptFunction(s, "pg_mkbox", "\n"
		"function pg_mkbox(pl, x,y,w,h, s, tl,bl,rl,ll, c1,c2, z)\n"
		"    {\n"
		"    tl.visibility = 'hidden';\n"
		"    bl.visibility = 'hidden';\n"
		"    rl.visibility = 'hidden';\n"
		"    ll.visibility = 'hidden';\n"
		"    tl.bgColor = c1;\n"
		"    ll.bgColor = c1;\n"
		"    bl.bgColor = c2;\n"
		"    rl.bgColor = c2;\n"
		"    tl.resizeTo(w,1);\n"
		"    tl.moveAbove(pl);\n"
		"    tl.moveToAbsolute(x,y);\n"
		"    tl.zIndex = z;\n"
		"    bl.resizeTo(w+s-1,1);\n"
		"    bl.moveAbove(pl);\n"
		"    bl.moveToAbsolute(x,y+h-s+1);\n"
		"    bl.zIndex = z;\n"
		"    ll.resizeTo(1,h);\n"
		"    ll.moveAbove(pl);\n"
		"    ll.moveToAbsolute(x,y);\n"
		"    ll.zIndex = z;\n"
		"    rl.resizeTo(1,h+1);\n"
		"    rl.moveAbove(pl);\n"
		"    rl.moveToAbsolute(x+w-s+1,y);\n"
		"    rl.zIndex = z;\n"
		"    tl.visibility = 'inherit';\n"
		"    bl.visibility = 'inherit';\n"
		"    rl.visibility = 'inherit';\n"
		"    ll.visibility = 'inherit';\n"
		"    return;\n"
		"    }\n", 0);

	/** To hide a box **/
	htrAddScriptFunction(s, "pg_hidebox", "\n"
		"function pg_hidebox(tl,bl,rl,ll)\n"
		"    {\n"
		"    tl.visibility = 'hidden';\n"
		"    bl.visibility = 'hidden';\n"
		"    rl.visibility = 'hidden';\n"
		"    ll.visibility = 'hidden';\n"
		"    tl.moveAbove(document.layers.pgtvl);\n"
		"    bl.moveAbove(document.layers.pgtvl);\n"
		"    rl.moveAbove(document.layers.pgtvl);\n"
		"    ll.moveAbove(document.layers.pgtvl);\n"
		"    return;\n"
		"    }\n", 0);

	/** Function to make a new clickable "area" **INTERNAL** **/
	htrAddScriptFunction(s, "pg_area", "\n"
		"function pg_area(pl,x,y,w,h,cls,nm,f)\n"
		"    {\n"
		"    this.layer = pl;\n"
		"    this.x = x;\n"
		"    this.y = y;\n"
		"    this.width = w;\n"
		"    this.height = h;\n"
		"    this.name = nm;\n"
		"    this.cls = cls;\n"
		"    this.flags = f;\n"
		"    return this;\n"
		"    }\n", 0);

	/** Function to add a new area to the arealist **/
	htrAddScriptFunction(s, "pg_addarea", "\n"
		"function pg_addarea(pl,x,y,w,h,cls,nm,f)\n"
		"    {\n"
		"    a = new pg_area(pl,x,y,w,h,cls,nm,f);\n"
		"    pg_arealist.splice(0,0,a);\n"
		"    return a;\n"
		"    }\n", 0);

	/** Function to remove an existing area... **/
	htrAddScriptFunction(s, "pg_removearea", "\n"
		"function pg_removearea(a)\n"
		"    {\n"
		"    for(i=0;i<pg_arealist.length;i++)\n"
		"        {\n"
		"        if (pg_arealist[i] == a)\n"
		"            {\n"
		"            pg_arealist.splice(i,1);\n"
		"            return 1;\n"
		"            }\n"
		"        }\n"
		"    return 0;\n"
		"    }\n", 0);

	/** Add a universal resize manager function. **/
	htrAddScriptFunction(s, "pg_resize", "\n"
		"function pg_resize(l)\n"
		"    {\n"
		"    maxheight=0;\n"
		"    maxwidth=0;\n"
		"    for(i=0;i<l.document.layers.length;i++)\n"
		"        {\n"
		"        cl = l.document.layers[i];\n"
		"        if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.y + cl.clip.height > maxheight)\n"
		"            maxheight = cl.y + cl.clip.height;\n"
		"        if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.x + cl.clip.width > maxwidth)\n"
		"            maxwidth = cl.x + cl.clip.width;\n"
		"        }\n"
		"    if (l.maxheight && maxheight > l.maxheight) maxheight = l.maxheight;\n"
		"    if (l.minheight && maxheight < l.minheight) maxheight = l.minheight;\n"
		"    if (l!=window) l.clip.height = maxheight;\n"
		"    else l.document.height = maxheight;\n"
		"    if (l.maxwidth && maxwidth > l.maxwidth) maxwidth = l.maxwidth;\n"
		"    if (l.minwidth && maxwidth < l.minwidth) maxwidth = l.minwidth;\n"
		"    if (l!=window) l.clip.width = maxwidth;\n"
		"    else l.document.width = maxwidth;\n"
		"    }\n", 0);

	/** Add a universal "is visible" function that handles inherited visibility. **/
	htrAddScriptFunction(s, "pg_isvisible", "\n"
		"function pg_isvisible(l)\n"
		"    {\n"
		"    if (l.visibility == 'show') return 1;\n"
		"    else if (l.visibility == 'hidden') return 0;\n"
		"    else if (l == window || l.parentLayer == null) return 1;\n"
		"    else return pg_isvisible(l.parentLayer);\n"
		"    }\n", 0);

	/** Cursor flash **/
	htrAddScriptFunction(s, "pg_togglecursor", "\n"
		"function pg_togglecursor()\n"
		"    {\n"
		"    if (pg_curkbdlayer != null && pg_curkbdlayer.cursorlayer != null)\n"
		"        {\n"
		"        if (pg_curkbdlayer.cursorlayer.visibility != 'inherit')\n"
		"            pg_curkbdlayer.cursorlayer.visibility = 'inherit';\n"
		"        else\n"
		"            pg_curkbdlayer.cursorlayer.visibility = 'hidden';\n"
		"        }\n"
		"    setTimeout(pg_togglecursor,333);\n"
		"    }\n", 0);
	htrAddScriptInit(s, "    pg_togglecursor();\n");

	/** Keyboard input handling **/
	htrAddScriptFunction(s, "pg_addkey", "\n"
		"function pg_addkey(s,e,mod,modmask,mlayer,klayer,tgt,action,aparam)\n"
		"    {\n"
		"    kd = new Object();\n"
		"    kd.startcode = s;\n"
		"    kd.endcode = e;\n"
		"    kd.mod = mod;\n"
		"    kd.modmask = modmask;\n"
		"    kd.mouselayer = mlayer;\n"
		"    kd.kbdlayer = klayer;\n"
		"    kd.target_obj = tgt;\n"
		"    kd.fnname = 'Action' + action;\n"
		"    kd.aparam = aparam;\n"
		"    pg_keylist.splice(0,0,kd);\n"
		"    pg_keylist.sort(pg_cmpkey);\n"
		"    return kd;\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_cmpkey", "\n"
		"function pg_cmpkey(k1,k2)\n"
		"    {\n"
		"    return (k1.endcode-k1.startcode) - (k2.endcode-k2.startcode);\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_removekey", "\n"
		"function pg_removekey(kd)\n"
		"    {\n"
		"    for(i=0;i<pg_keylist.length;i++)\n"
		"        {\n"
		"        if (pg_keylist[i] == kd)\n"
		"            {\n"
		"            pg_keylist.splice(i,1);\n"
		"            return 1;\n"
		"            }\n"
		"        }\n"
		"    return 0;\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_keytimeout", "\n"
		"function pg_keytimeout()\n"
		"    {\n"
		"    if (pg_lastkey != -1)\n"
		"        {\n"
		"        e = new Object();\n"
		"        e.which = pg_lastkey;\n"
		"        e.modifiers = pg_lastmodifiers;\n"
		"        pg_keyhandler(pg_lastkey, pg_lastmodifiers, e);\n"
		"        delete e;\n"
		"        pg_keytimeoutid = setTimeout(pg_keytimeout, 50);\n"
		"        }\n"
		"    }\n", 0);

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

	htrAddScriptFunction(s, "pg_keyhandler", "\n"
		"function pg_keyhandler(k,m,e)\n"
		"    {\n"
		"    pg_lastmodifiers = m;\n"
		"    if (pg_curkbdlayer != null && pg_curkbdlayer.keyhandler != null && pg_curkbdlayer.keyhandler(pg_curkbdlayer,e,k) == true) return false;\n"
		"    for(i=0;i<pg_keylist.length;i++)\n"
		"        {\n"
		"        if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (m & pg_keylist[i].modmask) == pg_keylist[i].mod)\n"
		"            {\n"
		"            pg_keylist[i].aparam.KeyCode = k;\n"
		"            pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam);\n"
		"            return false;\n"
		"            }\n"
		"        }\n"
		"    return false;\n"
		"    }\n", 0);

	// this isn't needed in Mozilla...
	//htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");

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

	//htrAddScriptInit(s,"    return false;\n");
	htrAddScriptInit(s,
		"    document.getElementById('pgtop').style.clip.left=0;\n"
		"    document.getElementById('pgtop').style.clip.top=0;\n"
		"    document.getElementById('pgtop').style.clip.x=1;\n"
		"    document.getElementById('pgtop').style.clip.y=1;\n"
		"    //document.getElementById('pginpt').style.left=window.innerWidth-2;\n"
		"    //document.getElementById('pginpt').style.top=20;\n"
		"    //document.getElementById('pginpt').visibility = 'inherit';\n"
		"    //document.getElementById('pginpt').getElementsByTagName('form')['tmpform'].getElementsByTagName('textarea')['x'].focus();\n");

    return 0;
    }
