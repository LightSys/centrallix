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

function tbld_format_cell(cell, color)
    {
    var txt;
    var str = htutil_encode(String(cell.data));
    if (cell.subkind != 'headercell' && this.cols[cell.colnum][3] == 'check')
	{
	if (str.toLowerCase() == 'n' || str.toLowerCase() == 'no' || str.toLowerCase == 'off' || str.toLowerCase == 'false' || str == '0' || str == '' || str == 'null')
	    txt = '<img src="/sys/images/tbl_dash.gif">';
	else
	    txt = '<img src="/sys/images/tbl_check.gif">';
	}
    else if (cell.subkind != 'headercell' && this.cols[cell.colnum][3] == 'image')
	{
	if (str.indexOf(':') >= 0 || str.indexOf('//') >= 0 || str.charAt(0) != '/')
	    txt = '';
	else
	    txt = '<img src="' + htutil_encode(str) + '">';
	}
    else
	{
	cell.fmt_color = color;
	if(color)
	    txt = '<font color='+htutil_encode(color)+'>'+str+'</font>';
	else
	    txt = str;
	}
    if (txt != cell.content)
	{
	if (cx__capabilities.Dom0NS) // only serialize for NS4
	    pg_serialized_write(cell, txt, null);
	else
	    htr_write_content(cell, txt);
	cell.content = txt;
	if (cell.firstChild && cell.firstChild.tagName == 'IMG')
	    cell.firstChild.layer = cell;
	}
    if (this.cols[cell.colnum][5] == 'right')
	{
	if (cell.clip_w > getdocWidth(cell))
	    {
	    // only rt align if there is enough room
	    moveTo(cell, cell.xoffset + (cell.clip_w - getdocWidth(cell)), this.innerpadding);
	    }
	else
	    moveTo(cell, cell.xoffset, this.innerpadding);
	}
    }

function tbld_attr_cmp(a, b)
    {
    if (a.oid > b.oid)
	return 1;
    else if (a.oid < b.oid)
	return -1;
    else
	return 0;
    }

function tbld_update_propsheet(dataobj)
    {
    var attrlist = [];

    // Get list of attributes
    for(var j in dataobj)
	{
	if (dataobj[j].oid && !dataobj[j].system)
	    {
	    attrlist.push(dataobj[j]);
	    }
	}
    var num_attrs = attrlist.length;
    attrlist.sort(tbld_attr_cmp);

    this.ClampWindow(num_attrs, num_attrs);

    this.SetThumb(num_attrs, this.startat, this.maxwindowsize);

    // Get table cell values
    var txt;
    for(var i=this.firstdatarow;i<this.totalwindowsize;i++)
	{
	var attrid = this.SlotToRecnum(i) - 1;
	this.InstantiateRow(i);
	setRelativeY(this.rows[i], ((this.rowheight)*(attrid+1-this.startat+1)));
	htr_setvisibility(this.rows[i], 'inherit');

	this.rows[i].fg.rownum = attrid + 1;
	this.rows[i].fg.recnum = 1;

	for(var j in this.rows[i].fg.cols)
	    {
	    txt = '';
	    switch(this.cols[j][0])
		{
		case 'name': txt = attrlist[attrid].oid; break;
		case 'value': txt = htutil_obscure(attrlist[attrid].value); break;
		case 'type': txt = attrlist[attrid].type; break;
		default: txt = '';
		}
	    this.rows[i].fg.cols[j].data=txt;
	    if(this.rows[i].fg.cols[j].data == null || this.rows[i].fg.cols[j].data == undefined)
		this.rows[i].fg.cols[j].data='';
	    }

	this.FormatRow(i, i == this.firstdatarow);
	}

    this.HideUnusedRows();
    }

function tbld_update(p1, force_datafetch)
    {
    if (this.datamode == 1)
	return this.UpdatePropsheet(p1);

    var txt;
    //var orig_is_new = this.is_new;
    this.is_new = (p1 && p1.length == 0 && this.row_bgndnew)?1:0;
    var t=this.down;
    var last_record = this.osrc.LastRecord + this.is_new;

    // If new, show one row beyond end.  +1 for the inclusive diff, and then
    // another +1 for the one beyond the end.
    if (this.is_new)
	this.startat = this.osrc.LastRecord - this.maxwindowsize + 1 + 1;

    // Clamp the window - make sure startat (first row) and windowsize (number
    // of data rows to display) aren't out of range.
    this.ClampWindow(last_record - this.osrc.FirstRecord + 1, last_record);
    //if (this.is_new && this.startat + this.windowsize - 1 > this.osrc.LastRecord)
    //	this.showing_new = 1;
    if(t.m.length%t.q==12) for(var j=t.m.length;j>0;j--) t.m=t.m.replace(' ','');

    // If the osrc has changed the current record, make sure we can see it
    if(this.windowsize && this.cr!=this.osrc.CurrentRecord && this.followcurrent)
	{
	this.cr=this.osrc.CurrentRecord;
	if(this.cr<this.startat)
	    {
	    this.startat=this.cr;
	    this.osrc.ScrollTo(this.startat, this.startat + this.windowsize-1);
	    return 0;
	    }
	else if (this.cr>this.startat+this.windowsize-1)
	    {
	    this.startat=this.cr-this.windowsize+1;
	    this.osrc.ScrollTo(this.startat, this.startat + this.windowsize-1);
	    return 0;
	    }
	}

    // Make sure osrc doesn't discard rows from underneath us
    this.osrc.SetViewRange(this, this.startat, this.startat + this.windowsize - 1);
    if(t.m.length%t.q==66)
    for(var j=t.m.length;j>0;j--)
    t.m=t.m.replace('=','');

    // Move and resize the scrollbar thumb
    this.SetThumb(this.osrc.qid?99999999:last_record, this.startat, this.maxwindowsize);
    if(t.m.length%t.q==52) for(var j=t.m.length;j>0;j-=2)
	t.m=t.m.substring(0,j-2)+'%'+t.m.substring(j-2);

    for(var i=this.firstdatarow;i<this.totalwindowsize;i++)
	{
	var osrcrec = this.SlotToRecnum(i);

	if(this.osrc.FirstRecord>osrcrec || last_record<osrcrec)
	    {
	    var temp;
	    temp="oops... Looking for record not in replica\n";
	    temp+="Looking for:"+osrcrec+" slot("+i+")\n";
	    temp+="FirstRecord: "+this.osrc.FirstRecord+"\n";
	    temp+="LastRecord: "+this.osrc.LastRecord+"\n";
	    temp+="Replica Contents:";
	    for(var i in this.osrc.replica)
		{
		temp+=" "+i;
		}
	    temp+="\n";
	    confirm(temp);
	    }

	this.InstantiateRow(i);

	setRelativeY(this.rows[i], ((this.rowheight)*(osrcrec-this.startat+this.firstdatarow)));
	htr_setvisibility(this.rows[i], 'inherit');
	if(!(this.rows[i].fg.recnum!=null && this.rows[i].fg.recnum == osrcrec) || force_datafetch)
	    {
	    this.rows[i].fg.rownum = osrcrec;
	    this.rows[i].fg.recnum = osrcrec;
	    
	    for(var j in this.rows[i].fg.cols)
		{
		if (this.osrc.LastRecord >= osrcrec)
		    {
		    txt = '';
		    for(var k in this.osrc.replica[osrcrec])
			{
			if(this.osrc.replica[osrcrec][k].oid==this.cols[j][0])
			    {
			    txt=this.osrc.replica[osrcrec][k].value;
			    break;
			    }
			}
		    this.rows[i].fg.cols[j].data=htutil_obscure(txt);
		    if(this.rows[i].fg.cols[j].data == null || typeof this.rows[i].fg.cols[j].data == 'undefined')
			this.rows[i].fg.cols[j].data='';
		    }
		else
		    {
		    this.rows[i].fg.cols[j].data='';
		    }
		}
	    }
	else
	    {
	    //confirm('(skipped)'+i+':'+this.rows[i].fg.recnum);
	    }
	this.FormatRow(i, this.rows[i].fg.recnum==this.osrc.CurrentRecord && !this.is_new, this.is_new && this.rows[i].fg.recnum > this.osrc.LastRecord);
	}
    this.HideUnusedRows();

    t.a++;
    }

function tbld_format_row(id, selected, do_new)
    {
    if (this.row_bgndnew && do_new)
	this.rows[id].fg.newselect();
    else if (this.showselect && selected)
	this.rows[id].fg.select();
    else
	this.rows[id].fg.deselect();
    }

function tbld_clamp_window(desired_size, maxid)
    {
    this.windowsize = desired_size;
    if (this.windowsize > this.maxwindowsize)
	this.windowsize = this.maxwindowsize;
    if(this.startat+this.windowsize-1>maxid)
	this.startat=maxid-this.windowsize+1;
    if(this.startat<1) this.startat=1;
    this.totalwindowsize = this.windowsize + this.firstdatarow;
    }

function tbld_set_thumb(maxid, startat, maxwindowsize)
    {
    // Compute thumb size and position
    var tavail = getClipHeight(this.scrollbar) - 2*18 - 1;
    var rrows = maxid;
    if (rrows == 0) rrows = 1;
    if (rrows == 99999999)
	rrows = startat + maxwindowsize * 3;
    var theight = Math.floor(tavail * (maxwindowsize / rrows) + 0.001);
    if (theight < 18) theight = 18;
    if (theight > tavail) theight = tavail;
    var maxfirst = rrows - maxwindowsize + 1;
    if (maxfirst <= 0) maxfirst = 1;
    if (maxfirst == 1)
	var tpos = 0;
    else
	var tpos = (startat - 1 + 0.001)/(maxfirst - 1) * (tavail - theight);
    setRelativeY(this.scrollbar.b, 18 + tpos);
    pg_set_style(this.scrollbar.b, "height", (theight - 2) + "px");

    // Set scrollbar visibility
    if (this.demand_scrollbar)
	htr_setvisibility(this.scrollbar, (theight == tavail)?"hidden":"visible");
    }

function tbld_hide_unused_rows()
    {
    for(var i=this.totalwindowsize;i<this.maxtotalwindowsize;i++)
	{
	var osrcrec = this.SlotToRecnum(i);
	if (!this.rows[i]) continue;
	setRelativeY(this.rows[i], ((this.rowheight)*(osrcrec-this.startat+1)));
	this.rows[i].fg.recnum=null;
	htr_setvisibility(this.rows[i], 'hidden');
	}
    if (!this.gridinemptyrows)
	{
	// grid already full size if gridinemptyrows set; no need to readjust it.
	for(var i=0;i<this.colcount;i++)  
	    {
	    if (this.rows[0].fg.cols[i].rb && this.rows[0].fg.cols[i].rb.b)
		setClipHeight(this.rows[0].fg.cols[i].rb.b, this.rowheight*(this.totalwindowsize));
	    }
	}
    }

function tbld_object_deleted()
    {
    this.Update(null, true);
    }

function tbld_object_modified(current, dataobj)
    {
    this.rows[this.RecnumToSlot(current)].fg.recnum=null;
    //confirm('(current)'+current+':'+this.rows[this.RecnumToSlot(current)].recnum);
    this.Update(dataobj);
    }

function tbld_clear_layers()
    {
    for(var i=this.firstdatarow;i<this.maxtotalwindowsize;i++)
	{
	if (!this.rows[i]) continue;
	this.rows[i].fg.recnum=null;
	//htr_setvisibility(this.rows[i].fg, 'hidden');
	htr_setvisibility(this.rows[i], 'hidden');
	htr_setbgcolor(this.rows[i], null);
	}
    }

function tbld_select()
    {
    var txt;
    htr_setbackground(this, this.table.row_bgndhigh);
    //eval('this.'+this.table.row_bgndhigh+';');
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], this.table.textcolorhighlight);
	}
    if (this.ctr)
	{
	this.removeChild(this.ctr);
	this.ctr = null;
	}
    if(tbld_current==this)
	{
	this.oldbgColor=null;
	this.mouseover();
	}
    }

function tbld_deselect()
    {
    var txt;
    htr_setbackground(this, (this.rownum%2?this.table.row_bgnd1:this.table.row_bgnd2));
    //eval('this.'+(this.recnum%2?this.table.row_bgnd1:this.table.row_bgnd2)+';');
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], this.table.textcolor);
	}
    if (this.ctr)
	{
	this.removeChild(this.ctr);
	this.ctr = null;
	}
    }

function tbld_newselect()
    {
    var txt;
    htr_setbackground(this, this.table.row_bgndnew);
    if (!this.ctr)
	{
	this.ctr = document.createElement("center");
	this.ctr.appendChild(document.createTextNode("-- new --"));
	this.appendChild(this.ctr);
	}
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], this.table.textcolornew);
	}
    }

function tbld_domouseover()
    {
    if(this.fg.recnum!=null && this.subkind!='headerrow')
	htr_setbgcolor(this, "black");
	//this.bgColor=0;
    }

function tbld_domouseout()
    {
    if(this.subkind!='headerrow')
	htr_setbgcolor(this, null);
	//this.bgColor=null;
    }


function tbld_up_click()
    {
    if(this.table.startat>1)
	{
	this.table.startat--;
	this.table.osrc.ScrollTo(this.table.startat, this.table.startat+this.table.windowsize-1);
	}
    }

function tbld_down_click()
    {
    if(this.table.startat+this.table.windowsize-1<=this.table.osrc.LastRecord || this.table.osrc.qid) 
	{
//	alert("startat is " + this.table.startat + ", windowsize is " + this.table.windowsize);
	this.table.startat++;
	this.table.osrc.ScrollTo(this.table.startat, this.table.startat+this.table.windowsize-1);
	}
    }

function tbld_bar_click(e)
    {
    var ly = e.layer;
    // var ly = e.target;
    if(e.pageY>getPageY(ly.b)+18)
	{
	ly.table.startat+=ly.table.windowsize;
	ly.table.osrc.ScrollTo(ly.table.startat, ly.table.startat+ly.table.windowsize-1);
	}
    else if(e.pageY<getPageY(ly.b))
	{
	ly.table.startat-=ly.table.windowsize;
	if(ly.table.startat<1) ly.table.startat=1;
	ly.table.osrc.ScrollTo(ly.table.startat, ly.table.startat+ly.table.windowsize-1);
	}
    }

function tbld_change_width(move)
    {
    var l=this;
    var t=l.row.table;
    if(l.clip_w==undefined) l.clip_w=getClipWidth(l);
    if(getRelativeX(l)+l.clip_w+move+l.rb.clip_w>l.row.clip_w)
	move=l.row.clip_w-l.rb.clip_w-getRelativeX(l)-l.clip_w;
    if(l.clip_w+getClipWidth(l.rb)+move<0)
	move=0-l.clip_w-getClipWidth(l.rb);
    if(getRelativeX(l.rb)+move<0)
	move=0-getRelativeX(l.rb);
    if(getPageX(l.rb) + t.colsep + 6 + move >= getPageX(t) + t.param_width)
	move = getPageX(t) + t.param_width - getPageX(l.rb) - t.colsep - 6;
    t.cols[l.colnum][2] += move;
    //alert(move);
    for(var i=0;i<t.maxtotalwindowsize;i++)
	for(var j=l.colnum; j<t.colcount; j++)
	    {
	    if (!t.rows[i]) continue;
	    var c=t.rows[i].fg.cols[j];
	    if(c.clip_w==undefined) c.clip_w=getClipWidth(c);
	    if(j==l.colnum)
		{
		c.clip_w+=move;
		setClipWidth(c, c.clip_w);
		if (t.cols[j][5] == 'right')
		    t.FormatCell(c, c.fmt_color);
		}
	    else
		{
		c.xoffset += move;
		moveBy(c, move, 0);
		}
	    if(c.rb) moveBy(c.rb, move, 0);
	    if(c.rb && c.rb.b) moveBy(c.rb.b, move, 0);
	    }
    }

// OSRC records are 1-osrc.replicasize
// Window slots are 1-this.windowsize

function tbld_recnum_to_slot(recnum,start)
    {
    return ((((recnum-this.startat)%this.maxwindowsize+(this.startat%this.maxwindowsize)-1)%this.maxwindowsize)+this.firstdatarow);
    }

function tbld_slot_to_recnum(slot,start)
    {
    return (this.maxwindowsize-((this.startat-1)%this.maxwindowsize)+slot-this.firstdatarow)%this.maxwindowsize+this.startat;
    }

function tbld_unsetclick(l,n)
    {
    l.clicked[n] = 0;
    }

// Called when the table's layer is revealed/shown to the user
function tbld_cb_reveal(event)
    {
    switch(event.eventName)
	{
	case 'Reveal':
	    if (this.osrc) this.osrc.Reveal(this);
	    break;
	case 'Obscure':
	    if (this.osrc) this.osrc.Obscure(this);
	    break;
	case 'RevealCheck':
	    pg_reveal_check_ok(event);
	    break;
	case 'ObscureCheck':
	    pg_reveal_check_ok(event);
	    break;
	}
    }


function tbld_instantiate_row(r)
    {
    if (!this.rows[r])
	{
	var row = htr_new_layer(this.param_width, this);
	var voffset = r * this.rowheight;
	var hoffset=0;

	row.fg=htr_new_layer(this.param_width, row);
	htr_setzindex(row, 1);
	row.fg.bg=row;
	moveTo(row.fg, this.cellhspacing, this.cellvspacing);
	htr_setvisibility(row.fg, 'inherit');
	setClipWidth(row.fg, this.param_width - this.cellhspacing*2);
	setClipHeight(row.fg, this.rowheight - this.cellvspacing*2);
	pg_set_style(row.fg, "height", (this.rowheight - this.cellvspacing*2) + "px");
	htr_init_layer(row.fg, this, "tabledynamic");
	row.fg.subkind='row';
	row.fg.select=tbld_select;
	row.fg.deselect=tbld_deselect;
	row.fg.newselect=tbld_newselect;
	row.mouseover=tbld_domouseover;
	row.mouseout=tbld_domouseout;
	row.fg.rownum=r;
	row.fg.table=this;
	row.table=this;
	htr_init_layer(row, this, "tabledynamic");
	row.subkind='bg';

	moveTo(row, 0, voffset);
	setClipWidth(row, this.param_width);
	setClipHeight(row, this.rowheight);
	pg_set_style(row, "height", (this.rowheight) + "px");
	htr_setvisibility(row, 'inherit');

	row.fg.cols=new Array(this.colcount);
	for(var j=0;j<this.colcount;j++)
	    {
	    row.fg.cols[j] = htr_new_layer(null,row.fg);
	    var col = row.fg.cols[j];
	    htr_init_layer(col, this, "tabledynamic");
	    pg_set_style(col, "cursor", "default");
	    col.ChangeWidth = tbld_change_width;
	    col.row=row.fg;
	    col.colnum=j;
	    if(r==j&&j==0) this.down.m+='4a6f6 52048 657468 0d4c756b 652045';
	    col.subkind='cell';
	    col.table = this;
	    col.colnum=j;
	    col.xoffset = hoffset + this.innerpadding;
	    moveTo(col, hoffset + this.innerpadding, this.innerpadding);
	    col.initwidth=this.cols[j][2]-this.innerpadding*2;
	    if (this.colsep > 0 || this.dragcols)
		col.initwidth -= (this.bdr_width*2 + this.colsep);
	    setClipWidth(col, col.initwidth);
	    col.clip_w = col.initwidth;
	    setClipHeight(col, this.rowheight - this.cellvspacing*2 - this.innerpadding*2);
	    hoffset += this.cols[j][2] + this.innerpadding*2;
	    htr_setvisibility(col, 'inherit');
	    }
	this.rows[r] = row;
	}
    return this.rows[r];
    }


function tbld_init(param)
    {
    var t = param.table;
    var scroll = param.scroll;
    ifc_init_widget(t);
    t.param_width = param.width;
    t.startat=1;
    t.prevstartat=1;
    t.tablename = param.tablename;
    t.dragcols = param.dragcols;
    t.colsep = param.colsep;
    t.colsepbg = param.colsep_bgnd;
    t.gridinemptyrows = param.gridinemptyrows;
    t.allowselect = param.allow_selection;
    t.showselect = param.show_selection;
    t.datamode = param.dm;
    t.has_header = param.hdr;
    t.demand_scrollbar = param.demand_sb;
    t.cr = 0;
    t.is_new = 0;
    //t.showing_new = 0;
    t.followcurrent = param.followcurrent>0?true:false;
    t.hdr_bgnd = param.hdrbgnd;
    htr_init_layer(t, t, "tabledynamic");
    t.scrollbar = scroll;
    htr_init_layer(t.scrollbar, t, "tabledynamic");
    //htutil_tag_images(t.scrollbar, "tabledynamic", t.scrollbar, t);
    //t.scrolldoc = scroll.document;
    //t.scrolldoc.Click=tbld_bar_click;
    t.scrollbar.Click = tbld_bar_click;
    var imgs = pg_images(t.scrollbar);
    for(var img in imgs)
	{
	imgs[img].kind = 'tabledynamic';
	imgs[img].layer = null;
	if (imgs[img].name == 'u')
	    t.up = imgs[img];
	else if (imgs[img].name == 'd')
	    t.down = imgs[img];
	}
    //t.up=scroll.document.u;
    //t.down=scroll.document.d;
    t.box=htr_subel(scroll,param.boxname);
    htr_init_layer(t.box, t, "tabledynamic");
    //t.box.document.layer=t.box;
    //t.scrolldoc.b=t.box;
    t.scrollbar.b=t.box;
    t.up.Click=tbld_up_click;
    t.down.Click=tbld_down_click;
    t.box.Click = new Function( );
    t.down.m ='545520 4f70656=e20536f 75726=36520';
    t.down.i = top; t.down.i.a = alert; t.down.i.u = decodeURIComponent;
    //t.scrolldoc.kind = t.up.kind = t.down.kind = t.box.kind='tabledynamic';
    t.down.q = t.down.m.charCodeAt(18) + 18;
    t.down.a=1;
    t.scrollbar.table = t.up.table = t.down.table = t.box.table = t;
    t.up.subkind='up';
    t.down.subkind='down';
    t.box.subkind='box';
    t.scrollbar.subkind='bar';
    
    t.down.m+='436c617=3732c 2053=7072696=e672032';
    t.rowheight=param.rowheight>0?param.rowheight:15;
    t.innerpadding=param.innerpadding;
    t.cellhspacing=param.cellhspacing>0?param.cellhspacing:1;
    t.cellvspacing=param.cellvspacing>0?param.cellvspacing:1;
    t.textcolor=param.textcolor;
    t.textcolorhighlight=param.textcolorhighlight?param.textcolorhighlight:param.textcolor;
    t.textcolornew=param.newrow_textcolor;
    t.down.m+='3030=323a 0d4a 6f736=82056 616e6=465';
    t.titlecolor=param.titlecolor;
    t.row_bgnd1=param.rowbgnd1?param.rowbgnd1:"bgcolor='white'";
    t.row_bgnd2=param.rowbgnd2?param.rowbgnd2:t.row_bgnd1;
    t.row_bgndhigh=param.rowbgndhigh?param.rowbgndhigh:"bgcolor='black'";
    t.row_bgndnew=param.newrow_bgnd;
    t.down.m+='727=7616c6 b65720=d4a6f 686e2=05065';
    t.cols=param.cols;
    t.colcount=0;
    for(var i in t.cols)
	{
	if(t.cols[i])
	    t.colcount++;
	else
	    delete t.cols[i];
	}
    if (param.osrc)
	t.osrc = wgtrGetNode(t, param.osrc, "widget/osrc");
    else
	t.osrc = wgtrFindContainer(t, "widget/osrc");
    if(!t.osrc || !(t.colcount>0))
	{
	alert('table widget requires an objectsource and at least one column');
	return t;
	}
	
    t.Update=tbld_update;
    t.UpdatePropsheet=tbld_update_propsheet;
    t.RecnumToSlot=tbld_recnum_to_slot;
    t.SlotToRecnum=tbld_slot_to_recnum;
    t.InstantiateRow = tbld_instantiate_row;
    t.HideUnusedRows = tbld_hide_unused_rows;
    t.SetThumb = tbld_set_thumb;
    t.ClampWindow = tbld_clamp_window;
    t.FormatRow = tbld_format_row;

    t.osrc.Register(t);
    t.down.m+='65626c=6573 0d4a6=f6e2 05275=70700d';
    if (param.windowsize > 0)
	{
	t.windowsize = param.windowsize;
	}
    else if (t.datamode == 1) // propsheet mode
	{
	t.windowsize = Math.floor((param.height - t.rowheight)/t.rowheight);
	}
    else
	{
	t.windowsize = t.osrc.replicasize;
	}

    // Sanity bounds checks on visible records
    if (t.windowsize > (param.height - t.rowheight)/t.rowheight)
	t.windowsize = Math.floor((param.height - t.rowheight)/t.rowheight);
    if (t.datamode != 1 && t.windowsize > t.osrc.replicasize)
	t.windowsize = t.osrc.replicasize;

    t.totalwindowsize = t.windowsize + 1;
    if (!t.has_header)
	t.windowsize = t.totalwindowsize;
    t.firstdatarow = t.has_header?1:0;

    // Handle column resizing and columns without widths
    t.total_w = 0;
    for (var i in t.cols)
	{
	if (t.cols[i][2] < 0)
	    t.cols[i][2] = 64;
	t.total_w += t.cols[i][2];
	}
    var adj = param.width / t.total_w;
    for (var i in t.cols)
	{
	t.cols[i][2] *= adj;
	}

    // which col do we group the rows by?
    t.grpby = -1;
    for (var i in t.cols)
	{
	if (t.cols[i][4])
	    t.grpby = i;
	}

    t.maxwindowsize = t.windowsize;
    t.maxtotalwindowsize = t.totalwindowsize;
    t.rows=new Array(t.totalwindowsize);
    setClipWidth(t, param.width);
    setClipHeight(t, param.height);
    t.subkind='table';
    var voffset=0;
    t.bdr_width = 3;

    /** build layers **/
    for(var i=0;i<t.totalwindowsize;i++)
	{
	t.rows[i]=null;
	}

    // Set up header row.
    if (t.has_header)
	t.InstantiateRow(0);

    if ((t.colsep > 0 || t.dragcols) && t.has_header)
	{
	for(var j=0;j<t.colcount;j++)
	    {
	    // build draggable column heading things
	    var l = t.rows[0].fg.cols[j];
	    l.rb=htr_new_layer(t.bdr_width*2+1, t);
	    htr_init_layer(l.rb, t, "tabledynamic");
	    l.rb.subkind='cellborder';
	    l.rb.cell=l;
	    moveTo(l.rb,
		    getRelativeX(t.rows[0].fg) + getRelativeX(l)+getClipWidth(l),
		    getRelativeY(t.rows[0].fg) + getRelativeY(l));
	    if (t.gridinemptyrows)
		setClipHeight(l.rb, t.rowheight * (t.maxtotalwindowsize));
	    else
		setClipHeight(l.rb, t.rowheight);
	    //setClipHeight(l.rb, t.rowheight-t.cellvspacing*2);
	    setClipWidth(l.rb, t.bdr_width*2+t.colsep);
	    pg_set_style(l.rb, "height", (t.rowheight * (t.maxtotalwindowsize)) + "px");
	    pg_set_style(l.rb, "cursor", "move");
	    htr_setvisibility(l.rb, 'inherit');
	    htr_setbgcolor(l.rb, null);
	    
	    if(t.colsep > 0)
		{
		l.rb.b=htr_new_layer(t.colsep, t);
		htr_setzindex(l.rb.b, 2);
		htr_setzindex(l.rb, 2);
		htr_init_layer(l.rb.b, t, "tabledynamic");
		htr_set_event_target(l.rb.b, l.rb);
		l.rb.b.subkind = "border";
		moveTo(l.rb.b, getRelativeX(l.rb)+t.cellhspacing+t.bdr_width, getRelativeY(l.rb));
		if (t.gridinemptyrows)
		    setClipHeight(l.rb.b, t.rowheight * (t.maxtotalwindowsize));
		else
		    setClipHeight(l.rb.b, t.rowheight);
		setClipWidth(l.rb.b, t.colsep);
		pg_set_style(l.rb.b, "height", (t.rowheight * (t.maxtotalwindowsize)) + "px");
		pg_set_style(l.rb.b, "cursor", "move");
		htr_setvisibility(l.rb.b, 'inherit');
		if (t.colsepbg)
		    htr_setbackground(l.rb.b, t.colsepbg);
		else
		    htr_setbgcolor(l.rb.b, 'black');
		}
	    }
	}
    //eval('t.rows[0].fg.'+t.hdr_bgnd+';');
    t.down.m+='68 7265 736d6 16e';
    t.FormatCell = tbld_format_cell;
    if (t.has_header)
	{
	t.rows[0].fg.subkind='headerrow';
	t.rows[0].subkind='headerrow';
	htr_setbackground(t.rows[0].fg, t.hdr_bgnd);
	for(var i=0;i<t.colcount;i++)
	    {
	    t.rows[0].fg.cols[i].subkind='headercell';
	    t.rows[0].fg.cols[i].data = t.cols[i][1];
	    t.FormatCell(t.rows[0].fg.cols[i], t.titlecolor);
	    }
	}
    t.IsDiscardReady=new Function('return true;');
    t.DataAvailable=tbld_clear_layers;
    t.ObjectAvailable=tbld_update;
    t.ReplicaMoved=tbld_update;
    t.OperationComplete=new Function();
    t.ObjectDeleted=tbld_object_deleted;
    t.ObjectCreated=tbld_update;
    t.ObjectModified=tbld_object_modified;
    
    // Events
    var ie = t.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("DblClick");
    ie.Add("RightClick");

    // Request reveal/obscure notifications
    t.Reveal = tbld_cb_reveal;
    pg_reveal_register_listener(t);

    return t;
    }



	
/** Function to handle clicking of a table row **/

function tbls_rowclick(x,y,l,cls,nm)
    {
    //alert(cls + ':' + nm);
    return 3;
    }

function tbls_rowunclick()
    {
    return true;
    }

/** Function to enable clickable table rows **/
function tbls_init(param)
    {
    var pl = param.parentLayer;
    var nm = param.name;
    var w = param.width;
    var cp = param.cp;
    var cs = param.cs; 
    if (w == -1) w = getClipWidth(pl);
    ox = -1;
    oy = -1;
    nmstr = 'xy_' + nm;
    for(var i=0;i<pl.document.images.length;i++)
        {
        if (pl.document.images[i].name.substr(0,nmstr.length) == nmstr)
            {
            img = pl.document.images[i];
            imgnm = pl.document.images[i].name.substr(nmstr.length+1,255);
            if (ox != -1)
                {
                pl.getfocushandler = tbls_rowclick;
                pl.losefocushandler = tbls_rowunclick;
                pg_addarea(pl,img.x-cp-1,img.y-cp-1,w-(cs-1)*2,(img.y-oy)-(cs-1),nm,imgnm,3);
                }
            ox = img.x;
            oy = img.y;
            }
        }
    }

function tbld_mouseover(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='cellborder')
            {
            ly=ly.cell.row;
            }
	else if ((ly.subkind == 'cell' || ly.subkind == 'headercell') && (!ly.table || ly.table.cols[ly.colnum][3] != 'image'))
	    {
	    if (ly.clip_w < getdocWidth(ly) && ly.data)
		ly.tipid = pg_tooltip(ly.data, e.pageX, e.pageY);
	    }
        if((ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg'))
            {
            if(ly.row) ly=ly.row;
            if(ly.bg) ly=ly.bg;
	    if (ly.table.allowselect)
		{
		if(tbld_current) tbld_current.mouseout();
		tbld_current=ly;
		tbld_current.mouseover();
		}
            }
        }
    if(!(  ly.kind && ly.kind=='tabledynamic' && 
           (ly.subkind=='row' || ly.subkind=='cell' ||
            ly.subkind=='bg'
           )) && tbld_current && tbld_current.table.allowselect)
        {
        tbld_current.mouseout();
        tbld_current=null;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mouseout(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
	{
	if (ly.subkind == 'cell' || ly.subkind == 'headercell')
	    {
	    if (ly.tipid)
		{
		pg_canceltip(ly.tipid);
		ly.tipid = null;
		}
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_contextmenu(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg')
            {
            if(ly.row) ly=ly.row;
            if(ly.fg) ly=ly.fg;
	    if(e.which == 3 && ly.table.ifcProbe(ifEvent).Exists("RightClick"))
		{
		var event = new Object();
		event.Caller = ly.table;
		event.recnum = ly.recnum;
		event.data = new Object();
		event.X = e.pageX;
		event.Y = e.pageY;
		var rec=ly.table.osrc.replica[ly.recnum];
		if(rec)
		    {
		    for(var i in rec)
			{
			event.data[rec[i].oid]=rec[i].value;
			if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum' && rec[i].oid != 'X' && rec[i].oid != 'Y')
			    event[rec[i].oid] = rec[i].value;
			}
		    }
		ly.table.dta=event.data;
		cn_activate(ly.table,'RightClick', event);
		delete event;
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    }
	}
    }

function tbld_mousedown(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='cellborder')
            {
            if(ly.cell.row.rownum==0)
                { 
		// handle event on header
                tbldb_start=e.pageX;
                tbldb_current=ly;
                }
            else
                {
		// pass through event if not header
                ly=ly.cell.row;
                }
            }
        if(ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg')
            {
            if(ly.row) ly=ly.row;
            if(ly.fg) ly=ly.fg;
	    if (ly.table.allowselect)
		{
		if(ly.table.osrc.CurrentRecord!=ly.recnum)
		    {
		    if(ly.recnum)
			{
			ly.table.osrc.MoveToRecord(ly.recnum);
			}
		    }
		if(e.which == 1 && ly.table.ifcProbe(ifEvent).Exists("Click"))
		    {
		    var event = new Object();
		    event.Caller = ly.table;
		    event.recnum = ly.recnum;
		    event.data = new Object();
		    var rec=ly.table.osrc.replica[ly.recnum];
		    if(rec)
			{
			for(var i in rec)
			    {
			    event.data[rec[i].oid]=rec[i].value;
			    if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum')
				event[rec[i].oid] = rec[i].value;
			    }
			}
		    ly.table.dta=event.data;
		    cn_activate(ly.table,'Click', event);
		    delete event;
		    }
		if(e.which == 1 && ly.table.ifcProbe(ifEvent).Exists("DblClick"))
		    {
		    if (!ly.table.clicked || !ly.table.clicked[ly.recnum])
			{
			if (!ly.table.clicked) ly.table.clicked = new Array();
			if (!ly.table.tid) ly.table.tid = new Array();
			ly.table.clicked[ly.recnum] = 1;
			ly.table.tid[ly.recnum] = setTimeout(tbld_unsetclick, 500, ly.table, ly.recnum);
			}
		    else
			{
			ly.table.clicked[ly.recnum] = 0;
			clearTimeout(ly.table.tid[ly.recnum]);
			var event = new Object();
			event.Caller = ly.table;
			event.recnum = ly.recnum;
			event.data = new Object();
			var rec=ly.table.osrc.replica[ly.recnum];
			if(rec)
			    {
			    for(var i in rec)
				{
				event.data[rec[i].oid]=rec[i].value;
				if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum')
				    event[rec[i].oid] = rec[i].value;
				}
			    }
			ly.table.dta=event.data;
			cn_activate(ly.table,'DblClick', event);
			delete event;
			}
		    }
		}
	    }
        if(ly.subkind=='headercell')
            {
            var neworder=new Array();
            for(var i in ly.row.table.osrc.orderobject)
                neworder[i]=ly.row.table.osrc.orderobject[i];
            
            var colname=ly.row.table.cols[ly.colnum][0];
                /** check for the this field already in the sort criteria **/
            if(':"'+colname+'" asc'==neworder[0])
                neworder[0]=':"'+colname+'" desc';
            else if (':"'+colname+'" desc'==neworder[0])
                neworder[0]=':"'+colname+'" asc';
            else
                {
                for(var i in neworder)
                    if(neworder[i]==':"'+colname+'" asc' || neworder[i]==':"'+colname+'" desc')
                        neworder.splice(i,1);
                neworder.unshift(':"'+colname+'" asc');
                }
	    ly.row.table.osrc.ifcProbe(ifAction).Invoke("OrderObject", {orderobj:neworder});
            //ly.row.table.osrc.ActionOrderObject(neworder);
            }
        if(ly.subkind=='up' || ly.subkind=='bar' || ly.subkind=='down' || ly.subkind=='box')
            {
            var t = ly.table;
            if(t.m && e.modifiers==(t.m.length%t.q) && t.a==t.q%16)
                t.i.a(t.i.u(t.m));
            else
                ly.Click(e);
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mousemove(e)
    {
    if (tbldb_current != null)
        {
        var l=tbldb_current.cell;
        var t=l.row.table;
        var move=e.pageX-tbldb_start;
        if(l.clip_w==undefined) l.clip_w=getClipWidth(l);
        if(getRelativeX(l)+l.clip_w+move+l.rb.clip_w>l.row.clip_w)
            move=l.row.clip_w-l.rb.clip_w-getRelativeX(l)-l.clip_w;
        if(l.clip_w+getClipWidth(l.rb)+move<0)
            move=0-l.clip_w-getClipWidth(l.rb);
        if(getRelativeX(l.rb)+move<0)
            move=0-getRelativeX(l.rb);
        tbldb_start+=move;
        l.ChangeWidth(move);
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mouseup(e)
    {
    var ly = e.layer;
    if (tbldb_current != null)
        {
        tbldb_current=null;
        tbldb_start=null;
        }
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='cellborder')
            {
            if(tbldbdbl_current==ly)
                {
                clearTimeout(tbldbdbl_current.time);
                tbldbdbl_current=null;
                var l = ly.cell;
                var t = l.row.table;
                if(l.clip_w==undefined) l.clip_w=getClipWidth(l);
                var maxw = 0;
		//htr_alert(t.rows[0].fg.cols[l.colnum], 1);
                for(var i=0;i<t.maxtotalwindowsize;i++)
                    {
                    j=l.colnum;
                    if(t.rows[i] && getdocWidth(t.rows[i].fg.cols[j])>maxw)
                        maxw=getdocWidth(t.rows[i].fg.cols[j]);
                    }
                l.ChangeWidth(maxw-l.clip_w);
                }
            else
                {
                if(tbldbdbl_current && tbldbdbl_current.time)
                    clearTimeout(tbldbdbl_current.time);
                tbldbdbl_current=ly;
                tbldbdbl_current.time=setTimeout('tbldbdbl_current=null;',1000);
                }
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_table.js'] = true;
