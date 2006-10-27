// Copyright (C) 1998-2006 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function cmp_cb_load_complete_4()
    {
    var dname = pg_links(this.cmp.loader)[0].target;
    var startupname = "startup_" + dname;
    window[startupname]();
    }

function cmp_cb_load_complete_3()
    {
    var larr = htr_get_layers(this.cmp.loader2);
    for(var i=0;i<larr.length;i++)
	this.cmp.loader.parentNode.appendChild(larr[i]);
    this.cmp.larr = larr;
    var scripts = document.getElementsByTagName('script');
    var css = document.getElementsByTagName('style');
    var head = document.getElementsByTagName('head')[0];
    var scripts_to_move = new Array();
    var css_to_move = new Array();
    for(var i=0;i<scripts.length;i++)
	{
	if (scripts[i].parentNode != head) 
	    scripts_to_move.push(scripts[i]);
	}
    for(var i=0;i<scripts_to_move.length;i++)
	{
	var newscript = document.createElement('script');
	var oldscript = scripts_to_move[i];
	if (oldscript.src) newscript.src = oldscript.src;
	if (oldscript.type) newscript.type = oldscript.type;
	if (oldscript.language) newscript.language = oldscript.language;
	if (oldscript.text) newscript.text = oldscript.text;
	head.appendChild(newscript);
	}
    for(var i=0;i<css.length;i++)
	{
	if (css[i].parentNode != head) 
	    css_to_move.push(css[i]);
	}
    for(var i=0;i<css_to_move.length;i++)
	{
	head.appendChild(css_to_move[i]);
	}
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_4', [], 200);
    }

function cmp_cb_load_complete_2()
    {
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_3', [], 30);
    }

function cmp_cb_load_complete_2ns()
    {
    //htr_alert(document.layers,1);
    var lnks = pg_links(this.loader);
    var dname = lnks[0].target;
    this.loader['startup_' + dname]();
    }

function cmp_cb_load_complete()
    {
    if (cx__capabilities.Dom0NS)
	{
	var larr = pg_layers(this);
	//htr_alert(document.layers,1);
	var lname = larr[0].id;
	var layers_to_move = new Array();
	for(var i=0;i<larr.length;i++)
	    {
	    layers_to_move.push(larr[i]);
	    //window.document.body.appendChild(larr[i]);
	    //alert('id ' + i + ' = ' + larr[i].id);
	    //moveAbove(larr[i], this.cmp.loader);
	    }
	for(var i=0;i<layers_to_move.length;i++)
	    {
	    moveAbove(layers_to_move[i], this.cmp.loader);
	    if (!this.cmp.loader.parentLayer.document.layers[layers_to_move[i].id])
		this.cmp.loader.parentLayer.document.layers[layers_to_move[i].id] = layers_to_move[i];
	    }
	this.cmp.larr = larr;
	pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_2ns', [], 300);
	}
    else
	{
	pg_serialized_write(this.cmp.loader2, this.contentWindow.document.getElementsByTagName("html")[0].innerHTML, cmp_cb_load_complete_2);
	}
    this.cmp.ifcProbe(ifEvent).Activate('LoadComplete', {});
    }

function cmp_instantiate(aparam)
    {
    var p = wgtrGetContainer(wgtrGetParent(this));
    var geom = tohex16(getWidth(p)) + tohex16(getHeight(p)) + tohex16(pg_charw) + tohex16(pg_charh) + tohex16(pg_parah);
    var graft = wgtrGetNamespace(this) + ':' + wgtrGetName(this);
    pg_serialized_load(this.loader, this.path + "?cx__geom=" + escape(geom) + "&cx__graft=" + escape(graft), cmp_cb_load_complete);
    }

function cmp_destroy(aparam)
    {
    var oldChild;
    //htr_alert(this,1);
    if (cx__capabilities.Dom1HTML)
	{
	for (var i = 0; i < this.larr.length;i++)
	    {
	    oldChild = window.document.body.removeChild(this.larr[i]);
	    delete oldChild;
	    delete this.larr[i];
	    }
	}
    else if (cx__capabilities.Dom0NS)
	{
	for (var i = 0; i < this.larr.length;i++)
	    {
	    var id = this.larr[i].id;
	    var p = this.larr[i].parentLayer;
	    delete p.document.layers[id];
	    }
	}
    }

function cmp_init(param)
    {
    var node = param.node;
    node.is_static = param.is_static;
    node.cmp = node;
    if (!param.is_static)
	{
	node.loader = param.loader;
	node.loader.cmp = node;
	node.path = param.path;
	htr_init_layer(node.loader,node,'cmp');

	if (cx__capabilities.Dom1HTML)
	    {
	    node.loader2 = htr_new_layer(pg_width, node.loader.parentNode);
	    node.loader2.cmp = node;
	    htr_setvisibility(node.loader2, 'hidden');
	    }
	}
    node.cmp_cb_load_complete = cmp_cb_load_complete;
    node.cmp_cb_load_complete_2 = cmp_cb_load_complete_2;
    node.cmp_cb_load_complete_2ns = cmp_cb_load_complete_2ns;
    node.cmp_cb_load_complete_3 = cmp_cb_load_complete_3;
    node.cmp_cb_load_complete_4 = cmp_cb_load_complete_4;
    ifc_init_widget(node);

    // Events
    var ie = node.ifcProbeAdd(ifEvent);
    ie.Add("LoadComplete");

    // Actions
    var ia = node.ifcProbeAdd(ifAction);
    ia.Add("Instantiate", cmp_instantiate);
    ia.Add("Destroy", cmp_destroy);

    return node;
    }
