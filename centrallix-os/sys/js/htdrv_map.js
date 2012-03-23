// sys/js/htdrv_map.js
//
// Copyright (C) 2012 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.


// Map Widget Initialization 
//
 var map;
  var markerLayer;
function map_init(param)
    {
   
    var l = param.layer;
    //htr_init_layer(l,l,"map");
     
     //Create map with corresponding controllers from openLayers
     map = new OpenLayers.Map(l.id, {
                    controls: [
                        new OpenLayers.Control.Navigation(),
                        new OpenLayers.Control.PanZoomBar(),
                        new OpenLayers.Control.KeyboardDefaults()
                    ],
                    numZoomLevels: 6
                    
                });
    //load layers from StreetMap
    var layer = new OpenLayers.Layer.OSM( "Simple OSM Map");
    var vector = new OpenLayers.Layer.Vector('vector');
    map.addLayers([layer, vector]);
    
    //Center map to the US
    map.setCenter(
		new OpenLayers.LonLat(-104.821363, 38.833882).transform(
						new OpenLayers.Projection("EPSG:4326"),
						map.getProjectionObject()
						), 1
	);
	
      //This layer will contain the makers that we will place on the screen
      markerLayer = new OpenLayers.Layer.Markers('Markers');
      map.addLayer(markerLayer);
   
    
    
    ifc_init_widget(l);
    l.w = getdocWidth(l);

    // params from user
    l.param = param;

    // data structures for image/color layers
    l.objects = new Array();
    l.layer_cache = new Array();

    // internal functions
    l.Refresh = map_refresh;
    l.Clear = map_clear;
    l.NewLayer = map_new_layer;
    l.FreeLayer = map_free_layer;
    l.AddOsrcObject = map_add_osrc_object;
    l.SelectItem = map_select_item;
    l.SelectItemById = map_select_item_by_id;

    // page focus handler stuff
    l.getfocushandler = map_getfocus;
    l.losefocushandler = map_losefocus;
    l.keyhandler = map_keyhandler;

    // osrc client interface callbacks
    l.DataAvailable = map_osrc_data_available;
    l.ReplicaMoved = map_osrc_replica_moved;
    l.IsDiscardReady = map_osrc_is_discard_ready;
    l.ObjectAvailable = map_osrc_object_available;
    l.ObjectCreated = map_osrc_object_created;
    l.ObjectModified = map_osrc_object_modified;
    l.ObjectDeleted = map_osrc_object_deleted;
    l.OperationComplete = map_osrc_operation_complete;

    // Register with osrc.
    if (param.osrc)
	l.osrc = param.osrc;
    else
	l.osrc = wgtrFindContainer(l, "widget/osrc");
    if (l.osrc)
	l.osrc.Register(l);
	
	
    return l;
    }


function map_get_osrc_property(o,n)
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
function map_refresh()
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
function map_add_osrc_object(o)
    {
    // get needed props
    var lat = map_get_osrc_property(o, 'lat');
    if (lat == null) lat = 0;
    var lon = map_get_osrc_property(o, 'lon');
    if (lon == null) lon = 0;
    //var w = map_get_osrc_property(o, 'width');
    //var h = map_get_osrc_property(o, 'height');
    var c = map_get_osrc_property(o, 'color');
    if (c == '') c = null;
    var i = map_get_osrc_property(o, 'image');
    if (i == '') i = null;
    var lbl = map_get_osrc_property(o, 'label');
    var popup_text = map_get_osrc_property(o, 'popup_text');
    if (lbl == '') lbl = null;
    if ((lbl == null && c == null && i == null)) return false;
   
    
    lat = parseFloat(lat);
    lon = parseFloat(lon);
    
     var Onemarker = new OpenLayers.Marker(new OpenLayers.LonLat(lon, lat ).transform(
		new OpenLayers.Projection("EPSG:4326"),
				map.getProjectionObject()
				));
				
				   

		
		   /* Register the events */
                Onemarker.events.register('mousedown', Onemarker, testFunc.bind(this, popup_text, Onemarker.lonlat, o.id) );
		//marker.events.register('mousedown', marker, function(evt) { alert(this.icon.url); OpenLayers.Event.stop(evt); });
		
		/* Add markers to the map */
                markerLayer.addMarker(Onemarker);
		map.setCenter(new OpenLayers.LonLat(lon,lat).transform(
		new OpenLayers.Projection("EPSG:4326"),
				map.getProjectionObject()
				), 4);
    /*
    // make and position the layer
    var l = this.NewLayer();
    htr_init_layer(l, this, 'map');
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
*/
    // add to the list
  //  this.objects.push(l);
    // tag the images so they do not steal focus from the focus area
   // htutil_tag_images(l, 'map', l, this);    

    return true;
    }

// Clear all objects from the canvas
//
function map_clear()
    {
    var l;
    while (l = this.objects.pop())
	{
	if (l.area) pg_removearea(l.area);
	l.area = null;
	this.FreeLayer(l);
	}
    }

    
function testFunc(popup_text, lonlat, id){
	
	var popup = new OpenLayers.Popup("chicken",
					lonlat,
					null,
					popup_text,
					true);
	popup.autoSize =true;
	map.addPopup(popup);
	
	
	
	if (id && this.param.allow_select && this.osrc)
	this.osrc.MoveToRecord(id);
	 
	
}


// Create a new layer, possibly from the cache
//
function map_new_layer()
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
function map_free_layer(l)
    {
    htr_setvisibility(l, 'hidden');
    this.layer_cache.push(l);
    }

// Give selection to an object on the canvas.
//
function map_select_item(l)
    {
    alert("select item");
    if (l.area && this.param.show_select)
	pg_setdatafocus(l.area);
    this.selected_item = l;
    }

// Give selection... by osrc row id
//
function map_select_item_by_id(id)
    {
    for(var i=0; i<this.objects.length; i++)
	{
	if (this.objects[i].osrc_id == id)
	    return this.SelectItem(this.objects[i]);
	}
    }


// Page focus handler stuff
//
function map_getfocus(x,y,l,grp,id,a)
    {
	alert("focus");
    if (id.osrc_id && this.param.allow_select && this.osrc)
	this.osrc.MoveToRecord(id.osrc_id);
    if (this.param.allow_select)
	return 2;
    else
	return 0;
    }

function map_losefocus()
    {
    return true;
    }

function map_keyhandler(l, e, k)
    {
    }


// OSRC client interface callbacks
//
function map_osrc_data_available()
    {
    this.Clear();
    }

function map_osrc_replica_moved()
    {
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function map_osrc_is_discard_ready()
    {
    return true;
    }

function map_osrc_object_available(o)
    {
   
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function map_osrc_object_created(o)
    {
    this.Refresh();
    }

function map_osrc_object_modified(o)
    {
    this.Refresh();
    if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function map_osrc_object_deleted(o)
    {
    this.Refresh();
    }

function map_osrc_operation_complete(o)
    {
    return true;
    }



// Event handler functions
//
function map_mouseup(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function map_mousedown(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function map_mouseover(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function map_mouseout(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function map_mousemove(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_map.js'] = true;
