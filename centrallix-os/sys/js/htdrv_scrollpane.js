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
    if(cx__capabilities.Dom0NS)
	{
	var layers = pg_layers(l);
	for(i=0;i<layers.length;i++)
	    {
	    ml=layers[i];
	    if(ml.name==aname) alayer=ml;
	    if(ml.name==tname) tlayer=ml;
	    }
	}
    else if(cx__capabilities.Dom1HTML)
	{
	alayer = document.getElementById(aname);
	tlayer = document.getElementById(tname);
	}
    else
	{
	alert('browser not supported');
	}
    var images = pg_images(l);
    for(i=0;i<images.length;i++)
	{
	img=images[i];
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
    images = pg_images(tlayer);
    images[0].kind='sp';
    images[0].layer = images[0];
    images[0].mainlayer=l;
    images[0].thum=tlayer;
    images[0].area=alayer;
    images[0].pane=l;
    alayer.clip.width=l.clip.width-18;
    alayer.maxwidth=alayer.clip.width;
    alayer.minwidth=alayer.clip.width;
    tlayer.nofocus = true;
    alayer.nofocus = true;
    alayer.mainlayer = l;
    if(cx__capabilities.Dom0NS)
	{
	alayer.document.layer = alayer;
	tlayer.document.layer = tlayer;
	l.document.layer = l;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	alayer.layer = alayer;
	tlayer.layer = tlayer;
	l.layer = l;
	}
    else
	{
	alert('browser not supported');
	}
    tlayer.mainlayer = l;
    alayer.kind = 'sp';
    tlayer.kind = 'sp';
    l.mainlayer = l;
    l.kind = 'sp';
    l.LSParent = p;
    }

function do_mv()
    {
    var ti=sp_target_img;
    //alert('moving : ' + ti.kind + ' -- ' + sp_mv_incr);
    if (ti.kind=='sp' && sp_mv_incr > 0)
	{
	h=ti.area.clip.height;
	d=h-ti.pane.clip.height;
	//alert(h + ' -- ' + d);
	incr=sp_mv_incr;
	if(d<0) incr=0; else if (d+ti.area.y<incr) incr=d+ti.area.y;
	var layers = pg_layers(ti.pane);
	for(i=0;i<layers.length;i++) if (layers[i] != ti.thum)
	    layers[i].y-=incr;
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
	var layers = pg_layers(ti.pane);
	for(i=0;i<layers.length;i++) if (layers[i] != ti.thum)
	    layers[i].y+=incr;
	v=ti.pane.clip.height-(3*18);
	if(d<=0) ti.thum.y=18;
	else ti.thum.y=18+v*(-ti.area.y/d);
	}
    else
	{
	alert(ti + ' -- ' + ti.id + ' is not known');
	}
    return true;
    }

function tm_mv()
    {
    do_mv();
    sp_mv_timeout=setTimeout(tm_mv,50);
    return false;
    }
