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
        this.rows[i].hiddenLayer.visibility = 'hidden';
        this.rows[i].contentLayer.visibility = 'inherit';
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
        this.rows[i].contentLayer.visibility = 'hidden';
        this.rows[i].hiddenLayer.visibility = 'hidden';
        }
    if (this.rows.length > 0) this.rows.splice(1,this.rows.length-1);
    if (tx_current == this)
        {
        this.cursorRow = 0;
        this.cursorCol = 0;
        tx_ibeam.moveToAbsolute(this.rows[this.cursorRow].contentLayer.pageX + this.cursorCol*tx_metric.charWidth, this.rows[this.cursorRow].contentLayer.pageY);
        tx_ibeam.visibility = 'inherit';
        }
    }

/** Enable control function **/
function tx_enable()
    {
    eval('this.document.'+this.bg);
    this.enabled='full';
    }

/** Disable control function **/
function tx_disable()
    {
    this.document.background='';
    this.document.bgColor='#e0e0e0';
    this.enabled='disabled';
    }

/** Readonly-mode function **/
function tx_readonly()
    {
    eval('this.document.'+this.bg);
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
    r.content = txt;
    r.contentLayer = new Layer(l.clip.width-2, l);
    r.hiddenLayer = new Layer(l.clip.width-1, l);
    r.contentLayer.visibility = 'hidden';
    r.hiddenLayer.visibility = 'hidden';
    r.contentLayer.mainlayer = l;
    r.hiddenLayer.mainlayer = l;
    r.contentLayer.kind = 'tx';
    r.hiddenLayer.kind = 'tx';
    r.contentLayer.document.write('<PRE>' + htutil_encode(txt) + '</PRE> ');
    r.contentLayer.document.close();
    r.changed = 1;
    l.rows.splice(index,0,r);
    for(i=index;i<l.rows.length;i++)
        {
        if (l.rows[i] != null)
            {
            l.rows[i].contentLayer.moveTo(1,i*tx_metric.charHeight+1);
            l.rows[i].hiddenLayer.moveTo(1,i*tx_metric.charHeight+1);
            }
        }
    }

/** Deletes an editable row from the textarea **/
function tx_deleteRow(l, index)
    {
    l.rows[index].contentLayer.visibility = 'hidden';
    l.rows[index].hiddenLayer.visibility = 'hidden';
    l.rows.splice(index,1);
    for(i=index;i<l.rows.length;i++)
        {
        if (l.rows[i] != null)
            {
            l.rows[i].contentLayer.moveTo(1, i*tx_metric.charHeight);
            l.rows[i].hiddenLayer.moveTo(1, i*tx_metric.charHeight);
            }
        }
    }

/** Updates an existing row's contents **/
function tx_updateRow(l, index, txt)
    {
    if (index > l.rows.length) index = l.rows.length-1;
    r = l.rows[index];
    r.hiddenLayer.document.write('<PRE>' + htutil_encode(txt) + '</PRE> ');
    r.hiddenLayer.document.close();
    tmp = r.contentLayer;
    r.contentLayer = r.hiddenLayer;
    r.hiddenLayer = tmp;
    r.content = txt;
    r.changed = 1;
    }

/** Wraps words from beginning of row to the end of the above row (recursive) **/
function tx_wordWrapUp(l,index,txt,c)
    {
    if (!l.rows[index+1] || l.rows[index+1].newLine)
        {
        if (!txt && !l.rows[index].newLine && index>0)
            {
//            if (index == 0) return c;
//            if (!l.rows[index+1]) return c;
//            if (index == 0 && l.rows[index+1] && l.rows[index+1].newLine) return c;
            tx_deleteRow(l,index);
            }
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
        for (var i=l.rowCharLimit-1; txt[i]!=' ' && i>=0; i--){}
        var sub = txt.substr(0,i+1);
        var add = txt.substr(i+1); 
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
    if (tx_current.form) tx_current.form.DataNotify(tx_current);
    if (k >= 32 && k < 127)
        {
        txt = l.rows[l.cursorRow].content;
        if(txt.length == l.rowCharLimit && k!=32)			// This is a work-around for a bug:
            {								// if you try inserting a character other
            for(var i=0;i<txt.length;i++) if(txt[i] == ' ') break;	// than a space when a row is full and
            if(i==txt.length) return false;				// contains no spaces, it will go into an infinite
            }								// loop and crash netscape... needs fixing.
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
        else if (l.cursorCol == txt.length-1 && txt[txt.length-1] == ' ' && l.rows[l.cursorRow+1] && l.rows[l.cursorRow+1].content[0] != ' ' && !l.rows[l.cursorRow+1].newLine)
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
            tx_getCursorPos(l,0,0);
            }
        }
    else return true;
    for(i=0;i<l.rows.length;i++)
        {
        if (l.rows[i].changed == 1)
            {
            l.rows[i].hiddenLayer.visibility = 'hidden';
            l.rows[i].contentLayer.visibility = 'inherit';
            l.rows[i].changed = 0;
            }
        }
    tx_ibeam.moveToAbsolute(l.rows[l.cursorRow].contentLayer.pageX + l.cursorCol*tx_metric.charWidth, l.rows[l.cursorRow].contentLayer.pageY);
    tx_ibeam.visibility = 'inherit';
    return false;
    }

/** Set focus to a new textarea **/
function tx_select(x,y,l,c,n)
    {
    if (l.form) l.form.FocusNotify(l);
    if (l.enabled != 'full') return 0;
    l.cursorRow = Math.floor(y/tx_metric.charHeight);
    l.cursorCol = Math.round(x/tx_metric.charWidth);
    if (l.cursorRow >= l.rows.length)
        {
            l.cursorRow = l.rows.length - 1;
            l.cursorCol = l.rows[l.cursorRow].content.length;
        }
    else if (l.cursorCol > l.rows[l.cursorRow].content.length) l.cursorCol = l.rows[l.cursorRow].content.length;
    l.cursorPos = tx_setCursorPos(l,l.cursorRow,l.cursorCol);
    l.cursorlayer = tx_ibeam;
    tx_current = l;
    tx_ibeam.visibility = 'hidden';
    tx_ibeam.moveAbove(l);
    tx_ibeam.moveToAbsolute(l.rows[0].contentLayer.pageX + l.cursorCol*tx_metric.charWidth, l.rows[0].contentLayer.pageY + l.cursorRow*tx_metric.charHeight);
    tx_ibeam.zIndex = l.zIndex + 2;
    tx_ibeam.visibility = 'inherit';
    return 1;
    }

/** Take focus away from textarea **/
function tx_deselect()
    {
    tx_ibeam.visibility = 'hidden';
    if (tx_current) tx_current.cursorlayer = null;
    tx_current = null;
    return true;
    }

/** Textarea initializer **/
function tx_init(l,fieldname,is_readonly,main_bg)
    {
    if (!main_bg) l.bg = "bgcolor='#c0c0c0'";
    else l.bg = main_bg;
    l.kind = 'textarea';
    l.fieldname = fieldname;
    if (!tx_ibeam || !tx_metric)
        {
        tx_metric = new Layer(24);
        tx_metric.visibility = 'hidden';
        tx_metric.document.write('<pre>xx</pre>');
        tx_metric.document.close();
        w2 = tx_metric.clip.width;
        h1 = tx_metric.clip.height;
        tx_metric.document.write('<pre>x\nx</pre>');
        tx_metric.document.close();
        tx_metric.charHeight = tx_metric.clip.height - h1;
        tx_metric.charWidth = w2 - tx_metric.clip.width;
        tx_ibeam = new Layer(1);
        tx_ibeam.visibility = 'hidden';
        tx_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');
        tx_ibeam.document.close();
        tx_ibeam.resizeTo(1,tx_metric.charHeight);
        }
    l.rowCharLimit = Math.floor((l.clip.width-2)/tx_metric.charWidth);
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
    if (is_readonly)
        {
        l.enablemodify = tx_disable;
        l.enabled = 'disable';
        }
    else
        {
        l.enablemodify = tx_enable;
        l.enabled = 'full';
        }
    l.isFormStatusWidget = false;
    pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'tbox', 'tbox', 1);
    if (fm_current) fm_current.Register(l);
    l.form = fm_current;
    l.changed = false;
    return l;
    }
