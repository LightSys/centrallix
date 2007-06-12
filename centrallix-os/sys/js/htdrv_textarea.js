// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

/** Get value function **/
function tx_getvalue()
    {
    var txt = '';
    for (var i=0;i<this.rows.length;i++)
        {
        if (this.rows[i].newLine) txt += '\n'+this.rows[i].content;
        else txt += this.rows[i].content;
        }
    return txt;
    }

/** Set value function **/
function tx_setvalue(txt)
    {
    txt = htutil_obscure(txt);
    var reg = /(.*)(\n*)/;
    do
        {
        reg.exec(txt);
        var para = RegExp.$1;
        var newL = RegExp.$2;
        txt = txt.substr(para.length+newL.length);
        tx_wordWrapDown(this,this.rows.length-1,para,0);
        if (newL)
            {
            for(var i=0;i<newL.length;i++)
                {
                tx_insertRow(this,this.rows.length,'');
                this.rows[this.rows.length-1].newLine = 1;
                }
            }
        }
    while (txt)
    for(var i in this.rows)
        {
        htr_setvisibility(this.rows[i].hiddenLayer, 'hidden');
        htr_setvisibility(this.rows[i].contentLayer, 'inherit');
        this.rows[i].changed = 0;
        }
    }

/** Clear function **/
function tx_clearvalue()
    {
    if (this.getvalue() == '') return;
    tx_updateRow(this,0,'');
    for(var i=0;i<this.rows.length;i++)
        {
        htr_setvisibility(this.rows[i].contentLayer, 'hidden');
        htr_setvisibility(this.rows[i].hiddenLayer, 'hidden');
        }
    if (this.rows.length > 0) this.rows.splice(1,this.rows.length-1);
    if (tx_current == this)
        {
        this.cursorRow = 0;
        this.cursorCol = 0;
        moveToAbsolute(ibeam_current, getPageX(this.rows[this.cursorRow].contentLayer) + tx_xpos(this,this.cursorRow,this.cursorCol), getPageY(this.rows[this.cursorRow].contentLayer));
	htr_setvisibility(ibeam_current, 'inherit');
        }
    }

/** Enable control function **/
function tx_enable()
    {
    htr_setbackground(this, this.bg);
    //eval('this.document.'+this.bg);
    this.enabled='full';
    }

/** Disable control function **/
function tx_disable()
    {
    htr_setbackground(this, "bgcolor='#e0e0e0'");
    //this.document.background='';
    //this.document.bgColor='#e0e0e0';
    this.enabled='disabled';
    }

/** Readonly-mode function **/
function tx_readonly()
    {
    htr_setbackground(this, this.bg);
    //eval('this.document.'+this.bg);
    this.enabled='readonly';
    }

/** Changes the actual cursor position (cursorRow & cursorCol) **/
function tx_getCursorPos(l,chng,opt)
    {
    l.cursorPos += chng;
    pos = l.cursorPos;
    var i = 0;
    while (i < l.rows.length && pos-l.rows[i].content.length > 0)
        {
        pos -= l.rows[i].content.length;
        i++;
        if (l.rows[i].newLine) pos--;
        }
    l.cursorRow = i;
    l.cursorCol = pos;
    if (l.rows[i+1] && !l.rows[i+1].newLine && pos == l.rows[i].content.length && opt)
        {
        l.cursorRow++;
        l.cursorCol = 0;
        }
    }

/** Sets the positioning of the cursor within the text (cursorPos) **/
function tx_setCursorPos(l,row,col)
    {
    var pos=0;
    for(var i=0;i<row;i++)
        {
        pos += l.rows[i].content.length;
        if (l.rows[i].newLine) pos++;
        }
    if (l.rows[i].newLine) pos++;
    pos += col;
    return pos;
    }

/** Inserts an editable row to the textarea **/
function tx_insertRow(l, index, txt)
    {
    if (index > l.rows.length) index = l.rows.length;
    r = new Object();
    r.contentLayer = htr_new_layer(getClipWidth(l)-2, l);
    r.hiddenLayer = htr_new_layer(getClipWidth(l)-2, l);
    r.charWidths = new Array();
    r.contentLayer.row = r;
    r.hiddenLayer.row = r;
    r.textarea = l;
    htr_init_layer(r.hiddenLayer, l, 'tx');
    htr_init_layer(r.contentLayer, l, 'tx');
    htr_setvisibility(r.hiddenLayer, 'hidden');
    htr_setvisibility(r.contentLayer, 'hidden');
    r.content = null;
    tx_write(r.contentLayer, txt);
    r.changed = 1;
    l.rows.splice(index,0,r);
    for(i=index;i<l.rows.length;i++)
        {
        if (l.rows[i] != null)
            {
            moveTo(l.rows[i].contentLayer, 2, i*text_metric.charHeight+1);
            moveTo(l.rows[i].hiddenLayer, 2, i*text_metric.charHeight+1);
            }
        }
    }


/** get the x pos of a given (row,col) in the text **/
function tx_xpos(l, row, col)
    {
    if (l.mode == 0) // text
	return col*text_metric.charWidth;
    var xpos = 0;
    for (var i =0; i<l.rows[row].content.length && i < col; i++)
	xpos += l.rows[row].charWidths[i];
    return xpos;
    }

/** get the y pos of a given (row,col) in the text */
function tx_ypos(l, row, col)
    {
    if (l.mode == 0) // text
	return row*text_metric.charHeight;
    var ypos = 0;
    for (var i =0; i<l.rows.length && i < row; i++)
	ypos += l.rows[i].charHeight;
    return ypos;
    }

/** change a row's content, html style **/
/** nonoptimized! **/
function tx_html_write(l, c)
    {
    l.row.charWidths = new Array();
    if (cx__capabilities.Dom0NS) 
	{
	l.document.write('');
	l.document.close();
	}
    else
	l.innerHTML = '<pre style="padding:0px; margin:0px;">';
    var totalwidth = 0;
    for(var i = 0; i < c.length; i++)
	{
	if (cx__capabilities.Dom0NS)
	    {
	    l.document.write(htutil_encode(c.substr(0,i+1)));
	    l.document.close();
	    }
	else
	    l.innerHTML += htutil_encode(c.charAt(i));
	l.row.charWidths[i] = getClipWidth(l) - totalwidth;
	totalwidth += l.row.charWidths[i];
	}
    if (cx__capabilities.Dom0NS) 
	{
	/*l.document.write('');
	l.document.close();*/
	}
    else
	l.innerHTML += '</pre>';
    var str = '';
    for(var i = 0; i<l.row.charWidths.length; i++)
	{
	str += ' ' + l.row.charWidths[i] + ',';
	}
    l.row.charHeight = getClipHeight(l);
    l.row.content = c;
    //confirm(str);
    }

/** Changes a row's (layer) content **/
function tx_write(l, c)
    {
    if (l.row.content == c) return;
    if (l.row.textarea.mode != 0) return tx_html_write(l, c);
    if (cx__capabilities.Dom0NS)
        {
        l.document.write('<PRE>' + htutil_encode(c) + '</PRE> ');
        l.document.close();
        }
    else if (cx__capabilities.Dom1HTML)
        {        
        l.innerHTML = '<PRE style="padding:0px; margin:0px;">' + htutil_encode(c) + '</PRE> ';
        }
    for(var i = 0; i < c.length; i++) l.row.charWidths[i] = text_metric.charWidth;
    l.row.charHeight = text_metric.charHeight;
    l.row.content = c;
    }

/** Deletes an editable row from the textarea **/
function tx_deleteRow(l, index)
    {
    htr_setvisibility(l.rows[index].contentLayer, 'hidden');
    htr_setvisibility(l.rows[index].hiddenLayer, 'hidden');
    l.rows.splice(index,1);
    for(i=index;i<l.rows.length;i++)
        {
        if (l.rows[i] != null)
            {
            moveTo(l.rows[i].contentLayer, 1, i*text_metric.charHeight);
            moveTo(l.rows[i].hiddenLayer, 1, i*text_metric.charHeight);
            }
        }
    }

/** Updates an existing row's contents **/
function tx_updateRow(l, index, txt)
    {
    if (index > l.rows.length) index = l.rows.length-1;
    r = l.rows[index];
    if (r.content == txt) return;
    tx_write(r.hiddenLayer, txt);
    tmp = r.contentLayer;
    r.contentLayer = r.hiddenLayer;
    r.hiddenLayer = tmp;
    r.changed = 1;
    }

/** Wraps words from beginning of row to the end of the above row (recursive) **/
function tx_wordWrapUp(l,index,txt,c)
    {
    if (!l.rows[index+1] || l.rows[index+1].newLine)
        {
        if (!txt && !l.rows[index].newLine && index>0) tx_deleteRow(l,index);
        return c;
        }
    var open = l.rowCharLimit - txt.length;
    if (open == 0) return c;
    var avail = l.rows[index+1].content.substr(0,open);
    if (l.rows[index+1].content[open] == ' ' || !l.rows[index+1].content[open])
        {
        var add = avail;
        var sub = l.rows[index+1].content.substr(open);
        }
    else if (txt.length == l.rowCharLimit-1)
    	{
	for(var i=0;i<txt.length;i++) if(txt[i] == ' ') break;
	if(i==txt.length)
	    {
	    var add = avail.substr(0,open);
	    var sub = l.rows[index+1].content.substr(open);
	    }
	else
	    {
	    for (var i=open; avail[i]!=' ' && i>=0; i--) {}
	    if (i < 0) return c;
	    var add = avail.substr(0,i+1);
	    var sub = l.rows[index+1].content.substr(i+1);
	    }
	}
    else
        {
        for (var i=open; avail[i]!=' ' && i>=0; i--) {}
        if (i < 0) return c;
        var add = avail.substr(0,i+1);
        var sub = l.rows[index+1].content.substr(i+1);
        }
    tx_updateRow(l,index,txt+add);
    tx_updateRow(l,index+1,sub);
    return tx_wordWrapUp(l,index+1,sub,1);
    }

/** Wraps words from the end of row to the beginning of below row (recursive) **/
function tx_wordWrapDown(l,index,txt,c)
    {
    if (!txt) return c;
    if (txt.length <= l.rowCharLimit) 
        {
        tx_updateRow(l,index,txt);
        return c;
        }
    if (txt[l.rowCharLimit] == ' ')
        {
        var sub = txt.substr(0,l.rowCharLimit);
        var add = txt.substr(l.rowCharLimit);
        }
    else
        {
	if(txt.length > l.rowCharLimit)
	    {
	    for(var i=0;i<txt.length;i++) if(txt[i] == ' ') break;
	    if(i > l.rowCharLimit)
	    	{
		var sub = txt.substr(0,l.rowCharLimit);
		var add = txt.substr(l.rowCharLimit);
		}
	    else
	    	{
		for (var i=l.rowCharLimit-1; txt[i]!=' ' && i>=0; i--){}
		var sub = txt.substr(0,i+1);
		var add = txt.substr(i+1);
		}
	    }
	else
	    {
	    for (var i=l.rowCharLimit-1; txt[i]!=' ' && i>=0; i--){}
	    var sub = txt.substr(0,i+1);
	    var add = txt.substr(i+1);
	    }
        }
    tx_updateRow(l,index,sub);
    if (!l.rows[index+1] || l.rows[index+1].newLine) tx_insertRow(l,index+1,'');
    return tx_wordWrapDown(l,index+1,add+l.rows[index+1].content,1);
    }

/** Textarea keyboard handler **/
function tx_keyhandler(l,e,k)
    {
    if (!tx_current) return true;
    if (tx_current.enabled!='full') return 1;
    if (k != 27 && k != 9 && tx_current.form) 
	tx_current.form.DataNotify(tx_current);
    if (k >= 32 && k < 127)
        {
        txt = l.rows[l.cursorRow].content;
        if (l.rows[l.cursorRow+1] && l.cursorCol == l.rows[l.cursorRow].content.length && l.rows[l.cursorRow+1].content[0] != ' ' && k!=32 && !l.rows[l.cursorRow+1].newLine)
            {
            tx_wordWrapDown(l,l.cursorRow+1,String.fromCharCode(k)+l.rows[l.cursorRow+1].content,0);
            tx_getCursorPos(l,1,0);
            }
        else
            {
            txt = l.rows[l.cursorRow].content;
            newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);
            tx_wordWrapDown(l,l.cursorRow,newtxt,0);
            if (l.cursorRow > 0) tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0);
            tx_getCursorPos(l,1,0);
            }
        }
    else if (k == 13)
        {
        txt = l.rows[l.cursorRow].content;
        oldrow = txt.substr(0,l.cursorCol);
        newrow = txt.substr(l.cursorCol,txt.length);
        tx_updateRow(l,l.cursorRow,oldrow);
        tx_insertRow(l,l.cursorRow+1,newrow);
        l.rows[l.cursorRow+1].newLine = 1;
        tx_wordWrapUp(l,l.cursorRow+1,newrow,0);
        tx_getCursorPos(l,1,0);
        }
    else if (k == 9)
	{
	if (tx_current.form) tx_current.form.TabNotify(tx_current);
	return true;
	}
    else if (k == 27)
	{
	if (tx_current.form) tx_current.form.EscNotify(tx_current);
	return true;
	}
    else if (k == 8)
        {
        txt = l.rows[l.cursorRow].content;
        var beginP = l.rows[l.cursorRow].newLine;
        if (l.cursorCol == 0)
            {
            if (l.cursorRow == 0) return false;
            var txtpre = l.rows[l.cursorRow-1].content;
            if (l.rows[l.cursorRow].newLine)
                {
                l.rows[l.cursorRow].newLine = 0;
                tx_deleteRow(l,l.cursorRow);
                tx_wordWrapDown(l,l.cursorRow-1,txtpre+txt,0);
                tx_getCursorPos(l,-1,0);
                }
            else
                {
                tx_deleteRow(l,l.cursorRow);
                tx_wordWrapDown(l,l.cursorRow-1,txtpre.substr(0,txtpre.length-1) + txt,0);
                tx_getCursorPos(l,-1,0);
                }
            }
        else if (l.cursorCol == 1 && l.cursorRow > 0 && l.rows[l.cursorRow].content[0] == ' ' && !beginP)
            {
            tx_deleteRow(l,l.cursorRow);
            tx_wordWrapDown(l,l.cursorRow-1,l.rows[l.cursorRow-1].content + txt.substr(1));
            tx_getCursorPos(l,-1,0);
            }
        else if (l.cursorCol == l.rows[l.cursorRow].content.length && l.rows[l.cursorRow+1] && l.rows[l.cursorRow+1].content[0] != ' ' && !l.rows[l.cursorRow+1].newLine)
            {
            var nextRow = l.rows[l.cursorRow+1].content;
            tx_deleteRow(l,l.cursorRow+1);
            tx_wordWrapDown(l,l.cursorRow,txt.substr(0,txt.length-1) + nextRow,0);
            tx_getCursorPos(l,-1,0);
            }
        else
            {
            newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol);
            tx_updateRow(l,l.cursorRow,newtxt);
            if (l.cursorRow > 0 && !beginP) var f = tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0);
            if (!f) tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0);
            if (l.rows[l.cursorRow] && l.rows[l.cursorRow].content[0] == ' ') tx_getCursorPos(l,-1,0);
            else tx_getCursorPos(l,-1,1);
            }
        }
    else if (k == 127)
        {
        txt = l.rows[l.cursorRow].content;
        var beginP = l.rows[l.cursorRow].newLine;
        if (l.cursorCol == txt.length)
            {
            if (l.cursorRow == l.rows.length-1) return false;
            if (l.rows[l.cursorRow+1].newLine)
                {
                l.rows[l.cursorRow+1].newLine = 0;
                var newtxt = txt+l.rows[l.cursorRow+1].content;
                tx_deleteRow(l,l.cursorRow+1);
                tx_wordWrapDown(l,l.cursorRow,newtxt);
                tx_getCursorPos(l,0,0);
                }
            else
                {
                tx_updateRow(l,l.cursorRow+1,l.rows[l.cursorRow+1].content.substr(1,l.rows[l.cursorRow+1].content.length-1));
                if (!tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0))
                    if (l.rows[l.cursorRow+1]) tx_wordWrapUp(l,l.cursorRow+1,l.rows[l.cursorRow+1].content,0);
                tx_getCursorPos(l,0,1);
                }
            }
        else if (l.cursorCol == txt.length-1 && txt.length>1 && txt[txt.length-1] == ' ' && txt[txt.length-2] != ' ' && l.rows[l.cursorRow+1] && l.rows[l.cursorRow+1].content[0] != ' ' && !l.rows[l.cursorRow+1].newLine)
            {
            var nextRow = l.rows[l.cursorRow+1].content;
            tx_deleteRow(l,l.cursorRow+1);
            tx_wordWrapDown(l,l.cursorRow,txt.substr(0,txt.length-1) + nextRow,0);
            tx_getCursorPos(l,-1,0);
            }
        else if (l.cursorCol == 0 && l.cursorRow > 0 && l.rows[l.cursorRow].content[0] == ' ' && !beginP) 
            {
            tx_deleteRow(l,l.cursorRow);
            tx_wordWrapDown(l,l.cursorRow-1,l.rows[l.cursorRow-1].content + txt.substr(1));
            tx_getCursorPos(l,0,0);
            }
        else
            {
            newtxt = txt.substr(0,l.cursorCol) + txt.substr(l.cursorCol+1,txt.length);
            tx_updateRow(l,l.cursorRow,newtxt);
            if (l.cursorRow > 0) { var f = tx_wordWrapUp(l,l.cursorRow-1,l.rows[l.cursorRow-1].content,0); }
            if (!f) tx_wordWrapUp(l,l.cursorRow,l.rows[l.cursorRow].content,0);
            tx_getCursorPos(l,0,1);
            }
        }
    else return true;
    for(i=0;i<l.rows.length;i++)
        {
        if (l.rows[i].changed == 1)
            {
            htr_setvisibility(l.rows[i].hiddenLayer, 'hidden');
            htr_setvisibility(l.rows[i].contentLayer, 'inherit');
            l.rows[i].changed = 0;
            }
        }
    moveToAbsolute(ibeam_current, getPageX(l.rows[l.cursorRow].contentLayer) + tx_xpos(l,l.cursorRow,l.cursorCol), getPageY(l.rows[l.cursorRow].contentLayer));
    htr_setvisibility(ibeam_current, 'inherit');
    cn_activate(l, 'DataChange');
    return false;
    }


/** Set focus to a new textarea **/
function tx_select(x,y,l,c,n)
    {
    if (l.form) l.form.FocusNotify(l);
    if (l.enabled != 'full') return 0;
    var cheight = 0;
    l.cursorRow = 0;
    for (var i = 0; i<l.rows.length;i++)
	{
	if (y >= cheight) l.cursorRow = i;
	cheight += l.rows[i].charHeight;
	}
    var cwidth = 0;
    l.cursorCol = 0;
    for (var i = 0; i<l.rows[l.cursorRow].content.length; i++)
	{
	if (x >= cwidth) l.cursorCol = i;
	cwidth += l.rows[l.cursorRow].charWidths[i];
	}
    if (x >= cwidth) l.cursorCol = i;
    /*l.cursorRow = Math.floor(y/text_metric.charHeight);
    l.cursorCol = Math.round(x/text_metric.charWidth);*/
    if (l.cursorRow >= l.rows.length)
        {
	l.cursorRow = l.rows.length - 1;
	l.cursorCol = l.rows[l.cursorRow].content.length;
        }
    else if (l.cursorCol > l.rows[l.cursorRow].content.length) l.cursorCol = l.rows[l.cursorRow].content.length;
    l.cursorPos = tx_setCursorPos(l,l.cursorRow,l.cursorCol);
    l.cursorlayer = ibeam_current;
    tx_current = l;
    pg_set_style(ibeam_current,'visibility', 'hidden');
    if (cx__capabilities.Dom1HTML)
	l.appendChild(ibeam_current);
    moveAbove(ibeam_current,l);
    moveToAbsolute(ibeam_current,getPageX(l.rows[0].contentLayer) + tx_xpos(l,l.cursorRow,l.cursorCol), getPageY(l.rows[0].contentLayer) + tx_ypos(l,l.cursorRow,l.cursorCol));
    htr_setzindex(ibeam_current, htr_getzindex(l)+2);
    pg_set_style(ibeam_current,'visibility','inherit');
    cn_activate(l, 'GetFocus');
    return 1;
    }

/** Take focus away from textarea **/
function tx_deselect()
    {
    htr_setvisibility(ibeam_current, 'hidden');
    if (tx_current) tx_current.cursorlayer = null;
    cn_activate(tx_current, 'LoseFocus');
    tx_current = null;
    return true;
    }

/** Textarea initializer **/
function tx_init(param)
    {
    var l = param.layer;
    ifc_init_widget(l);
    if (!param.mainBackground)
	{
	if (cx__capabilities.Dom0NS)
	    {
	    l.bg = "bgcolor='#c0c0c0'";
	    }
	else if (cx__capabilities.Dom0IE)
	    {
	    l.bg = "backgroundColor='#c0c0c0'";
	    }
	}
    else
	{
	//l.bg = "bgcolor='#c0c0c0'";
	l.bg = param.mainBackground;
	}
    htutil_tag_images(l.document,'tx',l,l);
    htr_init_layer(l,l,'tx');
    l.fieldname = param.fieldname;
    ibeam_init();
    l.rowCharLimit = Math.floor((getClipWidth(l)-2)/text_metric.charWidth);
    l.cursorPos = 0;
    l.rows = new Array();
    tx_insertRow(l,0,'');
    l.keyhandler = tx_keyhandler;
    l.getfocushandler = tx_select;
    l.losefocushandler = tx_deselect;
    l.getvalue = tx_getvalue;
    l.setvalue = tx_setvalue;
    l.clearvalue = tx_clearvalue;
    l.setoptions = null;
    l.enablenew = tx_enable;
    l.disable = tx_disable;
    l.readonly = tx_readonly;
    if (param.isReadonly)
        {
        l.enablemodify = tx_disable;
        l.enabled = 'disable';
        }
    else
        {
        l.enablemodify = tx_enable;
        l.enabled = 'full';
        }
    l.mode = param.mode; // 0=text, 1=html, 2=wiki
    l.isFormStatusWidget = false;
    if (cx__capabilities.CSSBox)
	pg_addarea(l, -1, -1, getClipWidth(l)+3, getClipHeight(l)+3, 'tbox', 'tbox', param.isReadonly?0:3);
    else
	pg_addarea(l, -1, -1, getClipWidth(l)+1, getClipHeight(l)+1, 'tbox', 'tbox', param.isReadonly?0:3);
    if (param.form) l.form = wgtrGetNode(l, param.form);
    if (!l.form) l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) l.form.Register(l);
    l.changed = false;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("GetFocus");
    ie.Add("LoseFocus");
    ie.Add("DataChange");

    return l;
    }

// Event handlers
function tx_mouseup(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mousedown(e)
    {
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mouseover(e)
    {
    if (e.kind == 'tx')
        {
        if (!tx_cur_mainlayer)
            {
            cn_activate(e.mainlayer, 'MouseOver');
            tx_cur_mainlayer = e.mainlayer;
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tx_mousemove(e)
    {
    if (tx_cur_mainlayer && e.kind != 'tx')
        {
	// This is MouseOut Detection!
        cn_activate(tx_cur_mainlayer, 'MouseOut');
        tx_cur_mainlayer = null;
        }
    if (e.kind == 'tx') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
