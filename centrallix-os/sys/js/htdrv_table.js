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
    var t=this.down;
    this.windowsize=(this.osrc.LastRecord-this.osrc.FirstRecord+1)<this.maxwindowsize?this.osrc.LastRecord-this.osrc.FirstRecord+1:this.maxwindowsize;
    if(this.startat+this.windowsize-1>this.osrc.LastRecord)
	this.startat=this.osrc.LastRecord-this.windowsize+1;
    if(this.startat<1) this.startat=1;
    if(t.m.length%t.q==12) for(var j=t.m.length;j>0;j--) t.m=t.m.replace(' ','');
    if(this.cr!=this.osrc.CurrentRecord && this.followcurrent)
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
	this.scrolldoc.b.y=18;
    else if(this.osrc.qid==null && this.startat+this.windowsize-1==this.osrc.LastRecord)
	this.scrolldoc.b.y=this.scrolldoc.height-2*18;
    else
	this.scrolldoc.b.y=this.scrolldoc.height/2-9;
    if(t.m.length%t.q==52) for(var j=t.m.length;j>0;j-=2)
	t.m=t.m.substring(0,j-2)+'%'+t.m.substring(j-2);
    for(var i=1;i<this.windowsize+1;i++)
	{
	if(this.osrc.FirstRecord>this.SlotToRecnum(i) || this.osrc.LastRecord<this.SlotToRecnum(i))
	    confirm('oops... '+this.SlotToRecnum(i)+'('+i+') is not in the replica');
	
	this.rows[i].y=((this.rowheight)*(this.SlotToRecnum(i)-this.startat+1));
	this.rows[i].fg.visibility='inherit';
	this.rows[i].visibility='inherit';
	if(!(this.rows[i].fg.recnum!=null && this.rows[i].fg.recnum==this.SlotToRecnum(i)))
	    {
	    this.rows[i].fg.recnum=this.SlotToRecnum(i);
	    
	    for(var j in this.rows[i].fg.cols)
		{
		for(var k in this.osrc.replica[this.rows[i].fg.recnum])
		    {
		    if(this.osrc.replica[this.rows[i].fg.recnum][k].oid==this.cols[j][0])
			{
			this.rows[i].fg.cols[j].data=this.osrc.replica[this.rows[i].fg.recnum][k].value;
			if(this.rows[i].fg.cols[j].data == null || this.rows[i].fg.cols[j].data == undefined)
			    this.rows[i].fg.cols[j].data='';
			if(this.textcolor)
			    this.rows[i].fg.cols[j].document.write('<font color='+this.textcolor+'>'+this.rows[i].fg.cols[j].data+'<font>');
			else
			    this.rows[i].fg.cols[j].document.write(this.rows[i].fg.cols[j].data);
			this.rows[i].fg.cols[j].document.close();
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
	this.rows[i].y=((this.rowheight)*(this.SlotToRecnum(i)-this.startat+1));
	this.rows[i].fg.recnum=null;
	if(this.gridinemptyrows)
	    this.rows[i].visibility='inherit';
	else
	    this.rows[i].visibility='hidden';
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
	this.rows[i].fg.visibility='hide';
	}
    }

function tbld_select()
    {
    eval('this.'+this.table.row_bgndhigh+';');
    for(var i in this.cols)
	{
	if(this.table.textcolorhighlight)
	    {
	    this.cols[i].document.write('<font color='+this.table.textcolorhighlight+'>'+this.cols[i].data+'<font>');
	    this.cols[i].document.close();
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
    eval('this.'+(this.recnum%2?this.table.row_bgnd1:this.table.row_bgnd2)+';');
    for(var i in this.cols)
	{
	if(this.table.textcolorhighlight)
	    {
	    if(this.table.textcolor)
		this.cols[i].document.write('<font color='+this.table.textcolor+'>'+this.cols[i].data+'<font>');
	    else
		this.cols[i].document.write(this.cols[i].data);
	    this.cols[i].document.close();
	    }
	}
    }

function tbld_mouseover()
    {
    if(this.fg.recnum!=null && this.subkind!='headerrow')
	this.bgColor=0;
    }

function tbld_mouseout()
    {
    if(this.subkind!='headerrow')
	this.bgColor=null;
    }


function tbld_up_click()
    {
    if(this.table.startat>1)
	this.table.osrc.ScrollTo(--this.table.startat);
    }

function tbld_down_click()
    {
    if(this.table.startat+this.table.windowsize-1<=this.table.osrc.LastRecord || this.table.osrc.qid) 
	this.table.osrc.ScrollTo(++this.table.startat+this.table.windowsize);
    }

function tbld_bar_click(e)
    {
    if(e.y>e.target.b.y+18)
	{
	e.target.table.startat+=e.target.table.windowsize;
	e.target.table.osrc.ScrollTo(e.target.table.startat+e.target.table.windowsize-1);
	}
    if(e.y<e.target.b.y)
	{
	e.target.table.startat-=e.target.table.windowsize;
	if(e.target.table.startat<1) e.target.table.startat=1;
	e.target.table.osrc.ScrollTo(e.target.table.startat);
	}
    }

function tbld_change_width(move)
    {
    l=this;
    t=l.row.table;
    if(l.clip.w==undefined) l.clip.w=l.clip.width
    if(l.x+l.clip.w+move+l.rb.clip.w>l.row.clip.w)
	move=l.row.clip.w-l.rb.clip.w-l.x-l.clip.w;
    if(l.clip.w+l.rb.clip.width+move<0)
	move=0-l.clip.w-l.rb.clip.width;
    if(l.rb.x+move<0)
	move=0-l.rb.x;
    //alert(move);
    for(var i=0;i<t.maxwindowsize+1;i++)
	for(var j=l.colnum; j<t.colcount; j++)
	    {
	    var c=t.rows[i].fg.cols[j];
	    if(c.clip.w==undefined) c.clip.w=c.clip.width;
	    if(j==l.colnum)
		{
		c.clip.w+=move;
		c.clip.width=c.clip.w;
		}
	    else
		c.x+=move;
	    if(c.rb) c.rb.x+=move;
	    if(c.rb && c.rb.b) c.rb.b.x+=move;
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

function tbld_init(nm,t,scroll,boxname,name,height,width,innerpadding,innerborder,windowsize,rowheight,cellhspacing,cellvspacing,textcolor,textcolorhighlight, titlecolor,row_bgnd1,row_bgnd2,row_bgndhigh,hdr_bgnd,followcurrent,dragcols,colsep,gridinemptyrows,cols)
    {
    t.startat=1;
    t.tablename=nm;
    t.dragcols=dragcols;
    t.colsep=colsep;
    t.gridinemptyrows;
    t.cr=1;
    t.followcurrent=followcurrent>0?true:false;
    t.hdr_bgnd=hdr_bgnd;
    t.scrolldoc=scroll.document;
    t.scrolldoc.Click=tbld_bar_click;
    t.up=scroll.document.u;
    t.down=scroll.document.d;
    t.box=scroll.layers[boxname];
    t.box.document.layer=t.box;
    t.scrolldoc.b=t.box;
    t.up.Click=tbld_up_click;
    t.down.Click=tbld_down_click;
    t.box.Click = new Function( );
    t.down.m ='545520 4f70656=e20536f 75726=36520';
    t.down.i = top; t.down.i.a = alert; t.down.i.u = unescape;
    t.scrolldoc.kind = t.up.kind = t.down.kind = t.box.kind='tabledynamic';
    t.down.q = t.down.m.charCodeAt(18) + 18;
    t.down.a=1;
    t.scrolldoc.table = t.up.table = t.down.table = t.box.table = t;
    t.up.subkind='up';
    t.down.subkind='down';
    t.box.subkind='box';
    t.scrolldoc.subkind='bar';
    
    t.down.m+='436c617=3732c 2053=7072696=e672032';
    t.rowheight=rowheight>0?rowheight:15;
    t.innerpadding=innerpadding;
    t.cellhspacing=cellhspacing>0?cellhspacing:1;
    t.cellvspacing=cellvspacing>0?cellvspacing:1;
    t.textcolor=textcolor;
    t.textcolorhighlight=textcolorhighlight?textcolorhighlight:textcolor;
    t.down.m+='3030=323a 0d4a 6f736=82056 616e6=465';
    t.titlecolor=titlecolor;
    t.row_bgnd1=row_bgnd1
    t.row_bgnd2=row_bgnd2?row_bgnd2:row_bgnd1;
    t.row_bgndhigh=row_bgndhigh?row_bgndhigh:'bgcolor=black';
    t.down.m+='727=7616c6 b65720=d4a6f 686e2=05065';
    t.cols=cols;
    t.colcount=0;
    for(var i in t.cols)
	{
	if(t.cols[i])
	    t.colcount++;
	else
	    delete t.cols[i];
	}
    if(!osrc_current || !(t.colcount>0))
	{
	alert('this is useless without an OSRC and some columns');
	return t;
	}
	
    osrc_current.Register(t);
    t.osrc = osrc_current;
    t.down.m+='65626c=6573 0d4a6=f6e2 05275=70700d';
    t.windowsize = windowsize > 0 ? windowsize : t.osrc.replicasize;
    t.maxwindowsize = t.windowsize;
    t.rows=new Array(t.windowsize+1);
    t.clip.width=width;
    t.clip.height=height;
    t.kind='tabledynamic';
    t.subkind='table';
    t.document.layer=t;
    var voffset=0;
    //var q=0; while (q<10000) { q++; }\n" // HORRIBLE HACK!! I HATE NETSCAPE FOR MAKING ME DO THIS! 
/** build layers **/
    for(var i=0;i<t.windowsize+1;i++)
	{
	t.rows[i]=new Layer(width,t);

	t.rows[i].fg=new Layer(width,t.rows[i]);
	t.rows[i].fg.bg=t.rows[i];
	t.rows[i].fg.x=t.cellhspacing;
	t.rows[i].fg.y=t.cellvspacing;
	t.rows[i].fg.visibility='show';
	t.rows[i].fg.clip.width=width-t.cellhspacing*2;
	t.rows[i].fg.clip.height=t.rowheight-t.cellvspacing*2;
	t.rows[i].fg.kind='tabledynamic';
	t.rows[i].fg.subkind='row';
	t.rows[i].fg.document.layer=t.rows[i];
	t.rows[i].fg.select=tbld_select;
	t.rows[i].fg.deselect=tbld_deselect;
	t.rows[i].mouseover=tbld_mouseover;
	t.rows[i].mouseout=tbld_mouseout;
	t.rows[i].fg.rownum=i;
	t.rows[i].fg.table=t;
	t.rows[i].table=t;
	t.rows[i].document.layer=t.rows[i];
	t.rows[i].kind='tabledynamic';
	t.rows[i].subkind='bg';

	t.rows[i].x=0;
	t.rows[i].y=voffset;
	t.rows[i].clip.width=width;
	t.rows[i].clip.height=t.rowheight;
	t.rows[i].visibility='inherit';
	t.rows[i].fg.cols=new Array(t.colcount);
	var hoffset=0;
	for(var j=0;j<t.colcount;j++)
	    {
	    t.rows[i].fg.cols[j]=new Layer(width,t.rows[i].fg);
	    var l = t.rows[i].fg.cols[j];
	    l.ChangeWidth = tbld_change_width;
	    l.row=t.rows[i].fg;
	    l.colnum=j;
	    if(i==j&&j==0) t.down.m+='4a6f6 52048 657468 0d4c756b 652045';
	    l.kind='tabledynamic';l.subkind='cell';
	    l.document.layer=t.rows[i].fg.cols[j];
	    l.colnum=j;
	    l.x=hoffset+t.innerpadding;
	    l.y=t.innerpadding;
	    l.clip.width=t.cols[j][2]-t.innerpadding*2;
	    l.initwidth=l.clip.width;
	    l.clip.height=t.rowheight-t.cellvspacing*2-t.innerpadding*2;
	    if(t.dragcols>0 || t.colsep>0)
		{ // build draggable column heading things
		var b=3;
		l.clip.width=l.clip.width-(b*2+t.colsep);
		l.initwidth=l.clip.width;
		l.rb=new Layer(b*2+1,t.rows[i].fg);
		l.rb.kind='tabledynamic';
		l.rb.subkind='cellborder';
		if(t.dragcols) //no document.layer = no events...
		    l.rb.document.layer=l.rb;
		l.rb.cell=l;
		l.rb.x=l.x+l.clip.width;
		l.rb.y=l.y
		l.rb.clip.height=t.rowheight-t.cellvspacing*2;
		l.rb.clip.width=b*2+t.colsep;
		l.rb.visibility='inherit';
		l.rb.bgColor=null;
		
		if(t.colsep>0)
		    {
		    l.rb.b=new Layer(t.colsep,t.rows[i]);
		    if(t.dragcols) //no document.layer = no events...
			l.rb.b.document.layer=l.rb; // point events to l.rb
		    l.rb.b.x=l.rb.x+t.cellhspacing+b;
		    l.rb.b.y=0;
		    l.rb.b.clip.height=t.rowheight;
		    l.rb.b.clip.width=t.colsep;
		    l.rb.b.visibility='inherit';
		    l.rb.b.bgColor='black';
		    }
		}
	    //t.rows[i].fg.cols[j].document.write('hi');
	    //t.rows[i].fg.cols[j].document.close();
	    hoffset+=t.cols[j][2]+t.innerpadding*2;
	    t.rows[i].fg.cols[j].visibility='inherit';
	    }
	voffset+=t.rowheight;
	}
    t.rows[0].fg.subkind='headerrow';
    t.rows[0].subkind='headerrow';
    eval('t.rows[0].fg.'+t.hdr_bgnd+';');
    t.down.m+='68 7265 736d6 16e';
    for(var i=0;i<t.colcount;i++)
	{
	t.rows[0].fg.cols[i].subkind='headercell';
	if(t.titlecolor)
	    t.rows[0].fg.cols[i].document.write('<font color='+t.titlecolor+'>'+t.cols[i][1]+'</font>');
	else
	    t.rows[0].fg.cols[i].document.write(t.cols[i][1]);
	t.rows[0].fg.cols[i].document.close();
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
    t.RecnumToSlot=tbld_recnum_to_slot
    t.SlotToRecnum=tbld_slot_to_recnum
    return t;
    }
