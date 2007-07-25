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

function tbld_update(p1)
    {
    var txt;
    var t=this.down;
    this.windowsize=(this.osrc.LastRecord-this.osrc.FirstRecord+1)<this.maxwindowsize?this.osrc.LastRecord-this.osrc.FirstRecord+1:this.maxwindowsize;
    if(this.startat+this.windowsize-1>this.osrc.LastRecord)
	this.startat=this.osrc.LastRecord-this.windowsize+1;
    if(this.startat<1) this.startat=1;
    if(t.m.length%t.q==12) for(var j=t.m.length;j>0;j--) t.m=t.m.replace(' ','');
    if(this.windowsize && this.cr!=this.osrc.CurrentRecord && this.followcurrent)
	{ /* the osrc has changed the current record, make sure we can see it */
	this.cr=this.osrc.CurrentRecord;
	if(this.cr<this.startat)
	    {
	    this.startat=this.cr;
	    this.osrc.ScrollTo(this.startat);
	    return 0;
	    }
	else if (this.cr>this.startat+this.windowsize-1)
	    {
	    this.startat=this.cr-this.windowsize+1;
	    this.osrc.ScrollTo(this.startat);
	    return 0;
	    }
	}
    if(t.m.length%t.q==66)
    for(var j=t.m.length;j>0;j--)
    t.m=t.m.replace('=','');
    if(this.startat==1)
	setRelativeY(this.scrollbar.b, 18);
    else if(this.osrc.qid==null && this.startat+this.windowsize-1==this.osrc.LastRecord)
	setRelativeY(this.scrollbar.b, getClipHeight(this.scrollbar)-2*18);
    else
	setRelativeY(this.scrollbar.b, getClipHeight(this.scrollbar)/2-9);
    if(t.m.length%t.q==52) for(var j=t.m.length;j>0;j-=2)
	t.m=t.m.substring(0,j-2)+'%'+t.m.substring(j-2);

    for(var i=1;i<this.windowsize+1;i++)
	{
	if(this.osrc.FirstRecord>this.SlotToRecnum(i) || this.osrc.LastRecord<this.SlotToRecnum(i))
	    {
	    var temp;
	    temp="oops... Looking for record not in replica\n";
	    temp+="Looking for:"+this.SlotToRecnum(i)+" slot("+i+")\n";
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

	setRelativeY(this.rows[i], ((this.rowheight)*(this.SlotToRecnum(i)-this.startat+1)));
	htr_setvisibility(this.rows[i].fg, 'inherit');
	htr_setvisibility(this.rows[i], 'inherit');
	if(!(this.rows[i].fg.recnum!=null && this.rows[i].fg.recnum==this.SlotToRecnum(i)))
	    {
	    this.rows[i].fg.recnum=this.SlotToRecnum(i);
	    
	    for(var j in this.rows[i].fg.cols)
		{
		for(var k in this.osrc.replica[this.rows[i].fg.recnum])
		    {
		    if(this.osrc.replica[this.rows[i].fg.recnum][k].oid==this.cols[j][0])
			{
			this.rows[i].fg.cols[j].data=htutil_obscure(this.osrc.replica[this.rows[i].fg.recnum][k].value);
			if(this.rows[i].fg.cols[j].data == null || this.rows[i].fg.cols[j].data == undefined)
			    this.rows[i].fg.cols[j].data='';
			if(this.textcolor)
			    txt = '<font color='+this.textcolor+'>'+this.rows[i].fg.cols[j].data+'<font>';
			else
			    txt = this.rows[i].fg.cols[j].data;
			if (this.rows[i].fg.cols[j].content != txt)
			    {
			    pg_serialized_write(this.rows[i].fg.cols[j], txt, null);
			    //this.rows[i].fg.cols[j].document.write(txt);
			    //this.rows[i].fg.cols[j].document.close();
			    this.rows[i].fg.cols[j].content = txt;
			    }
			}
		    }
		}
	    }
	else
	    {
	    //confirm('(skipped)'+i+':'+this.rows[i].fg.recnum);
	    }
	if(this.rows[i].fg.recnum==this.osrc.CurrentRecord)
	    this.rows[i].fg.select();
	else
	    this.rows[i].fg.deselect();
	}
    for(var i=this.windowsize+1;i<this.maxwindowsize+1;i++)
	{
	setRelativeY(this.rows[i], ((this.rowheight)*(this.SlotToRecnum(i)-this.startat+1)));
	this.rows[i].fg.recnum=null;
	//htr_setvisibility(this.rows[i], this.gridinemptyrows?'inherit':'hidden');
	htr_setvisibility(this.rows[i], 'hidden');
	}

    // adjust grid
    if (!this.gridinemptyrows)
	{
	// grid already full size if gridinemptyrows set; no need to readjust it.
	for(var i=0;i<this.colcount;i++)  
	    {
	    if (this.rows[0].fg.cols[i].rb && this.rows[0].fg.cols[i].rb.b)
		setClipHeight(this.rows[0].fg.cols[i].rb.b, this.rowheight*(this.windowsize+1));
	    }
	}
    t.a++;
    }

function tbld_object_modified(current)
    {
    this.rows[this.RecnumToSlot(current)].fg.recnum=null;
    //confirm('(current)'+current+':'+this.rows[this.RecnumToSlot(current)].recnum);
    this.Update();
    }

function tbld_clear_layers()
    {
    for(var i=1;i<this.maxwindowsize+1;i++)
	{
	this.rows[i].fg.recnum=null;
	htr_setvisibility(this.rows[i].fg, 'hidden');
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
	if(this.table.textcolorhighlight)
	    {
	    txt = '<font color='+this.table.textcolorhighlight+'>'+this.cols[i].data+'<font>';
	    if (txt != this.cols[i].content)
		{
		pg_serialized_write(this.cols[i], txt, null);
		//this.cols[i].document.write(txt);
		//this.cols[i].document.close();
		this.cols[i].content = txt;
		}
	    }
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
    htr_setbackground(this, (this.recnum%2?this.table.row_bgnd1:this.table.row_bgnd2));
    //eval('this.'+(this.recnum%2?this.table.row_bgnd1:this.table.row_bgnd2)+';');
    for(var i in this.cols)
	{
	if(this.table.textcolorhighlight)
	    {
	    if(this.table.textcolor)
		txt = '<font color='+this.table.textcolor+'>'+this.cols[i].data+'<font>';
	    else
		txt = this.cols[i].data;
	    if (txt != this.cols[i].content)
		{
		pg_serialized_write(this.cols[i], txt, null);
		//this.cols[i].document.write(txt);
		//this.cols[i].document.close();
		this.cols[i].content = txt;
		}
	    }
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
	this.table.osrc.ScrollTo(--this.table.startat);
    }

function tbld_down_click()
    {
    if(this.table.startat+this.table.windowsize-1<=this.table.osrc.LastRecord || this.table.osrc.qid) 
	{
//	alert("startat is " + this.table.startat + ", windowsize is " + this.table.windowsize);
	this.table.osrc.ScrollTo(++this.table.startat+this.table.windowsize);
	}
    }

function tbld_bar_click(e)
    {
    var ly = e.layer;
    // var ly = e.target;
    if(e.pageY>getPageY(ly.b)+18)
	{
	ly.table.startat+=ly.table.windowsize;
	ly.table.osrc.ScrollTo(ly.table.startat+ly.table.windowsize-1);
	}
    else if(e.pageY<getPageY(ly.b))
	{
	ly.table.startat-=ly.table.windowsize;
	if(ly.table.startat<1) ly.table.startat=1;
	ly.table.osrc.ScrollTo(ly.table.startat);
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
    //alert(move);
    for(var i=0;i<t.maxwindowsize+1;i++)
	for(var j=l.colnum; j<t.colcount; j++)
	    {
	    var c=t.rows[i].fg.cols[j];
	    if(c.clip_w==undefined) c.clip_w=getClipWidth(c);
	    if(j==l.colnum)
		{
		c.clip_w+=move;
		setClipWidth(c, c.clip_w);
		}
	    else
		moveBy(c, move, 0);
	    if(c.rb) moveBy(c.rb, move, 0);
	    if(c.rb && c.rb.b) moveBy(c.rb.b, move, 0);
	    }
    }

// OSRC records are 1-osrc.replicasize
// Window slots are 1-this.windowsize

function tbld_recnum_to_slot(recnum,start)
    {
    return ((((recnum-this.startat)%this.maxwindowsize+(this.startat%this.maxwindowsize)-1)%this.maxwindowsize)+1);
    }

function tbld_slot_to_recnum(slot,start)
    {
    return (this.maxwindowsize-((this.startat-1)%this.maxwindowsize)+slot-1)%this.maxwindowsize+this.startat;
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



function tbld_init(param)
    {
    var t = param.table;
    var scroll = param.scroll;
    ifc_init_widget(t);
    t.startat=1;
    t.prevstartat=1;
    t.tablename = param.tablename;
    t.dragcols = param.dragcols;
    t.colsep = param.colsep;
    t.gridinemptyrows = param.gridinemptyrows;
    t.cr = 1;
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
    t.down.i = top; t.down.i.a = alert; t.down.i.u = unescape;
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
    t.down.m+='3030=323a 0d4a 6f736=82056 616e6=465';
    t.titlecolor=param.titlecolor;
    t.row_bgnd1=param.rowbgnd1?param.rowbgnd1:"bgcolor='white'";
    t.row_bgnd2=param.rowbgnd2?param.rowbgnd2:t.row_bgnd1;
    t.row_bgndhigh=param.rowbgndhigh?param.rowbgndhigh:"bgcolor='black'";
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
    t.osrc = wgtrFindContainer(t, "widget/osrc");
    if(!t.osrc || !(t.colcount>0))
	{
	alert('this is useless without an OSRC and some columns');
	return t;
	}
	
    t.osrc.Register(t);
    t.down.m+='65626c=6573 0d4a6=f6e2 05275=70700d';
    t.windowsize = param.windowsize > 0 ? param.windowsize : t.osrc.replicasize;

    // Sanity bounds checks on visible records
    if (t.windowsize > (param.height - t.rowheight)/t.rowheight)
	t.windowsize = Math.floor((param.height - t.rowheight)/t.rowheight);
    if (t.windowsize > t.osrc.replicasize)
	t.windowsize = t.osrc.replicasize;

    t.maxwindowsize = t.windowsize;
    t.rows=new Array(t.windowsize+1);
    setClipWidth(t, param.width);
    setClipHeight(t, param.height);
    t.subkind='table';
    var voffset=0;
    var b=3;
    //var q=0; while (q<10000) { q++; }\n" // HORRIBLE HACK!! I HATE NETSCAPE FOR MAKING ME DO THIS! 
/** build layers **/
    for(var i=0;i<t.windowsize+1;i++)
	{
	t.rows[i]=htr_new_layer(param.width,t);

	t.rows[i].fg=htr_new_layer(param.width,t.rows[i]);
	t.rows[i].fg.bg=t.rows[i];
	moveTo(t.rows[i].fg, t.cellhspacing, t.cellvspacing);
	htr_setvisibility(t.rows[i].fg, 'inherit');
	setClipWidth(t.rows[i].fg, param.width-t.cellhspacing*2);
	setClipHeight(t.rows[i].fg, t.rowheight-t.cellvspacing*2);
	pg_set_style(t.rows[i].fg, "height", (t.rowheight-t.cellvspacing*2) + "px");
	htr_init_layer(t.rows[i].fg, t, "tabledynamic");
	t.rows[i].fg.subkind='row';
	//t.rows[i].fg.document.layer=t.rows[i];
	t.rows[i].fg.select=tbld_select;
	t.rows[i].fg.deselect=tbld_deselect;
	t.rows[i].mouseover=tbld_domouseover;
	t.rows[i].mouseout=tbld_domouseout;
	t.rows[i].fg.rownum=i;
	t.rows[i].fg.table=t;
	t.rows[i].table=t;
	htr_init_layer(t.rows[i], t, "tabledynamic");
	//t.rows[i].document.layer=t.rows[i];
	//t.rows[i].kind='tabledynamic';
	t.rows[i].subkind='bg';

	moveTo(t.rows[i], 0, voffset);
	setClipWidth(t.rows[i], param.width);
	setClipHeight(t.rows[i], t.rowheight);
	pg_set_style(t.rows[i], "height", (t.rowheight) + "px");
	htr_setvisibility(t.rows[i], 'inherit');
	t.rows[i].fg.cols=new Array(t.colcount);
	var hoffset=0;
	for(var j=0;j<t.colcount;j++)
	    {
	    t.rows[i].fg.cols[j]=htr_new_layer(param.width,t.rows[i].fg);
	    var l = t.rows[i].fg.cols[j];
	    htr_init_layer(l, t, "tabledynamic");
	    l.ChangeWidth = tbld_change_width;
	    l.row=t.rows[i].fg;
	    l.colnum=j;
	    if(i==j&&j==0) t.down.m+='4a6f6 52048 657468 0d4c756b 652045';
	    //l.kind='tabledynamic';
	    l.subkind='cell';
	    //l.document.layer=t.rows[i].fg.cols[j];
	    l.colnum=j;
	    moveTo(l, hoffset+t.innerpadding, t.innerpadding);
	    l.initwidth=t.cols[j][2]-t.innerpadding*2;
	    if (t.colsep > 0 || t.dragcols)
		l.initwidth -= (b*2+t.colsep);
	    setClipWidth(l, l.initwidth);
	    setClipHeight(l, t.rowheight-t.cellvspacing*2-t.innerpadding*2);
	    //setClipLeft(l, -b);
	    //t.rows[i].fg.cols[j].document.write('hi');
	    //t.rows[i].fg.cols[j].document.close();
	    hoffset+=t.cols[j][2]+t.innerpadding*2;
	    htr_setvisibility(t.rows[i].fg.cols[j], 'inherit');
	    }
	voffset+=t.rowheight;
	}
    if (t.colsep > 0 || t.dragcols)
	{
	for(var j=0;j<t.colcount;j++)
	    {
	    // build draggable column heading things
	    var l = t.rows[0].fg.cols[j];
	    l.rb=htr_new_layer(b*2+1, t);
	    htr_init_layer(l.rb, t, "tabledynamic");
	    l.rb.subkind='cellborder';
	    l.rb.cell=l;
	    moveTo(l.rb,
		    getRelativeX(t.rows[0].fg) + getRelativeX(l)+getClipWidth(l),
		    getRelativeY(t.rows[0].fg) + getRelativeY(l));
	    if (t.gridinemptyrows)
		setClipHeight(l.rb, t.rowheight * (t.maxwindowsize+1));
	    else
		setClipHeight(l.rb, t.rowheight);
	    //setClipHeight(l.rb, t.rowheight-t.cellvspacing*2);
	    setClipWidth(l.rb, b*2+t.colsep);
	    pg_set_style(l.rb, "height", (t.rowheight * (t.maxwindowsize+1)) + "px");
	    pg_set_style(l.rb, "cursor", "move");
	    htr_setvisibility(l.rb, 'inherit');
	    htr_setbgcolor(l.rb, null);
	    
	    if(t.colsep > 0)
		{
		l.rb.b=htr_new_layer(t.colsep, t);
		htr_init_layer(l.rb.b, t, "tabledynamic");
		htr_set_event_target(l.rb.b, l.rb);
		l.rb.b.subkind = "border";
		moveTo(l.rb.b, getRelativeX(l.rb)+t.cellhspacing+b, getRelativeY(l.rb));
		if (t.gridinemptyrows)
		    setClipHeight(l.rb.b, t.rowheight * (t.maxwindowsize+1));
		else
		    setClipHeight(l.rb.b, t.rowheight);
		setClipWidth(l.rb.b, t.colsep);
		pg_set_style(l.rb.b, "height", (t.rowheight * (t.maxwindowsize+1)) + "px");
		pg_set_style(l.rb.b, "cursor", "move");
		htr_setvisibility(l.rb.b, 'inherit');
		htr_setbgcolor(l.rb.b, 'black');
		}
	    }
	}
    t.rows[0].fg.subkind='headerrow';
    t.rows[0].subkind='headerrow';
    htr_setbackground(t.rows[0].fg, t.hdr_bgnd);
    //eval('t.rows[0].fg.'+t.hdr_bgnd+';');
    t.down.m+='68 7265 736d6 16e';
    for(var i=0;i<t.colcount;i++)
	{
	t.rows[0].fg.cols[i].subkind='headercell';
	if(t.titlecolor)
	    pg_serialized_write(t.rows[0].fg.cols[i], '<font color='+t.titlecolor+'>'+t.cols[i][1]+'</font>', null);
	else
	    pg_serialized_write(t.rows[0].fg.cols[i], t.cols[i][1], null);
	}
    t.IsDiscardReady=new Function('return true;');
    t.DataAvailable=tbld_clear_layers;
    t.ObjectAvailable=tbld_update;
    t.ReplicaMoved=tbld_update;
    t.OperationComplete=new Function();
    t.ObjectDeleted=tbld_update;
    t.ObjectCreated=tbld_update;
    t.ObjectModified=tbld_object_modified;
    
    t.Update=tbld_update;
    t.RecnumToSlot=tbld_recnum_to_slot;
    t.SlotToRecnum=tbld_slot_to_recnum;

    // Events
    var ie = t.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("DblClick");

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
    for(i=0;i<pl.document.images.length;i++)
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
        if(ly.subkind=='row' || ly.subkind=='cell' || ly.subkind=='bg')
            {
            if(ly.row) ly=ly.row;
            if(ly.bg) ly=ly.bg;
            if(tbld_current) tbld_current.mouseout();
            tbld_current=ly;
            tbld_current.mouseover();
            }
        }
    if(!(  ly.kind && ly.kind=='tabledynamic' && 
           (ly.subkind=='row' || ly.subkind=='cell' ||
            ly.subkind=='bg'
           )) && tbld_current)
        {
        tbld_current.mouseout();
        tbld_current=null;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
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
            if(ly.table.osrc.CurrentRecord!=ly.recnum)
                {
                if(ly.recnum)
                    {
                    ly.table.osrc.MoveToRecord(ly.recnum);
                    }
                }
            if(ly.table.ifcProbe(ifEvent).Exists("Click"))
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
                        }
                    }
                ly.table.dta=event.data;
                cn_activate(ly.table,'Click', event);
                delete event;
                }
            if(ly.table.ifcProbe(ifEvent).Exists("DblClick"))
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
                            }
                        }
                    ly.table.dta=event.data;
                    cn_activate(ly.table,'DblClick', event);
                    delete event;
                    }
                }
            }
        if(ly.subkind=='headercell')
            {
            var neworder=new Array();
            for(i in ly.row.table.osrc.orderobject)
                neworder[i]=ly.row.table.osrc.orderobject[i];
            
            var colname=ly.row.table.cols[ly.colnum][0];
                /** check for the this field already in the sort criteria **/
            if(':'+colname+' asc'==neworder[0])
                neworder[0]=':'+colname+' desc';
            else if (':'+colname+' desc'==neworder[0])
                neworder[0]=':'+colname+' asc';
            else
                {
                for(i in neworder)
                    if(neworder[i]==':'+colname+' asc' || neworder[i]==':'+colname+' desc')
                        neworder.splice(i,1);
                neworder.unshift(':'+colname+' asc');
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
                for(var i=0;i<t.maxwindowsize+1;i++)
                    {
                    j=l.colnum;
                    if(t.rows[i].fg.cols[j].document.width>maxw)
                        maxw=t.rows[i].fg.cols[j].document.width;
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
