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

function sp_init(l,aname,tname,p)
    {
    var alayer=null;
    var tlayer=null;
    for(i=0;i<l.layers.length;i++)
	{
	ml=l.layers[i];
	if(ml.name==aname) alayer=ml;
	if(ml.name==tname) tlayer=ml;
	}
    for(i=0;i<l.document.images.length;i++)
	{
	img=l.document.images[i];
	if(img.name=='d' || img.name=='u' || img.name=='b')
	    {
	    img.pane=l;
	    img.layer = img;
	    img.area=alayer;
	    img.thum=tlayer;
	    img.kind='sp';
	    img.mainlayer=l;
	    }
	}
    tlayer.document.images[0].kind='sp';
    tlayer.document.images[0].layer = tlayer.document.images[0];
    tlayer.document.images[0].mainlayer=l;
    tlayer.document.images[0].thum=tlayer;
    tlayer.document.images[0].area=alayer;
    tlayer.document.images[0].pane=l;
    alayer.clip.width=l.clip.width-18;
    alayer.maxwidth=alayer.clip.width;
    alayer.minwidth=alayer.clip.width;
    tlayer.nofocus = true;
    alayer.nofocus = true;
    alayer.document.layer = alayer;
    alayer.mainlayer = l;
    tlayer.document.layer = tlayer;
    tlayer.mainlayer = l;
    alayer.kind = 'sp';
    tlayer.kind = 'sp';
    alayer.mainlayer
    l.document.layer = l;
    l.mainlayer = l;
    l.kind = 'sp';
    l.LSParent = p;
    }

function do_mv()
    {
    var ti=sp_target_img;
    if (ti.kind=='sp' && sp_mv_incr > 0)
	{
	h=ti.area.clip.height;
	d=h-ti.pane.clip.height;
	incr=sp_mv_incr;
	if(d<0) incr=0; else if (d+ti.area.y<incr) incr=d+ti.area.y;
	for(i=0;i<ti.pane.document.layers.length;i++) if (ti.pane.document.layers[i] != ti.thum)
	    ti.pane.document.layers[i].y-=incr;
	v=ti.pane.clip.height-(3*18);
	if (d<=0) ti.thum.y=18;
	else ti.thum.y=18+v*(-ti.area.y/d);
	}
    else if (ti.kind=='sp' && sp_mv_incr < 0)
	{
	h=ti.area.clip.height;
	d=h-ti.pane.clip.height;
	incr = -sp_mv_incr;
	if(d<0)incr=0; else if (ti.area.y>-incr) incr=-ti.area.y;
	for(i=0;i<ti.pane.document.layers.length;i++) if (ti.pane.document.layers[i] != ti.thum)
	    ti.pane.document.layers[i].y+=incr;
	v=ti.pane.clip.height-(3*18);
	if(d<=0) ti.thum.y=18;
	else ti.thum.y=18+v*(-ti.area.y/d);
	}
    return true;
    }

function tm_mv()
    {
    do_mv();
    sp_mv_timeout=setTimeout(tm_mv,50);
    return false;
    }
