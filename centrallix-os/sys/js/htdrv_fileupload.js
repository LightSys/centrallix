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
//
//Created by: Brady Steed
//Date: June, 2, 2014

function fu_change(e)
	{
	if (e.kind == 'fu')
		{
		
		if(e.mainlayer.files.length > 1)
			{
			/*var val = '';
			for(var i=0; i<e.mainlayer.files.length; i++)
			    {
			    if (i) val += ',';
			    val += e.mainlayer.files[i];
			    }*/
			e.layer.value = e.mainlayer.files.length + " files selected";
			e.layer.status = e.mainlayer.files.length + " files selected";
			cn_activate(e.layer, 'DataChange', {NewValue:e.mainlayer.files.length + " files selected", OldValue:e.layer.oldvalue});
			}
		else
			{
			e.layer.value = e.mainlayer.value;
			e.layer.status = e.mainlayer.value + ' selected';
			cn_activate(e.layer, 'DataChange', {NewValue:e.mainlayer.value, OldValue:e.layer.oldvalue});
			}
		}
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	}
	
function fu_clearvalue()
	{
	this.oldvalue = this.input.value;
	this.value = '';
	this.status = 'no files selected';
	this.pane.reset();
	cn_activate(this, 'DataChange', {NewValue:"", OldValue:this.input.oldvalue});
	}

function fu_prompt()
	{
	this.oldvalue = this.input.value;
	this.input.click();
	}
	
function fu_submit()
	{
	if(this.value != '')
        {
            var wgt = this;
            var form = new FormData();
            for(var i = 0; i < this.input.files.length; i++)
                form.append('file', this.input.files[i]);
            
	    this.status = 'upload in progress...';
            $.ajax({
            type: "POST",
            url: wgt.url,
            data: form,
            cache: false,
            dataType: 'json',
            processData: false,
            contentType: false,
            success: function(json)
                {
                wgt.oldvalue = wgt.input.value;
                wgt.value = '';
		wgt.status = 'upload complete';
                wgt.pane.reset();
                
                var data = {};
                for(var i = 0; i < json.length; i++)
                    {
                    data['OrigName' + (i+1)] = json[i]['fn'];
                    data['NewName' + (i+1)] = json[i]['up'];
                    }
                
                cn_activate(wgt, 'DataChange', {NewValue:"", OldValue:wgt.input.oldvalue});
                cn_activate(wgt, 'UploadComplete', data);
                },
            error: function()
		{
		wgt.status = 'upload failed';
                cn_activate(wgt, 'UploadError', {});
		}
            });
        }
	}
	
function fu_init(param)
	{
	var layer = param.layer;
	layer.input = param.input;
	layer.pane = param.pane;
	layer.iframe = param.iframe;
	layer.target = param.target;
	layer.oldvalue = '';
	layer.value = '';
	layer.status = 'no files selected';
	
	htr_init_layer(layer, layer, "fu");
	htr_init_layer(layer, layer.iframe, "fu");
	htr_init_layer(layer, layer.pane, "fu");
	htr_init_layer(layer, layer.input, "fu");
	ifc_init_widget(layer);

    
    layer.url = "?cx__akey="+akey + "&target="+htutil_escape(layer.target);
    
	if (param.form)
	layer.form = wgtrGetNode(layer, param.form);
    else
	layer.form = wgtrFindContainer(layer,"widget/form");
    if (layer.form) layer.form.Register(layer);
	
	//Events
	var ie = layer.ifcProbeAdd(ifEvent);
	ie.Add("DataChange");
	ie.Add("UploadComplete");
	ie.Add("UploadError");
	ie.Add("Change");
	
	//Actions
	var ia = layer.ifcProbeAdd(ifAction);
	ia.Add("Clear", fu_clearvalue);
	ia.Add("Prompt", fu_prompt);
	ia.Add("Submit", fu_submit);
	}

if (window.pg_scripts) pg_scripts['htdrv_fileupload.js'] = true;
