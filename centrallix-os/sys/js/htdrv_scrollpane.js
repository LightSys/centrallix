// Copyright (C) 1998-2004 LightSys Technology Services, Inc.
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
    var ml;
    var img;
    var i;
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
    setClipWidth(alayer, getClipWidth(l)-18);
    alayer.maxwidth=getClipWidth(alayer);
    alayer.minwidth=getClipWidth(alayer);
    tlayer.nofocus = true;
    alayer.nofocus = true;
    htr_init_layer(l,l,"sp");
    htr_init_layer(tlayer,l,"sp");
    htr_init_layer(alayer,l,"sp");
    l.LSParent = p;
    l.thum = tlayer;
    l.area = alayer;
    l.UpdateThumb = sp_UpdateThumb;
    l.ActionScrollTo = sp_action_scrollto;

    if(cx__capabilities.Dom0IE)
        {
        alayer.runtimeStyle.clip.pane = l;
        // how to watch this in IE?
        alayer.runtimeStyle.clip.onpropertychange = sp_WatchHeight;
	}
    else
    	{
    	alayer.clip.pane = l;
    	alayer.clip.watch("height",sp_WatchHeight);
    	}
    }

function sp_action_scrollto(aparam)
    {
    var h=getClipHeight(this.area); // height of content
    var d=h-getClipHeight(this); // height of non-visible content (max scrollable distance)
    if (d < 0) d=0;
    if (aparam.Percent)
	{
	if (aparam.Percent < 0) aparam.Percent = 0;
	else if (aparam.Percent > 100) aparam.Percent = 100;
	this.area.y = -d*aparam.Percent/100;
	}
    else if (aparam.Offset)
	{
	if (aparam.Offset < 0) aparam.Offset = 0;
	else if (aparam.Offset > d) aparam.Offset = d;
	this.area.y = -aparam.Offset;
	}
    this.UpdateThumb(h);
    }

function sp_WatchHeight(property, oldvalue, newvalue)
    {
    if (cx__capabilities.Dom0IE)
        {
        newvalue = htr_get_watch_newval(window.event);
        }

    // make sure region not offscreen now
    if (getRelativeY(this.pane.area) + newvalue < getClipHeight(this.pane)) setRelativeY(this.pane.area, getClipHeight(this.pane) - newvalue);
    if (newvalue < getClipHeight(this.pane)) setRelativeY(this.pane.area, 0);
    this.pane.UpdateThumb(newvalue);
    this.bottom = this.top + newvalue; /* ns seems to unlink bottom = top + height if you modify clip obj */
    return newvalue;
    }

function sp_UpdateThumb(h)
    {
    /** 'this' is a spXpane **/
    if(!h)
	{ /** if h is supplied, it is the soon-to-be clip.height of the spXarea **/
	h=getClipHeight(this.area); // height of content
	}
    var d=h-getClipHeight(this); // height of non-visible content (max scrollable distance)
    var v=getClipHeight(this)-(3*18);
    if(d<=0)
	setRelativeY(this.thum, 18);
    else
	setRelativeY(this.thum, 18+v*(-getRelativeY(this.area)/d));
    }

function do_mv()
    {

    var ti=sp_target_img;
    /** not sure why, but it's getting called with a null sp_target_img sometimes... **/
    if(!ti)
	{
	return;
	}
    var h=getClipHeight(ti.area); // height of content
    var d=h-getClipHeight(ti.pane); // height of non-visible content (max scrollable distance)
    var incr=sp_mv_incr;
    if(d<0)
	incr=0;
    if (ti.kind=='sp')
	{
	var scrolled = -getRelativeY(ti.area); // distance scrolled already
	if(incr > 0 && scrolled+incr>d)
	    incr=d-scrolled;

	/** if we've scrolled down less than we want to go up, go up the distance we went down **/
	if(incr < 0 && scrolled<-incr)
	    incr=-scrolled;

	/*var layers = pg_layers(ti.pane);
	for(var i=0;i<layers.length;i++)
	    {
	    if(layers[i] != ti.thum)
		{
		layers[i].y-=incr;
		}
	    }*/

	/** actually move the displayed content **/
	setRelativeY(ti.area, getRelativeY(ti.area)-incr);
	}
    else
	{
	alert(ti + ' -- ' + ti.id + ' is not known');
	}
    ti.pane.UpdateThumb();
    return true;
    }

function tm_mv()
    {
    sp_mv_timeout=null;
    do_mv();
    if (!sp_mv_timeout) sp_mv_timeout=setTimeout(tm_mv,50);
    return false;
    }
