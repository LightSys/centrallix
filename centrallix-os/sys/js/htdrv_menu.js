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

// Form manipulation

function mn_getvalue() 
    {
    return this.Values[this.VisLayer.index][1];
    }

function mn_setvalue(v) 
    {
    for (i=0; i < this.Values.length; i++)
	{
	if (this.Values[i][1] == v)
	    {
	    mn_select_item(this, i);
	    return true;
	    }
	}
    return false;
}

function mn_clearvalue()
    {
    mn_select_item(this, null);
    }

function mn_resetvalue()
    {
    this.clearvalue();
    }

function mn_enable()
    {
    //mrc-arrow//this.document.images[4].src = '/sys/images/ico15b.gif';
    this.keyhandler = mn_keyhandler;
    this.enabled = 'full';
    }

function mn_readonly()
    {
    this.document.images[4].src = '/sys/images/ico15b.gif';
    this.keyhandler = null;
    this.enabled = 'readonly';
    }

function mn_disable()
    {
    if (mn_current)
	{
	mn_current.PaneLayer.visibility = 'hide';
	mn_current = null;
	}
    this.document.images[4].src = '/sys/images/ico15a.gif';
    this.keyhandler = null;
    this.enabled = 'disabled';
    }

// Normal functions

function mn_keyhandler(l,e,k)
    {
    if (!mn_current) return;
    if (mn_current.enabled != 'full') return 1;
    if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122))
	{
	if (k < 97) 
	    {
	    k_lower = k + 32;
	    k_upper = k;
	    k = k + 32;
	    }
	else
	    {
	    k_lower = k;
	    k_upper = k - 32;
	    }
	if (!mn_lastkey || mn_lastkey != k)
	    {
	    for (i=0; i < this.Values.length; i++)
		{
		if (this.Values[i][0].substring(0, 1) == String.fromCharCode(k_upper) ||
		    this.Values[i][0].substring(0, 1) == String.fromCharCode(k_lower))
		    {
		    mn_hilight_item(this,i);
		    i=this.Values.length;
		    }
		}
	    }
	else
	    {
	    var first = -1;
	    var last = -1;
	    var next = -1;
	    for (i=0; i < this.Values.length; i++)
		{
		if (this.Values[i][0].substring(0, 1) == String.fromCharCode(k_upper) ||
		    this.Values[i][0].substring(0, 1) == String.fromCharCode(k_lower))
		    {
		    if (first < 0) { first = i; last = i; }
		    for (var j=i; j < this.Values.length && 
			(this.Values[j][0].substring(0, 1) == String.fromCharCode(k_upper) ||
			 this.Values[j][0].substring(0, 1) == String.fromCharCode(k_lower)); j++)
			{
			if (this.Items[j] == this.Items[this.SelectedItem])
			    next = j + 1;
			last = j;
			}
		    if (next <= last)
			mn_hilight_item(this,next);
		    else
			mn_hilight_item(this,first);
		    i=this.Values.length;
		    }
		}
	    }
	}
    else if (k == 13 && mn_lastkey != 13)
	{
	mn_select_item(this,this.SelectedItem);
	mn_unhilight_item(this,this.SelectedItem);
	}
    mn_lastkey = k;
    return false;
    }

function mn_show_menu(l,i)
    {
    if (l.SelectedItem != -1)
        {
  	l.Items[l.SelectedItem].HidLayer.visibility = 'hide';
	l.SelectedItem = -1;
	}
    l.Items[i].HidLayer.visibility = 'show';
    l.SelectedItem = i;
    }

function mn_toggle_item(l,i)
    {
    if (l.SelectedItem != null)
	mn_untoggle_item(l,l.SelectedItem);
    l.SelectedItem = i;
    //l.Items[i].bgColor=l.hl;
    mn_toggle(l,i)
    mn_scroll_to(l,i);
    }

function mn_untoggle_item(l,i)
    {
    l.SelectedItem = null;
    l.Items[i].bgColor=l.bg;
    }


function mn_hilight_item(l,i)
    {
        if (l.SelectedItem != null)
	    mn_unhilight_item(l,l.SelectedItem);
        l.SelectedItem = i;
        l.Items[i].bgColor=l.hl;
        mn_scroll_to(l,i);
    }

function mn_unhilight_item(l,i)
    {
    l.SelectedItem = null;
    l.Items[i].bgColor=l.bg;
    }

function mn_select_item(l,i)
    {
    l.PaneLayer.visibility = 'hide';
    mn_current = null;
    }

function mn_getfocus()
    {
    cn_activate(this, "GetFocus");
    return 0;
    }

function mn_losefocus()
    {
    cn_activate(this, "LoseFocus");
    return true;
    }

function mn_toggle(l,i) 
    {
    if (i) {
        for (x=l.Items[i].x; x<i+l.Values[i][2];x++) {
	    //if (x == 4)
	        //continue;
	    if (l.document.images[x].src.substr(-14, 6) == 'dkgrey')
	        l.document.images[x].src = '/sys/images/white_1x1.png';
	    else
	        l.document.images[x].src = '/sys/images/dkgrey_1x1.png';
        }
    }
    for (x=0; x<l.document.images.length;x++) {
	if (x == 4)
	    continue;
	else if (l.document.images[x].src.substr(-14, 6) == 'dkgrey')
	    l.document.images[x].src = '/sys/images/white_1x1.png';
	else
	    l.document.images[x].src = '/sys/images/dkgrey_1x1.png';
	}
    }

function mn_scroll_to(l, n)
    {
    var top=mn_current.PaneLayer.ScrLayer.clip.top;
    var btm=top+(mn_current.PaneLayer.clip.height-4);
    var il=l.Items[n];

    if (il.y>=top && il.y+16<=btm) //none
	return;
    else if (il.y<top) //up
	{
	mn_target_img=l.PaneLayer.BarLayer.document.images[0];
	mn_incr = (top-il.y);
	}
    else //down
	{
	mn_target_img=l.PaneLayer.BarLayer.document.images[2];
	//mrc-scrlbr//mn_incr = (top-il.y+(16*(mn_current.NumDisplay-1)));
	mn_incr = (top-il.y+(16*(mn_current.NumElements-1)));
	}
    mn_scroll();
    }

function mn_scroll_tm()
    {
    mn_scroll();
    mn_timeout=setTimeout(mn_scroll_tm,50);
    return false;
    }

function mn_scroll(t)
    {
    var ti=mn_target_img;
    var px=mn_incr;
    var ly=mn_current.PaneLayer.ScrLayer;
    var ht1=ly.y-2;
    var ht2=mn_current.PaneLayer.h-ly.clip.height+ht1;
    var h=mn_current.PaneLayer.h;
    var d=h-mn_current.PaneLayer.clip.height+4;
    var v=mn_current.PaneLayer.clip.height-(3*18)-4;
    if (ht1+px>0) px = -ht1;
    if (ht2+px<0) px = -ht2;
    if (px<0 && ht2>0) // down
	{
	ly.y += px;
	ly.clip.height -= px;
	ly.clip.top -= px;
	if (t==null)
	    {
	    if (d<=0) ti.thum.y=18;
	    else ti.thum.y=20+(-v*((ly.y-2)/d));
	    }
	}
    else if (px>0 && ht1<0) // up
	{
	ly.y += px;
	ly.clip.height -= px;
	ly.clip.top -= px;
	if (t==null)
	    {
	    if (d<=0) ti.thum.y=18;
	    else ti.thum.y=20+(-v*((ly.y-2)/d));
	    }
	}
    }

function mn_create_toppane(l)
    {
    //p = new Layer(1024,l);
    //p.kind = 'mn_pn';
    //p.visibility = 'inherit';
    //p.document.layer = p;
    //p.mainlayer = l;


    /**  Add items  **/
    var w=0;
    for (var i=0; i < l.Values.length; i++)
	{
	if (!l.Items[i])
	    {
	    l.Items[i] = new Layer(1024, l);
	    l.Items[i].kind = 'mn_itm';
            }
 	l.Items[i].visibility = 'inherit';
	l.Items[i].document.layer = l.Items[i];
	l.Items[i].mainlayer = l;

	// Create the hidden layer than shows the menu items pushed in...
	if (!l.Items[i].HidLayer)
	    {
	    l.Items[i].HidLayer = new Layer(1024, l);
	    l.Items[i].HidLayer.kind = 'mn_itm';
	    }

	// Create the visual layer that shows all the menu items...
	if (!l.Items[i].VisLayer)
	    {
	    //l.Items[i].VisLayer = new Layer(1024, l.Items[i]); //this does not fill in the menu items...
	    l.Items[i].VisLayer = new Layer(1024, l);
	    l.Items[i].VisLayer.kind = 'mn_itm';
	    }

 	l.Items[i].HidLayer.visibility = 'hide';
 	l.Items[i].VisLayer.visibility = 'inherit';

	l.Items[i].HidLayer.document.layer = l.Items[i].HidLayer;
	l.Items[i].VisLayer.document.layer = l.Items[i].VisLayer;

	l.Items[i].HidLayer.mainlayer = l
	l.Items[i].VisLayer.mainlayer = l


	//Good code that fills in the first menu items...
	l.Items[i].VisLayer.document.write("<TABLE border=0 cellpadding=0 cellspacing=0 width="+(l.Values[i][2]-1)+" height="+(l.h-1)+">");
	l.Items[i].VisLayer.document.write("<TR><TD></TD>");
	l.Items[i].VisLayer.document.write("    <TD height=1 width="+(l.Values[i][2]-2)+"></TD>");
	l.Items[i].VisLayer.document.write("    <TD></TD></TR>");
	l.Items[i].VisLayer.document.write("<TR><TD height="+(l.h-2)+" width=1></TD>");
	l.Items[i].VisLayer.document.write("    <TD>"+l.Values[i][0]+"</TD>");
	l.Items[i].VisLayer.document.write("    <TD height="+(l.h-2)+" width=1></TD></TR>");
	l.Items[i].VisLayer.document.write("<TR><TD></TD>");
	l.Items[i].VisLayer.document.write("    <TD height=1 width="+(l.Values[i][2]-2)+"></TD>");
	l.Items[i].VisLayer.document.write("    <TD></TD></TR>");
	l.Items[i].VisLayer.document.write("</TABLE>");
	l.Items[i].VisLayer.document.close();

 	// Good code for button pushed in...
 	l.Items[i].HidLayer.document.write("<TABLE border=0 cellpadding=0 cellspacing=0 width="+(l.Values[i][2]-1)+" height="+(l.h-1)+">");
 	l.Items[i].HidLayer.document.write("<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>");
 	l.Items[i].HidLayer.document.write("    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.Values[i][2]-2)+"></TD>");
 	l.Items[i].HidLayer.document.write("    <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>");
 	l.Items[i].HidLayer.document.write("<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h-2)+" width=1></TD>");
 	l.Items[i].HidLayer.document.write("    <TD>"+l.Values[i][0]+"</TD>");
 	l.Items[i].HidLayer.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h-2)+" width=1></TD></TR>");
 	l.Items[i].HidLayer.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>");
 	l.Items[i].HidLayer.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.Values[i][2]-2)+"></TD>");
 	l.Items[i].HidLayer.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>");
 	l.Items[i].HidLayer.document.write("</TABLE>");
 	l.Items[i].HidLayer.document.close();


 	//htutil_tag_images(l.Items[i].HidLayer.document,'mn_pn',l.Items[i],l);
 	//htutil_tag_images(l.Items[i].VisLayer.document,'mn_pn',l.Items[i],l);

	l.Items[i].HidLayer.x = w;
	l.Items[i].VisLayer.x = w;

	// calculate the width to determine the next menu items position...
 	w+=l.Values[i][2];

	l.Items[i].HidLayer.y = 0;
	l.Items[i].VisLayer.y = 0;

	l.Items[i].HidLayer.clip.height = 25;
	l.Items[i].VisLayer.clip.height = 25;

	l.Items[i].HidLayer.index = i;
	l.Items[i].VisLayer.index = i;
        }
    return l.Items;
    }


function mn_create_pane(l)
    {
    p = new Layer(1024);
    p.kind = 'dd_pn';
    p.visibility = 'hide';
    p.document.layer = p;
    p.mainlayer = l;
    p.document.write("<BODY bgcolor="+l.bg+">");
    p.document.write("<TABLE border=0 cellpadding=0 cellspacing=0 width="+l.w+" height="+l.h2+">");
    p.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.w-2)+"></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>");
    p.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h2-2)+" width=1></TD>");
    p.document.write("  <TD valign=top>");
    p.document.write("  </TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h2-2)+" width=1></TD></TR>");
    p.document.write("<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.w-2)+"></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>");
    p.document.write("</TABLE>");
    p.document.write("</BODY>");
    p.document.close();
    htutil_tag_images(p.document,'dt_pn',p,l);

    /**  Create scroll background layer  **/
    p.ScrLayer = new Layer(1024, p);
    p.ScrLayer.document.layer = p;
    p.ScrLayer.mainlayer = l;
    p.ScrLayer.x = 2; p.ScrLayer.y = 2;
    p.ScrLayer.clip.height = l.h2;
    if (l.NumDisplay < l.NumElements)
        {
        /**  If we need a scrollbar, put one in  **/
        p.ScrLayer.clip.width = p.clip.width - 22;

        p.BarLayer = new Layer(1024, p)
        p.BarLayer.kind = 'dd_sc';
        p.BarLayer.x = l.w-20; p.BarLayer.y = 2;
        p.BarLayer.visibility = 'inherit';
        p.BarLayer.mainlayer = l;
        var pd = p.BarLayer.document;
        pd.layer = p.BarLayer;
        pd.write('<TABLE border=0 cellpadding=0 cellspacing=0 width=18 height='+(l.h2-4)+'>');
        pd.write('<TR><TD><IMG name=u src=/sys/images/ico13b.gif></TD></TR>');
        pd.write('<TR><TD><IMG name=b src=/sys/images/trans_1.gif height='+(l.h2-40)+'></TD></TR>');
        pd.write('<TR><TD><IMG name=d src=/sys/images/ico12b.gif></TD></TR>');
        pd.write('</TABLE>');
        pd.close();
        pd.images[0].mainlayer = pd.images[1].mainlayer = pd.images[2].mainlayer = l;
        pd.images[0].kind = pd.images[1].kind = pd.images[2].kind = 'dd_sc';
        l.imgup = pd.images[0];
        l.imgdn = pd.images[2];

        p.TmbLayer = new Layer(1024, p);
        pd.images[0].thum = pd.images[1].thum = pd.images[2].thum = p.TmbLayer;
        p.TmbLayer.x = l.w-20; p.TmbLayer.y = 20;
        p.TmbLayer.visibility = 'inherit';
        p.TmbLayer.mainlayer = l;
        var pd = p.TmbLayer.document;
        pd.write('<IMG src=/sys/images/ico14b.gif NAME=t>');
        pd.close();
        pd.images[0].mainlayer = l;
        pd.images[0].thum = p.TmbLayer;
        pd.images[0].kind = 'dd_sc';
        l.imgtm = pd.images[0];
        }
    else
        {
        /**  If no scrollbar is needed, don't use one!  **/
        p.ScrLayer.clip.width = p.clip.width - 4;
        }
    p.ScrLayer.clip.height = p.clip.height - 4;
    p.ScrLayer.visibility = 'inherit';

    /**  Add items  **/
    for (var i=0; i < l.Values.length; i++)
        {
        if (!l.Items[i])
            {
            l.Items[i] = new Layer(1024, p.ScrLayer);
            l.Items[i].kind = 'dd_itm';
            }
        l.Items[i].mainlayer = l;
        l.Items[i].document.layer = l.Items[i];
        l.Items[i].x = 0;
        l.Items[i].y = (i*16);
        l.Items[i].clip.width = p.ScrLayer.clip.width;
        l.Items[i].clip.height = 16;
        l.Items[i].document.write(l.Values[i][0]);
        l.Items[i].document.close();
        l.Items[i].visibility = 'inherit';
        l.Items[i].index = i;
        }

    return p;

    }

function mn_add_items(l,i,ary)
    {
    l.Values = ary;
    l.NumElements = l.Values.length;
    l.h2 = ((l.NumDisplay<l.NumElements?l.NumDisplay:l.NumElements)*16)+4;
    l.PaneLayer = dd_create_pane(l);
    l.PaneLayer.h = l.NumElements*16;
    l.PaneLayer.mainlayer = l;
    }

function mn_add_top_layer_items(l,ary)
    {
    l.Values = ary;
    l.NumElements = l.Values.length;
    l.MenuLayer = mn_create_toppane(l);
    l.MenuLayer.h = l.h;
    l.MenuLayer.mainlayer = l;
    }

function mn_init(l,bg,hl,w,h)
    {
    l.Items = new Array();
    l.setvalue   = mn_setvalue;
    l.getvalue   = mn_getvalue;
    l.enable     = mn_enable;
    l.readonly   = mn_readonly;
    l.disable    = mn_disable;
    l.clearvalue = mn_clearvalue;
    l.resetvalue = mn_resetvalue;
    l.keyhandler = mn_keyhandler;
    l.losefocushandler = mn_losefocus;
    l.getfocushandler = mn_getfocus;
    l.bg = bg;
    l.hl = hl;
    l.w = w; l.h = h;
    l.enabled = 'full';
    l.form = fm_current;
    l.document.layer = l;
    l.SelectedItem = -1;
    l.mainlayer = l;
    l.kind = 'mn';
    htutil_tag_images(l.document,'mn',l,l);
    pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'mn', 'mn', 0);
    if (fm_current) fm_current.Register(l);
    return l;
    }
