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
function spnr_getvalue()
    {
    return parseFloat(this.content);
    }

/** Set value function **/
function spnr_setvalue(v,f)
    {
    if(this.form) this.form.DataNotify(this);
    spnr_settext(this,v);
    }

/** Enable control function **/
function spnr_enable()
    {
    }

/** Disable control function **/
function spnr_disable()
    {
    }

/** Readonly-mode function **/
function spnr_readonly()
    {
    }

/** Clear value function **/
function spnr_clearvalue()
    {
    }

/** Editbox set-text-value function **/
function spnr_settext(l,txt)
    {
    l.HiddenLayer.document.write('<PRE>' + txt + '</PRE> ');
    l.HiddenLayer.document.close();
    l.HiddenLayer.visibility = 'inherit';
    l.ContentLayer.visibility = 'hidden';
    tmp = l.ContentLayer;
    l.ContentLayer = l.HiddenLayer;
    l.HiddenLayer = tmp;
    if(!txt)
        txt=0;
    l.content= new String(txt);
    }

/** Editbox keyboard handler **/
function spnr_keyhandler(l,e,k)
    {
    txt = l.content;
    if (k >= 49 && k < 58)
        {
        newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);
        l.cursorCol++;
        }
    else if (k == 8 && l.cursorCol > 0)
        {
        newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length);
        l.cursorCol--;
        }
    else
        {
        return true;
        }
    spnr_ibeam.visibility = 'hidden';
    spnr_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*spnr_metric.charWidth, l.ContentLayer.pageY);
    spnr_settext(l,newtxt);
    adj = 0;
    if (spnr_ibeam.pageX < l.pageX + 1)
        adj = l.pageX + 1 - spnr_ibeam.pageX;
    else if (spnr_ibeam.pageX > l.pageX + l.clip.width - 1)
        adj = (l.pageX + l.clip.width - 1) - spnr_ibeam.pageX;
    if (adj != 0)
        {
        spnr_ibeam.pageX += adj;
        l.ContentLayer.pageX += adj;
        l.HiddenLayer.pageX += adj;
        }
    spnr_ibeam.visibility = 'inherit';
    return false;
    }

/** Set focus to a new editbox **/
function spnr_select(x,y,l,c,n)
    {
    l.cursorCol = Math.round((x + l.pageX - l.ContentLayer.pageX)/spnr_metric.charWidth);
    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;
    if (spnr_current) spnr_current.cursorlayer = null;
    spnr_current = l;
    spnr_current.cursorlayer = spnr_ibeam;
    spnr_ibeam.visibility = 'hidden';
    spnr_ibeam.moveAbove(spnr_current);
    spnr_ibeam.moveToAbsolute(spnr_current.ContentLayer.pageX + spnr_current.cursorCol*spnr_metric.charWidth, spnr_current.ContentLayer.pageY);
    spnr_ibeam.zIndex = spnr_current.zIndex + 2;
    spnr_ibeam.visibility = 'inherit';
    l.form.focusnotify(l);
    return 1;
    }

/** Take focus away from editbox **/
function spnr_deselect()
    {
    spnr_ibeam.visibility = 'hidden';
    if (spnr_current) spnr_current.cursorlayer = null;
    spnr_current = null;
    return true;
    }

/** Spinner box initializer **/
function spnr_init(param)
    {
    var main = param.main;
    var l = param.layer;
    var c1 = param.c1;
    var c2 = param.c2;
    l.content = 0;
    l.mainlayer=main;
    l.document.Layer = l;
    main.ContentLayer = c1;
    main.ContentLayer.document.write('0');
    main.ContentLayer.document.close();
    main.HiddenLayer = c2;
    main.HiddenLayer.document.write('0');
    main.HiddenLayer.document.close();
    main.form=wgtrFindContainer(main,"widget/form");
    if (!spnr_ibeam || !spnr_metric)
        {
        spnr_metric = new Layer(24);
        spnr_metric.visibility = 'hidden';
        spnr_metric.document.write('<pre>xx</pre>');
        spnr_metric.document.close();
        w2 = spnr_metric.clip.width;
        h1 = spnr_metric.clip.height;
        spnr_metric.document.write('<pre>x\\nx</pre>');
        spnr_metric.document.close();
        spnr_metric.charHeight = spnr_metric.clip.height - h1;
        spnr_metric.charWidth = w2 - spnr_metric.clip.width;
        spnr_ibeam = new Layer(1);
        spnr_ibeam.visibility = 'hidden';
        spnr_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');
        spnr_ibeam.document.close();
        spnr_ibeam.resizeTo(1,spnr_metric.charHeight);
        }
    htr_init_layer(c1,main,'spinner');
    htr_init_layer(c2,main,'spinner');
    htr_init_layer(main,main,'spinner');
    ifc_init_widget(main);
    main.layers.spnr_button_up.document.images[0].kind='spinner';
    main.layers.spnr_button_down.document.images[0].kind='spinner';
    main.layers.spnr_button_up.document.images[0].subkind='up';
    main.layers.spnr_button_down.document.images[0].subkind='down';
    main.layers.spnr_button_up.document.images[0].eb_layers=main;
    main.layers.spnr_button_down.document.images[0].eb_layers=main;
    l.keyhandler = spnr_keyhandler;
    l.getfocushandler = spnr_select;
    l.losefocushandler = spnr_deselect;
    main.getvalue = spnr_getvalue;
    main.setvalue = spnr_setvalue;
    main.setoptions = null;
    main.enable = spnr_enable;
    main.disable = spnr_disable;
    main.readonly = spnr_readonly;
    main.clearvalue = spnr_clearvalue;
    main.isFormStatusWidget = false;
    pg_addarea(main, -1,-1,main.clip.width+1,main.clip.height+1, 'spinner', 'spinner', 1);
    c1.y = ((l.clip.height - spnr_metric.charHeight)/2);
    c2.y = ((l.clip.height - spnr_metric.charHeight)/2);
    if (main.form) main.form.Register(main);
    return main;
    }

function spnr_mousedown(e)
    {
    if (e.target != null && e.target.kind == 'spinner') 
	{
	if(e.target.subkind=='up')
	    {
	    e.target.eb_layers.setvalue(e.target.eb_layers.getvalue()+1);
	    }
	if(e.target.subkind=='down')
	    {
	    e.target.eb_layers.setvalue(e.target.eb_layers.getvalue()-1);
	    }
	}
    }
