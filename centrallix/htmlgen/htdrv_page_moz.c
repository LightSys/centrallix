
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
	htrAddStylesheetItem(s,"\t#pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:1; Z-INDEX:0; }\n");
	htrAddStylesheetItem(s,"\t#pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	htrAddStylesheetItem(s,"\t#pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:20; Z-INDEX:20; }\n");
	htrAddStylesheetItem(s,"\t#pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; WIDTH:0; HEIGHT:0; Z-INDEX:0;}\n");

  	htrAddScriptInclude(s,"/sys/js/htdrv_page_moz.js",0);

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


	htrAddScriptInit(s, "    pg_togglecursor();\n");


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
