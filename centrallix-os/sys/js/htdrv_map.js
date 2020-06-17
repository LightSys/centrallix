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
//var map;
//var markerLayer;

// Function to use Openlayers Styling Sheets
var addCss = fileName => {
  let head = document.head;
  let link = document.createElement("link");

  link.type = "text/css";
  link.rel = "stylesheet";
  link.href = fileName;

  head.appendChild(link);
};
addCss("/sys/css/htdrv_map.css");
addCss("/sys/js/openlayers/css/ol.css");

function map_init(param) {
  var l = param.layer;

  // var layer = new ol.layer.Tile({
  //   source: new ol.source.OSM({
  //     cacheSize: 1
  //   })
  // })

  var vector = new ol.layer.Vector({
    source: new ol.source.Vector()
  });

  var layer = new ol.layer.Tile({
    source: new ol.source.OSM({
      cacheSize: 1
    })
  });

  l.map = new ol.Map({
    target: l.id,
    layers: [layer, vector],
    view: new ol.View({
      projection: "EPSG:4326",
      center: ol.proj.fromLonLat([-104.821363, 38.833882]),
      zoom: 4
    }),
    controls: [
      //for some reason icon for zoom-out created a random character, which is why it is manually created here
      new ol.control.Zoom({ zoomOutLabel: "\u2013" }),
      new ol.control.ScaleLine()
    ]
  });

  l.map.setView(
    new ol.View({
      center: l.map.getView().getCenter(),
      zoom: l.map.getView().getZoom()
    })
  );

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

  htr_init_layer(l, l, "mapdynamic");

  //events
  var ie = l.ifcProbeAdd(ifEvent);
  ie.Add("Click");
  ie.Add("DblClick");
  ie.Add("RightClick");

  // Register with osrc.
  if (param.osrc) l.osrc = param.osrc;
  else l.osrc = wgtrFindContainer(l, "widget/osrc");
  if (l.osrc) l.osrc.Register(l);

  return l;
}

function map_get_osrc_property(o, n) {
  for (var k in o) {
    if (o[k].oid == n) {
      return o[k].value;
    }
  }
  return null;
}

// Refresh the data on the canvas - very stupid refresh for now totally
// clobbers all visual objects and re-creates them.
//
function map_refresh() {
  // loops through map to find marker then removes from layer
  this.map
    .getLayers()
    .getArray()
    .filter(layer => layer.get("name") === "Marker")
    .forEach(layer => this.map.removeLayer(layer));

  //loops through map to find popup element then removes from layer
  this.map
    .getOverlays()
    .getArray()
    .filter(overlay => overlay.get("element"))
    .forEach(overlay => this.map.removeOverlay(overlay));
  // First, clear all objects
  this.Clear;
  // Now, re-create them.
  for (var i = this.osrc.FirstRecord; i <= this.osrc.LastRecord; i++) {
    this.AddOsrcObject(this.osrc.replica[i], i);
  }
}

// Add an object from a record in the osrc
//
function map_add_osrc_object(o, id) {
  // get needed props
  var lat = map_get_osrc_property(o, "lat");
  if (lat == null) lat = 0;
  var lon = map_get_osrc_property(o, "lon");
  if (lon == null) lon = 0;
  var c = map_get_osrc_property(o, "color");
  if (c == "") c = null;
  var i = map_get_osrc_property(o, "image");
  if (i == "") i = null;
  var lbl = map_get_osrc_property(o, "label");
  var popup_text = map_get_osrc_property(o, "popup_text");
  if (lbl == "") lbl = null;
  if (lbl == null && c == null && i == null) return false;

  lat = parseFloat(lat);
  lon = parseFloat(lon);

  //creates marker
  var oneMarker = new ol.Feature({
    geometry: new ol.geom.Point(ol.proj.fromLonLat([lon, lat])),
    name: "Marker",
    content: popup_text
  });

  oneMarker.unique_id = id;

  // maker styling
  oneMarker.setStyle(
    new ol.style.Style({
      image: new ol.style.Icon({
        crossOrigin: "anonymous",
        src: "/sys/js/openlayers/img/marker.png"
      })
    })
  );

  // adds marker to vector source
  var vectorSource = new ol.source.Vector({
    features: [oneMarker]
  });

  // adds vectore source containing marker to vector layer
  var oneMarkerVectorLayer = new ol.layer.Vector({
    source: vectorSource,
    name: "Marker"
  });

  // creates popup html elements
  var containerDiv = document.createElement("div");
  containerDiv.id = "popup";
  containerDiv.setAttribute("class", "ol-popup");
  document.head.appendChild(containerDiv);

  var containerCloser = document.createElement("a");
  containerCloser.id = "popup-closer";
  containerCloser.setAttribute("href", "#");
  containerCloser.setAttribute("class", "ol-popup-closer");
  containerDiv.appendChild(containerCloser);

  var contentDiv = document.createElement("div");
  contentDiv.id = "popup-content";
  containerDiv.appendChild(contentDiv);

  var element = document.getElementById(containerDiv.id);
  var popupCloser = document.getElementById(containerCloser.id);

  //adds maker to map layer and focuses view
  this.map.addLayer(oneMarkerVectorLayer);
  this.map.getView().setCenter(ol.proj.fromLonLat([lon, lat]));
  this.map.getView().setZoom(4);

  //popup layer
  var popup = new ol.Overlay({
    element: element,
    stopEvent: false,
    name: "popover"
  });
  this.map.addOverlay(popup);

  //button to close popup
  popupCloser.onclick = () => {
    popup.setPosition(undefined);
    popupCloser.blur();
    return false;
  };

  //event to handle marker click
  this.map.on("click", evt => {
    let feature = this.map.forEachFeatureAtPixel(evt.pixel, feature => {
      return feature;
    });
    if (feature && feature.unique_id && this.param.allow_select && this.osrc) {
      this.trigger = true;
      this.osrc.MoveToRecord(feature.unique_id);
      delete this.trigger;

      contentDiv.innerHTML = feature.get("content");
      let coordinates = feature.getGeometry().getCoordinates();
      popup.setPosition(coordinates);

      let event = new Object();
      event.recnum = feature.unique_id;
      this.ifcProbe(ifEvent).Activate("Click", event);
      delete event;
    }
  });

  // handles double click
  this.map.on("dblclick", evt => {
    let feature = this.map.forEachFeatureAtPixel(evt.pixel, feature => {
      return feature;
    });
    if (feature) {
      let event = new Object();
      event.recnum = feature.unique_id;
      this.ifcProbe(ifEvent).Activate("DblClick", event);
      delete event;
      return false;
    }
  });

  this.map.on("contextmenu", evt => {
    let feature = this.map.forEachFeatureAtPixel(evt.pixel, feature => {
      return feature;
    });
    evt.preventDefault();
    if (feature && feature.unique_id && this.param.allow_select && this.osrc) {
      this.trigger = true;
      this.osrc.MoveToRecord(feature.unique_id);
      delete this.trigger;

      let event = new Object();
      event.recnum = feature.unique_id;
      this.ifcProbe(ifEvent).Activate("RightClick", event);
      delete event;
      return false;
    }
  });

  //changes curose when it is on marker
  this.map.on("pointermove", e => {
    // if (e.dragging) {
    //   $(element).popover("dispose");
    //   return;
    // }
    var pixel = this.map.getEventPixel(e.originalEvent);
    var hit = this.map.hasFeatureAtPixel(pixel);
    var target = document.getElementById(this.map.getTarget());
    target.style.cursor = hit ? "pointer" : "";
  });
  return true;
}

//Clear all objects from the canvas

function map_clear() {
  var l;
  while ((l = this.objects.pop())) {
    if (l.area) pg_removearea(l.area);
    l.area = null;
    this.FreeLayer(l);
  }
}

// Create a new layer, possibly from the cache
//
function map_new_layer() {
  var nl;
  if ((nl = this.layer_cache.pop()) == null) {
    nl = htr_new_layer(this.w, this);
    pg_set_style(nl, "position", "absolute");
  }
  htr_setvisibility(nl, "visible");
  return nl;
}

// Release a layer back to the cache.
//
function map_free_layer(l) {
  htr_setvisibility(l, "hidden");
  this.layer_cache.push(l);
}

// Give selection to an object on the canvas.
//
function map_select_item(l) {
  alert("select item");
  if (l.area && this.param.show_select) pg_setdatafocus(l.area);
  this.selected_item = l;
}

// Give selection... by osrc row id
//
function map_select_item_by_id(id) {
  for (var i = 0; i < this.objects.length; i++) {
    if (this.objects[i].osrc_id == id) return this.SelectItem(this.objects[i]);
  }
}

// Page focus handler stuff
//
function map_getfocus(x, y, l, grp, id, a) {
  alert("focus");
  if (id.osrc_id && this.param.allow_select && this.osrc)
    this.osrc.MoveToRecord(id.osrc_id);
  if (this.param.allow_select) return 2;
  else return 0;
}

function map_losefocus() {
  return true;
}

function map_keyhandler(l, e, k) {}

// OSRC client interface callbacks
//
function map_osrc_data_available() {
  this.Clear();
}

function map_osrc_replica_moved() {
  this.Refresh();
  if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
}

function map_osrc_is_discard_ready() {
  return true;
}

function map_osrc_object_available(o) {
  //link: http://dev.openlayers.org/docs/files/OpenLayers/Layer/Markers-js.html#OpenLayers.Layer.Markers.removeMarker
  //This is not working yet because the layer needs to be redrawn after changing the icon size.
  //var size = new OpenLayers.Size(100,100);
  //Onemarker.icon.size = size;
  /*
	for(var i in this.markerLayer.markers){
		this.markerLayer.markers[i].icon.size = size;
		
        }*/
  if (!this.trigger) {
    this.Refresh();
  }
  if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
}

function map_osrc_object_created(o) {
  this.Refresh();
}

function map_osrc_object_modified(o) {
  this.Refresh();
  if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
}

function map_osrc_object_deleted(o) {
  this.Refresh();
}

function map_osrc_operation_complete(o) {
  return true;
}

// Event handler functions
//
function map_mouseup(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function map_mousedown(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function map_mouseover(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function map_mouseout(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function map_mousemove(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function map_contextmenu(e) {
  return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}
//
// Load indication
if (window.pg_scripts) pg_scripts["htdrv_map.js"] = true;
