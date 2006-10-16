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

function sp_init(param)
    {
    var l = param.layer; 
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
	    if(ml.name==param.aname) alayer=ml;
	    if(ml.name==param.tname) tlayer=ml;
	    }
	}
    else if(cx__capabilities.Dom1HTML)
	{
	alayer = document.getElementById(param.aname);
	tlayer = document.getElementById(param.tname);
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
    ifc_init_widget(l);
    //l.LSParent = param.parent;
    l.thum = tlayer;
    l.area = alayer;
    l.UpdateThumb = sp_UpdateThumb;

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("ScrollTo", sp_action_scrollto);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

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
    var h = getClipHeight(this.area)+getClipTop(this.area); // height of content
    var ch = getClipHeight(this);
    var d = h-ch; // height of non-visible content (max scrollable distance)
    if (d < 0) d=0;
    if (typeof aparam.Percent != 'undefined')
	{
	if (aparam.Percent < 0) aparam.Percent = 0;
	else if (aparam.Percent > 100) aparam.Percent = 100;
	setRelativeY(this.area, -d*aparam.Percent/100);
	}
    else if (typeof aparam.Offset != 'undefined')
	{
	if (aparam.Offset < 0) aparam.Offset = 0;
	else if (aparam.Offset > d) aparam.Offset = d;
	setRelativeY(this.area, -aparam.Offset);
	}
    else if (typeof aparam.RangeStart != 'undefined' && typeof aparam.RangeEnd != 'undefined')
	{
	var ny = -getRelativeY(this.area);
	if (ny + ch < aparam.RangeEnd) ny = aparam.RangeEnd - ch;
	if (ny > aparam.RangeStart) ny = aparam.RangeStart;
	if (ny < 0) ny = 0;
	if (ny > d) ny = d;
	setRelativeY(this.area, -ny);
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
    newvalue += getClipTop(this.pane.area);
    if (getRelativeY(this.pane.area) + newvalue < getClipHeight(this.pane)) setRelativeY(this.pane.area, getClipHeight(this.pane) - newvalue);
    if (newvalue < getClipHeight(this.pane)) setRelativeY(this.pane.area, 0);
    this.pane.UpdateThumb(newvalue);
    newvalue -= getClipTop(this.pane.area);
    this.bottom = this.top + newvalue; /* ns seems to unlink bottom = top + height if you modify clip obj */
    return newvalue;
    }

function sp_UpdateThumb(h)
    {
    /** 'this' is a spXpane **/
    if(!h)
	{ /** if h is supplied, it is the soon-to-be clip.height of the spXarea **/
	h=getClipHeight(this.area)+getClipTop(this.area); // height of content
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
    var h=getClipHeight(ti.area)+getClipTop(ti.area); // height of content
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
