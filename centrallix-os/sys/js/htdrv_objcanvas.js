// sys/js/htdrv_objcanvas.js
//
// Copyright (C) 2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.


// ObjectCanvas Widget Initialization 
//
function oc_init(param)
    {
    var l = param.layer;
    htr_init_layer(l,l,"oc");
    ifc_init_widget(l);
    l.w = getdocWidth(l);

    // params from user
    l.param = param;

    // data structures for image/color layers
    l.objects = new Array();
    l.layer_cache = new Array();

    // internal functions
    l.Refresh = oc_refresh;
    l.Clear = oc_clear;
    l.NewLayer = oc_new_layer;
    l.FreeLayer = oc_free_layer;
    l.AddOsrcObject = oc_add_osrc_object;
    l.SelectItem = oc_select_item;
    l.SelectItemById = oc_select_item_by_id;

    // page focus handler stuff
    l.getfocushandler = oc_getfocus;
    l.losefocushandler = oc_losefocus;
    l.keyhandler = oc_keyhandler;

    // osrc client interface callbacks
    l.DataAvailable = oc_osrc_data_available;
    l.ReplicaMoved = oc_osrc_replica_moved;
    l.IsDiscardReady = oc_osrc_is_discard_ready;
    l.ObjectAvailable = oc_osrc_object_available;
    l.ObjectCreated = oc_osrc_object_created;
    l.ObjectModified = oc_osrc_object_modified;
    l.ObjectDeleted = oc_osrc_object_deleted;
    l.OperationComplete = oc_osrc_operation_complete;

    // Register with osrc.
    if (param.osrc)
	l.osrc = param.osrc;
    else
	l.osrc = wgtrFindContainer(l, "widget/osrc");
    if (l.osrc)
	l.osrc.Register(l);

    return l;
    }


function oc_get_osrc_property(o,n)
    {
    for(var k in o)
	{
	if (o[k].oid == n) return o[k].value;
	}
    return null;
    }


// Refresh the data on the canvas - very stupid refresh for now totally
// clobbers all visual objects and re-creates them.
//
function oc_refresh()
    {
    // First, clear all objects
    this.Clear();

    // Now, re-create them.
    for(var i = this.osrc.FirstRecord; i<= this.osrc.LastRecord; i++)
	{
	this.AddOsrcObject(this.osrc.replica[i]);
	}
    }

// Add an object from a record in the osrc
//
function oc_add_osrc_object(o)
    {
    // get needed props
    var x = oc_get_osrc_property(o, 'x');
    if (x == null) x = 0;
    var y = oc_get_osrc_property(o, 'y');
    if (y == null) y = 0;
    var w = oc_get_osrc_property(o, 'width');
    var h = oc_get_osrc_property(o, 'height');
    var c = oc_get_osrc_property(o, 'color');
    if (c == '') c = null;
    var i = oc_get_osrc_property(o, 'image');
    if (i == '') i = null;
    var lbl = oc_get_osrc_property(o, 'label');
    if (lbl == '') lbl = null;
    if ((lbl == null && c == null && i == null)) return false;
    if (w != null) w = parseInt(w);
    if (h != null) h = parseInt(h);
    x = parseInt(x);
    y = parseInt(y);

    // make and position the layer
    var l = this.NewLayer();
    htr_init_layer(l, this, 'oc');
    l.osrc_oid = o.oid;
    l.osrc_id = o.id;
    moveTo(l, x, y);
    if (w && h)
	{
	resizeTo(l, w, h);
	setClipWidth(l, w);
	setClipHeight(l, h);
	}

    //alert('color = ' + c + ', image = ' + i);
    //htr_alert(o, 2);

    // set color
    if (c != null && htr_getbgcolor(l) != c)
	htr_setbgcolor(l, c);
    else 
	htr_setbgcolor(l, null);

    // set image
    if (i != null && (!l.imgsrc || l.imgsrc != i))
	htr_write_content(l, "<img src=\"" + i + "\" width=" + w + " height=" + h + ">");
    if (i == null && l.imgsrc != null)
	htr_write_content(l, "<img src=/sys/images/trans_1.gif width=" + w + " height=" + h + ">");
    l.imgsrc = i;
    if (i) l.img = pg_images(l)[0];
    else l.img = null;

    // add a focus area?
    if (this.param.allow_select || this.param.show_select)
	{
	l.area = pg_addarea(this, x-1, y-1, w+1, h+1, this.param.name, l, this.param.allow_select?1:0);
	//htr_alert(l.area, 1);
	}

    // add to the list
    this.objects.push(l);
    // tag the images so they do not steal focus from the focus area
    htutil_tag_images(l, 'oc', l, this);    

    return true;
    }

// Clear all objects from the canvas
//
function oc_clear()
    {
    var l;
    while (l = this.objects.pop())
	{
	if (l.area) pg_removearea(l.area);
	l.area = null;
	this.FreeLayer(l);
	}
    }

// Create a new layer, possibly from the cache
//
function oc_new_layer()
    {
    var nl;
    if ((nl = this.layer_cache.pop()) == null)
	{
	nl = htr_new_layer(this.w, this);
	pg_set_style(nl, 'position', 'absolute');
	}
    htr_setvisibility(nl, 'visible');
    return nl;
    }

// Release a layer back to the cache.
//
function oc_free_layer(l)
    {
    htr_setvisibility(l, 'hidden');
    this.layer_cache.push(l);
    }

// Give selection to an object on the canvas.
//
function oc_select_item(l)
    {
    if (l.area && this.param.show_select)
	pg_setdatafocus(l.area);
    this.selected_item = l;
    }

// Give selection... by osrc row id
//
function oc_select_item_by_id(id)
    {
    for(var i=0; i<this.objects.length; i++)
	{
	if (this.objects[i].osrc_id == id)
	    return this.SelectItem(this.objects[i]);
	}
    }


// Page focus handler stuff
//
function oc_getfocus(x,y,l,grp,id,a)
    {
    if (id.osrc_id && this.param.allow_select && this.osrc)
	this.osrc.MoveToRecord(id.osrc_id);
    if (this.param.allow_select)
	return 2;
    else
	return 0;
    }

function oc_losefocus()
    {
    return true;
    }

function oc_keyhandler(l, e, k)
    {
    }


// OSRC client interface callbacks
//
function oc_osrc_data_available()
    {
    this.Clear();
    }

function oc_osrc_replica_moved()
    {
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function oc_osrc_is_discard_ready()
    {
    return true;
    }

function oc_osrc_object_available(o)
    {
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function oc_osrc_object_created(o)
    {
    this.Refresh();
    }

function oc_osrc_object_modified(o)
    {
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function oc_osrc_object_deleted(o)
    {
    this.Refresh();
    }

function oc_osrc_operation_complete(o)
    {
    return true;
    }



// Event handler functions
//
function oc_mouseup(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function oc_mousedown(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function oc_mouseover(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function oc_mouseout(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function oc_mousemove(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_objcanvas.js'] = true;
