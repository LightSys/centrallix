// Copyright (C) 1998-2014 LightSys Technology Services, Inc.
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

window.tbld_touches = [];

// tbld_format_cell (FormatCell) - This function formats one cell in one row of the
// table, based on the source data and style configuration.
//
function tbld_format_cell(cell, color)
    {
    var txt = '', captxt = '', titletxt = '';
    var imgsrc = '', imgstyle = '';
    var style = 'margin:0px; padding:0px; ';
    var capstyle = 'font-size:80%; margin:2px 0px 0px 0px; padding:0px; ';
    var titlestyle = 'font-size:120%; margin:0px 0px 2px 0px; padding:0px; ';
    var colinfo = this.cols[cell.colnum];
    if (cell.subkind != 'headercell' && colinfo.type != 'check' && colinfo.type != 'image')
	var str = htutil_encode(String(htutil_obscure(cell.data)), colinfo.wrap != 'no');
    else
	var str = htutil_encode(String(cell.data));
    if (colinfo.wrap != 'no')
	str = htutil_nlbr(str);
    if (cell.subkind != 'headercell' && colinfo.type == 'progress')
	{
	// Progress Bar
	var val = cell.data;
	if (val !== null)
	    {
	    var roundto = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[cell.colnum].name, this.cols[cell.colnum].ns), 'round_to');
	    if (!roundto) roundto = 1.0;
	    var pad = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[cell.colnum].name, this.cols[cell.colnum].ns), 'bar_padding');
	    if (!pad) pad = 0;
	    pad = parseInt(pad);
	    var barcolor = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[cell.colnum].name, this.cols[cell.colnum].ns), 'bar_color');
	    if (!barcolor) barcolor = '#a0a0a0';
	    barcolor = String(barcolor).replace(/[^a-z0-9A-Z#]/g, "");
	    var bartext = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[cell.colnum].name, this.cols[cell.colnum].ns), 'bar_textcolor');
	    if (!bartext) bartext = 'black';
	    bartext = String(bartext).replace(/[^a-z0-9A-Z#]/g, "");
	    var actpct = '' + (100 * ((val < 0)?0:((val > 1)?1:val))) + '%';
	    actpct = String(actpct).replace(/[^0-9.%]/g, "");
	    var pct = '' + (Math.round(val * 100 / roundto) * roundto) + '%';
	    if (val >= 0.5)
		{
		innertxt = pct + ' ';
		outertxt = '';
		}
	    else
		{
		innertxt = ' ';
		outertxt = ' ' + pct;
		}
	    txt = '<div style="display:inline-block; width:100%;">' +
		      '<div style="display:inline-block; color:' + bartext + '; background-color:' + barcolor + '; padding:' + pad + 'px; text-align:right; min-width:1px; width:' + actpct + ';">' +
			  htutil_encode(innertxt) + 
		      '</div>' +
		      (outertxt?('<span style="padding:' + pad + 'px;">' +
			  htutil_encode(outertxt) + 
		      '</span>'):'') + 
		  '</div>';
	    }
	else
	    txt = '';
	}
    else if (cell.subkind != 'headercell' && colinfo.type == 'check')
	{
	// Checkmark
	var sl = str.toLowerCase();
	if (sl == 'n' || sl == 'no' || sl == 'off' || sl == 'false' || str == '0' || str == '' || str == 'null')
	    txt = '<img width="16" height="16" src="/sys/images/tbl_dash.gif">';
	else
	    txt = '<img width="16" height="16" src="/sys/images/tbl_check.gif">';
	}
    else if (cell.subkind != 'headercell' && colinfo.type == 'image')
	{
	// Image
	if (!(str.indexOf(':') >= 0 || str.indexOf('//') >= 0 || str.charAt(0) != '/'))
	    {
	    imgsrc = cell.data;
	    //txt = '<img src="' + htutil_encode(String(cell.data),true) + '"';
	    //if (colinfo.image_maxwidth || colinfo.image_maxheight)
		//{
		//txt += ' style="';
		//if (colinfo.image_maxwidth) txt += 'max-width:' + colinfo.image_maxwidth + 'px; ';
		if (colinfo.image_maxwidth) imgstyle += 'max-width:' + colinfo.image_maxwidth + 'px; ';
		//if (colinfo.image_maxheight) txt += 'max-height:' + colinfo.image_maxheight + 'px; ';
		if (colinfo.image_maxheight) imgstyle += 'max-height:' + colinfo.image_maxheight + 'px; ';
		//txt += '"';
		//}
	    //txt += ">";
	    imgstyle += htutil_getstyle(wgtrFindDescendent(this, colinfo.name, colinfo.ns), 'image', {} );
	    }
	}
    else
	{
	// Text
	txt = '<span onclick="function() {}">' + str + '</span>';
	}
    style += htutil_getstyle(wgtrFindDescendent(this,colinfo.name,colinfo.ns), null, {textcolor: color});
    if (cell.capdata)
	{
	// Caption (added to any of the above types)
	captxt = '<span onclick="function() {}">' + htutil_encode(htutil_obscure(cell.capdata), colinfo.wrap != 'no') + '</span>';
	if (colinfo.wrap != 'no')
	    captxt = htutil_nlbr(captxt);
	capstyle += htutil_getstyle(wgtrFindDescendent(this,colinfo.name,colinfo.ns), "caption", {textcolor: color});
	}
    if (cell.titledata)
	{
	titletxt = '<span onclick="function() {}">' + htutil_encode(htutil_obscure(cell.titledata), colinfo.wrap != 'no') + '</span>';
	if (colinfo.wrap != 'no')
	    titletxt = htutil_nlbr(titletxt);
	titlestyle += htutil_getstyle(wgtrFindDescendent(this,colinfo.name,colinfo.ns), "title", {textcolor: color});
	}

    // If style or content has changed, then update it.
    if (txt != cell.content || captxt != cell.capcontent || titletxt != cell.titlecontent || style != cell.cxstyle || capstyle != cell.cxcapstyle || titlestyle != cell.cxtitlestyle || imgsrc != cell.imgsrc || imgstyle != cell.imgstyle)
	{
	// Build the paragraph elements of the cell
	var t_p = null;
	var c_p = null;
	if (titletxt)
	    {
	    t_p = document.createElement('p');
	    $(t_p).attr("style", titlestyle);
	    $(t_p).css({'margin':'0px'});
	    $(t_p).append(titletxt);
	    }
	var p = document.createElement('p');
	$(p).attr("style", style);
	$(p).css({'margin':'0px'});
	if (txt)
	    $(p).append(txt);
	if (imgsrc)
	    {
	    var ie = document.createElement('img');
	    $(ie).attr('src', imgsrc);
	    if (imgstyle)
		$(ie).attr('style', imgstyle);
	    $(p).append(ie);
	    }
	if (captxt)
	    {
	    c_p = document.createElement('p');
	    $(c_p).attr("style", capstyle);
	    $(c_p).css({'margin':'0px'});
	    $(c_p).append(captxt);
	    }

	// build the cell
	$(cell).empty();
	if (titletxt)
	    $(cell).append(t_p)
	$(cell).append(p);
	if (captxt)
	    $(cell).append(c_p);

	// Remember the data so we can tell if it changed
	cell.content = txt;
	cell.capcontent = captxt;
	cell.titlecontent = titletxt;
	cell.cxstyle = style;
	cell.cxcapstyle = capstyle;
	cell.cxtitlestyle = titlestyle;
	cell.el_title = t_p;
	cell.el_text = p;
	cell.el_caption = c_p;
	cell.imgsrc = imgsrc;
	cell.imgstyle = imgstyle;

	// If an image, then test for final image loading, and readjust row
	// height once the image is loaded.
	//
	if (cell.firstChild && cell.firstChild.firstChild && cell.firstChild.firstChild.tagName == 'IMG')
	    {
	    var img = cell.firstChild.firstChild;
	    img.layer = cell;
	    $(img).one("load", function()
		{
		var t = this.layer.table;
		var row = this.layer.row;
		var pre_h = $(row).height();
		var upd_rows = [];
		//var disp_rows = [];
		t.UpdateHeight(row);
		if (pre_h != $(row).height() && row.positioned)
		    {
		    for(var i=row.rownum+1; i<=t.rows.last; i++)
			{
			if (t.rows[i])
			    {
			    t.rows[i].positioned = false;
			    upd_rows.push(t.rows[i]);
			    /*if (i <= t.rows.lastvis+1)
				disp_rows.push(t.rows[i]);*/
			    }
			}
		    t.PositionRows(upd_rows);
		    t.DisplayRows(upd_rows);
		    }
		});
	    }
	}
    }


// This function is used to compare two property sheet rows, and is used in
// the sorting process to alphabetize the property sheet items.
//
function tbld_attr_cmp(a, b)
    {
    if (a.oid > b.oid)
	return 1;
    else if (a.oid < b.oid)
	return -1;
    else
	return 0;
    }


// tbld_redraw_all (RedrawAll) - this is the core function that uses the objectsource
// data to create the necessary data rows in the table.
//
function tbld_redraw_all(dataobj, force_datafetch)
    {
    //this.log.push("tbld_redraw_all()");
    var new_rows = [];

    // Creating a new record?  Give indication if so.
    this.was_new = this.is_new;
    this.is_new = ((this.was_new && dataobj == null) || (dataobj && dataobj.length == 0 && this.row_bgndnew))?1:0;
    if (this.was_new && !this.is_new && this.rows.last)
	{
	var recnum = this.rows.last;
	if (this.rows[recnum].disp_mode == 'newselect')
	    {
	    this.scroll_maxheight -= ($(this.rows[recnum]).height() + this.cellvspacing*2);
	    this.scroll_maxrec--;
	    this.RemoveRow(this.rows[recnum]);
	    }
	}

    // Note the current record.
    this.cr = this.osrc.CurrentRecord;
    if (this.is_new)
	{
	this.cr = this.rows.last + 1 - this.was_new;
	this.target_range = {start:this.rows.first, end:this.rows.last + 1 - this.was_new};
	}

    // Presentation mode -- rows or propsheet?
    if (this.datamode == 1)
	{
	if (!dataobj && this.osrc.CurrentRecord && this.osrc.replica[this.osrc.CurrentRecord])
	    dataobj = this.osrc.replica[this.osrc.CurrentRecord];
	// Propsheet mode - build the attr list
	this.attrlist = [];
	for(var j in dataobj)
	    {
	    if (dataobj[j].oid && !dataobj[j].system)
		{
		this.attrlist.push(dataobj[j]);
		}
	    }
	this.attrlist.sort(tbld_attr_cmp);
	this.rows.lastosrc = this.attrlist.length;
	var min = 1;
	var max = this.attrlist.length;
	}
    else
	{
	// Rows mode.  OSRC says it has found the final record?
	if (this.osrc.FinalRecord)
	    this.rows.lastosrc = this.osrc.FinalRecord + this.is_new;

	var min = this.osrc.FirstRecord;
	var max = this.osrc.LastRecord + this.is_new;
	}

    // "no data" message?
    var ndm = $(this).children('#ndm');
    if (max >= min)
	{
	ndm.hide();
	}
    else
	{
	ndm.show();
	ndm.text(wgtrGetServerProperty(this,"nodata_message"));
	ndm.css({"top":((this.param_height - ndm.height())/2) + "px", "color":wgtrGetServerProperty(this,"nodata_message_textcolor") });
	}

    // (re)draw the loaded records
    var selected_position_changed = false;
    for(var i=this.target_range.start; i<=this.target_range.end; i++)
	{
	if (i >= min && i <= max)
	    {
	    if (!this.rows[i])
		{
		this.rows[i] = this.InstantiateRow(this.scrolldiv,0,0);
		this.rows[i].rownum = i;
		new_rows.push(this.rows[i]);
		}
	    var row = this.rows[i];
	    this.osrc.SetEvalRecord(i);
	    this.SetupRowData(i, this.is_new && i == this.target_range.end);
	    if (this.cr == i)
		{
		if (this.initselect && this.crname && this.crname != row.name && !selected_position_changed)
		    this.initselect = this.initselect_orig;
		this.crname = null;
		}
		//selected_position_changed = true;
	    //if (this.crname == row.name && this.cr != i)
	//	selected_position_changed = i;
	    var is_selected = this.initselect && i == this.osrc.CurrentRecord && !this.is_new;
	    this.FormatRow(i, is_selected, this.is_new && i == this.target_range.end);
	    this.osrc.SetEvalRecord(null);
	    }
	else if (i > 0)
	    {
	    this.RemoveRow(this.rows[i]);
	    }
	}

    // Position and display the new rows
    this.PositionRows(new_rows);
    this.DisplayRows(new_rows);
    if (this.rows.first != null && this.rows.last != null)
	this.RescanRowVisibility();

    if (this.rows.first === null)
	return;

    // Remove unneeded rows
    if (this.rows.first != null)
	{
	for(var i=this.rows.first; i<this.target_range.start && i<=this.rows.last; i++)
	    this.RemoveRow(this.rows[i]);
	}
    if (this.rows.last != null)
	{
	for(var i=this.rows.last; i>this.target_range.end && i>=this.rows.first; i--)
	    this.RemoveRow(this.rows[i]);
	}

    // Let OSRC know that we're showing these records, so they don't disappear
    // out from under us.
    if (this.datamode != 1)
	this.osrc.SetViewRange(this, this.rows.first, this.rows.last);

    // If selection changed, re-select the selected row by name
    //if (selected_position_changed)
//	{
//	this.osrc.MoveToRecord(selected_position_changed);
//	}

    // Need to scroll?
    if (this.target_y != null)
	{
	this.Scroll(this.target_y, this.target_anim);
	return;
	}

    // Adjust scrollbar thumb
    this.UpdateThumb(false);

    // Bring a record into view?
    if (this.bring_into_view)
	{
	this.BringIntoView(this.bring_into_view);
	return;
	}

    // Double check current record visibility
    if (this.old_scroll_y && this.cr >= this.rows.first && this.cr <= this.rows.last)
	{
	var new_y = this.old_scroll_y;
	this.old_scroll_y = 0;
	this.Scroll(new_y, false);
	return;
	}
    if ((this.cr < this.rows.firstvis || this.cr > this.rows.lastvis) && this.osrc_last_op != 'ScrollTo')
	{
	this.BringIntoView(this.cr);
	return;
	}

    // space at bottom of table?
    if (this.CheckBottom())
	return;

    // Dispatch osrc requests
    this.OsrcDispatch();
    }


// Handle deletion or new row cancel cases where blank space is now visible
// at the bottom of the table, even though there are enough rows to fill the
// visible area.
//
function tbld_check_bottom()
    {
    if (this.rows.lastvis > 0)
	{
	var rowid = this.rows.lastvis;
	if (this.rows[rowid+1] && this.rows[rowid+1].vis == 'partial')
	    rowid++;
	var lastrow = this.rows[rowid];
	var sy = this.scroll_y;
	var y_space = this.vis_height - ($(lastrow).height() + getRelativeY(lastrow) + this.cellvspacing*2 + sy);
	if (y_space > 0 && sy < (0-this.scroll_minheight))
	    {
	    if (y_space > -sy)
		y_space = -sy;
	    this.Scroll(sy + y_space, true);
	    return true;
	    }
	}

    return false;
    }


function tbld_find_osrc_value(rowslot, attrname)
    {
    var txt = '';
    if (this.osrc.LastRecord >= rowslot && this.osrc.FirstRecord <= rowslot)
	{
	for(var k in this.osrc.replica[rowslot])
	    {
	    if (this.osrc.replica[rowslot][k].oid == attrname)
		{
		txt = this.osrc.replica[rowslot][k].value;
		break;
		}
	    }
	if (txt == null || typeof txt == 'undefined')
	    txt = '';
	}
    return txt;
    }


function tbld_setup_row_data(rowslot, is_new)
    {
    var row = this.rows[rowslot];
    var changed = false;

    if (is_new)
	{
	for(var j in row.cols)
	    {
	    row.cols[j].data = '';
	    row.cols[j].capdata = '';
	    }
	row.needs_redraw = true;
	return true;
	}

    if (this.datamode == 1)
	{
	// Propsheet
	var attrid = rowslot-1;

	for(var j in row.cols)
	    {
	    var txt = '';
	    switch(this.cols[j].fieldname)
		{
		case 'name': txt = this.attrlist[attrid].oid; break;
		case 'value': txt = htutil_obscure(this.attrlist[attrid].value); break;
		case 'type': txt = this.attrlist[attrid].type; break;
		default: txt = '';
		}
	    if(txt == null || typeof txt == 'undefined')
		txt = '';
	    if (typeof row.cols[j].data == 'undefined' || (row.cols[j].data == null && txt) || txt != row.cols[j].data)
		changed = true;
	    row.cols[j].data=txt;
	    }
	}
    else
	{
	// Normal
	for(var j in row.cols)
	    {
	    // title value
	    var titlefield = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "title_fieldname");
	    if (titlefield)
		var txt = this.FindOsrcValue(rowslot, titlefield);
	    else
		var txt = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "title_value");
	    if (typeof row.cols[j].titledata == 'undefined' || (row.cols[j].titledata == null && txt) || txt != row.cols[j].titledata)
		changed = true;
	    row.cols[j].titledata = txt;

	    // main value
	    var txt = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "value");
	    if (!txt && this.cols[j].fieldname)
		txt = this.FindOsrcValue(rowslot, this.cols[j].fieldname);
	    if (typeof row.cols[j].data == 'undefined' || (row.cols[j].data == null && txt) || txt != row.cols[j].data)
		changed = true;
	    row.cols[j].data = txt;

	    // caption value
	    if (this.cols[j].caption_fieldname)
		var txt = this.FindOsrcValue(rowslot, this.cols[j].caption_fieldname);
	    else
		var txt = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "caption_value");
	    if (typeof row.cols[j].capdata == 'undefined' || (row.cols[j].capdata == null && txt) || txt != row.cols[j].capdata)
		changed = true;
	    row.cols[j].capdata = txt;
	    }
	row.name = this.FindOsrcValue(rowslot, 'name');
	}

    if (changed)
	row.needs_redraw = true;

    return changed;
    }


function tbld_get_selected_geom()
    {
    if (this.cr && this.rows[this.cr])
	var obj = this.rows[this.cr];
    else
	var obj = this;
    return {x:$(obj).offset().left,y:$(obj).offset().top,width:$(obj).width(),height:$(obj).height()};
    }


function tbld_css_height(element, seth)
    {
    if (seth == null)
	{
	return parseFloat($(element).css("height"));
	}
    else
	{
	$(element).css("height",seth + "px");
	}
    }


function tbld_update_height(row)
    {
    if (this.min_rowheight != this.max_rowheight)
	{
	var maxheight = 0;
	for(var i in row.cols)
	    {
	    var col = row.cols[i];
	    var h = $(col.firstChild).innerHeight();
	    if (col.firstChild && col.firstChild.nextSibling)
		h += ($(col.firstChild.nextSibling).innerHeight() + 2);
	    if (col.firstChild && col.firstChild.nextSibling && col.firstChild.nextSibling.nextSibling)
		h += ($(col.firstChild.nextSibling.nextSibling).innerHeight() + 2);
	    if (h > this.max_rowheight - this.cellvspacing*2)
		h = this.max_rowheight - this.cellvspacing*2;
	    if (h < this.min_rowheight - this.cellvspacing*2)
		h = this.min_rowheight - this.cellvspacing*2;
	    if (tbld_css_height(col) != h)
		tbld_css_height(col,h);
	    if (h > maxheight)
		maxheight = h;
	    }
	}
    else
	{
	var maxheight = this.min_rowheight - this.cellvspacing*2;
	}

    // widget/table-row-detail positioning
    for(var i=0; i<row.detail.length; i++)
	{
	var dw = row.detail[i];
	$(dw).css({"top": maxheight + "px"});
	maxheight += $(dw).height();
	}

    // No change?
    if (tbld_css_height(row) == maxheight + this.innerpadding*2)
	return false;

    tbld_css_height(row,maxheight + this.innerpadding*2);
    return true;
    }


function tbld_format_row(id, selected, do_new)
    {
    var new_disp_mode;
    if (this.row_bgndnew && do_new)
	new_disp_mode = 'newselect';
    else if (this.showselect && selected)
	new_disp_mode = 'select';
    else
	new_disp_mode = 'deselect';

    if (new_disp_mode == this.rows[id].disp_mode && this.rows[id].needs_redraw == false)
	return;

    switch(new_disp_mode)
	{
	case 'newselect':
	    this.selected_row = this.rows[id];
	    this.selected = id;
	    this.rows[id].newselect();
	    break;
	case 'select':
	    this.selected_row = this.rows[id];
	    this.selected = id;
	    this.rows[id].select();
	    break;
	case 'deselect':
	    this.rows[id].deselect();
	    if (this.selected_row == this.rows[id])
		{
		this.selected_row = null;
		this.selected = null;
		}
	    break;
	}
    if (this.UpdateHeight(this.rows[id]) && this.rows[id].positioned)
	{
	// Height changed; move the rows below us.
	var upd_rows = [];
	for(var j=id; j<=this.rows.last; j++)
	    {
	    if (this.rows[j] && this.rows[j].positioned)
		{
		if (j>id)
		    this.rows[j].positioned = false;
		upd_rows.push(this.rows[j]);
		}
	    }
	if (upd_rows.length)
	    {
	    this.PositionRows(upd_rows);
	    this.DisplayRows(upd_rows);
	    //this.CheckBottom();
	    }
	}
    this.rows[id].disp_mode = new_disp_mode;
    this.rows[id].needs_redraw = false;
    }


function tbld_bring_into_view(rownum)
    {
    //this.log.push("tbld_bring_into_view(" + rownum + ")");
    this.bring_into_view = null;

    // Clamp the requested row to the available range
    if (rownum < 1)
	rownum = 1;
    if (this.rows.lastosrc && rownum > this.rows.lastosrc)
	rownum = this.rows.lastosrc;

    // Already visible?
    if (rownum >= this.rows.firstvis && rownum <= this.rows.lastvis)
	{
	this.OsrcDispatch();
	return;
	}

    // If row is in current set, just scroll backward.
    if (rownum >= this.rows.first && rownum < this.rows.firstvis)
	{
	this.Scroll(0 - getRelativeY(this.rows[rownum]), true);
	return;
	}

    // Likewise, if row is in current set, scroll forward.
    if (rownum <= this.rows.last && rownum > this.rows.lastvis)
	{
	this.Scroll(this.vis_height - (getRelativeY(this.rows[rownum]) + $(this.rows[rownum]).height() + this.cellvspacing*2), true);
	return;
	}

    // Out of range
    if (rownum < this.rows.first && this.rows.first != null)
	{
	this.bring_into_view = rownum;
	//this.target_range = {start:rownum, end:this.rows.lastvis+1};
	this.target_range = {start:rownum, end:rownum + this.rowcache_size};
	if (this.rows.lastosrc && this.target_range.end > this.rows.lastosrc)
	    this.target_range.end = this.rows.lastosrc;
	if (this.target_range.start > 1 && this.target_range.end - this.target_range.start < this.rowcache_size)
	    this.target_range.start = this.target_range.end - this.rowcache_size;
	if (this.target_range.start < 1)
	    this.target_range.start = 1;
	this.OsrcRequest('ScrollTo', {start:this.target_range.start, end:this.target_range.end});
	}
    else if (rownum > this.rows.last && this.rows.last != null)
	{
	this.bring_into_view = rownum;
	//this.target_range = {start:this.rows.firstvis-1, end:rownum};
	this.target_range = {start:rownum - this.rowcache_size, end:rownum};
	if (this.target_range.start < 1)
	    this.target_range.start = 1;
	this.OsrcRequest('ScrollTo', {start:this.target_range.start, end:this.target_range.end});
	}
    else
	{
	this.OsrcDispatch();
	}
    }


function tbld_update_scrollbar()
    {
    $(this.scrollbar).stop(false, true);
    if (this.thumb_height != this.thumb_avail)
	htr_setvisibility(this.scrollbar, "inherit");
    $(this.scrollbar).animate({"opacity": (this.thumb_height == this.thumb_avail)?0.0:(this.has_mouse?1.0:0.33)}, 150, "linear",
	() => { if (this.thumb_height == this.thumb_avail) htr_setvisibility(this.scrollbar, "hidden"); } );
    }


function tbld_update_thumb(anim)
    {
    // Current offset of scroll area
    this.thumb_sy = (0 - this.scroll_y) - this.scroll_minheight;

    // Estimate the scrolling geometries
    if (!this.rows.lastosrc)
	this.thumb_sh = (this.scroll_maxheight - this.scroll_minheight)*3;
    else if (this.scroll_maxrec && this.scroll_maxrec < this.rows.lastosrc)
	this.thumb_sh = (this.scroll_maxheight - this.scroll_minheight)*(this.rows.lastosrc)/(this.scroll_maxrec - this.scroll_minrec + 1);
    else if (this.scroll_maxrec && this.scroll_maxrec == this.rows.lastosrc)
	{
	this.thumb_sh = (this.scroll_maxheight - this.scroll_minheight);
	if (this.thumb_sh < (this.scroll_maxrec - this.scroll_minrec + 1)*this.min_rowheight)
	    {
	    this.thumb_sh = (this.scroll_maxrec - this.scroll_minrec + 1)*this.min_rowheight;
	    if (this.rows.firstvis == 1)
		this.thumb_sy = 0;
	    else
		this.thumb_sy = this.min_rowheight*(this.rows.lastvis - this.max_display);
	    }
	}
    else
	this.thumb_sh = this.vis_height;
    this.thumb_sh -= this.vis_height;
    if (this.thumb_sh < 0)
	this.thumb_sh = 0;
    this.thumb_avail = this.vis_height - 2*18 - 1;
    this.thumb_height = this.thumb_avail * this.vis_height / (this.thumb_sh + this.vis_height);
    if (this.thumb_height < 10)
	this.thumb_height = 10;
    this.thumb_pos = (this.thumb_sh==0)?0:Math.round((this.thumb_avail - this.thumb_height) * (this.thumb_sy / this.thumb_sh));

    // Move the scroll thumb
    if (anim)
	{
	$(this.scrollbar.b).stop(false, false);
	$(this.scrollbar.b).animate({"top": (18+Math.round(this.thumb_pos))+"px", "height": (Math.round(this.thumb_height)-2)+"px"}, 250, anim, null);
	}
    else
	{
	setRelativeY(this.scrollbar.b, 18 + Math.round(this.thumb_pos));
	$(this.scrollbar.b).height(Math.round(this.thumb_height) - 2);
	}

    // Set scrollbar visibility
    if (this.demand_scrollbar)
	{
	this.UpdateScrollbar();
	}
	//$(this.scrollbar).css({"opacity": (this.thumb_height == this.thumb_avail)?0.0:1.0});
	//htr_setvisibility(this.scrollbar, (this.thumb_height == this.thumb_avail)?"hidden":"inherit");
    }


function tbld_object_created(recnum)
    {
    //this.log.push("Object Created callback (" + recnum + ") from osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
    if (this.rows.lastosrc && recnum > this.rows.lastosrc)
	this.rows.lastosrc = recnum;
    if (recnum < this.rows.first)
	this.target_range = {start:recnum, end:this.rows.last};
    else if (recnum > this.rows.last)
	this.target_range = {start:this.rows.first, end:recnum};
    this.osrc_busy = false;
    this.RedrawAll(null, true);
    }

function tbld_object_deleted(recnum)
    {
    //this.log.push("Object Deleted callback (" + recnum + ") from osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
    if (this.rows.lastosrc && this.rows.lastosrc == recnum)
	this.rows.lastosrc--;
    if (this.rows[recnum] && this.scroll_maxheight)
	{
	this.scroll_maxheight -= ($(this.rows[recnum]).height() + this.cellvspacing*2);
	this.scroll_maxrec--;
	}
    this.osrc_busy = false;
    this.RedrawAll(null, true);
    }

function tbld_object_modified(current, dataobj)
    {
    //this.log.push("Object Modified callback from osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
    this.osrc_busy = false;
    this.RedrawAll(null, true);
    }

function tbld_replica_changed(dataobj, force_datafetch, why)
    {
    //this.log.push("ReplicaMoved / ObjectAvailable from osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
    this.osrc_busy = false;
    this.RedrawAll(dataobj, force_datafetch);
    this.osrc_last_op = null;
    }

function tbld_operation_complete(stat, osrc)
    {
    // If the operation (move, etc.) failed...
    if (!stat)
	this.osrc_busy = false;
    }

function tbld_clear_rows(fromobj, why)
    {
    if (why == 'refresh')
	{
	this.crname = (this.cr && this.rows[this.cr])?this.rows[this.cr].name:null;
	this.old_scroll_y = this.scroll_y;
	}
    else
	{
	this.crname = null;
	this.old_scroll_y = 0;
	}
    for(var i=this.rows.first; i<=this.rows.last; i++)
	{
	this.RemoveRow(this.rows[i]);
	}
    setRelativeY(this.scrolldiv,0);
    this.scroll_y = 0;
    this.rows.lastosrc = null;
    this.scroll_maxheight = null;
    this.scroll_maxrec = null;
    this.scroll_minheight = null;
    this.scroll_minrec = null;
    this.target_range = {start:1, end:this.rowcache_size};
    this.UpdateThumb(false);
    if (why != 'refresh')
	this.initselect = this.initselect_orig;
    }

function tbld_select()
    {
    var txt;
    //this.style = htutil_getstyle(this.table, "rowhighlight", {});
    htr_stylize_element(this, this.table, ["rowhighlight","row"], {border_color:'transparent'});
    //tbld_setbackground(this, this.table, 'rowhighlight');
    //htr_setbackground(this, wgtrGetServerProperty(this.table, 'rowhighlight_bgcolor', this.table.row_bgndhigh));
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], wgtrGetServerProperty(this.table, 'textcolorhighlight', this.table.textcolorhighlight));
	}
    if (this.ctr)
	{
	this.removeChild(this.ctr);
	this.ctr = null;
	}
    if(tbld_current==this)
	{
	this.mouseover();
	}
    this.showdetail(false);
    }


function tbld_showdetail(on_new)
    {
    for(var i=0; i<this.table.detail_widgets.length; i++)
	{
	var dw = this.table.detail_widgets[i];
	dw.on_new = on_new;
	this.updatedetail(dw);
	}
    }


function tbld_update_detail(dw)
    {
    if (dw.display_for && (this.table.initselect !== 2 || (this.table.initselect == 2 && dw.on_new)) /* 2 = noexpand */ && (!dw.on_new || wgtrGetServerProperty(dw, 'show_on_new', 0)))
	{
	var found=false;
	for(var j=0; j<this.detail.length; j++)
	    {
	    if (this.detail[j] == dw)
		{
		found=true;
		break;
		}
	    }

	if (!found)
	    {
	    // already a part of another row?
	    if ($(dw).css("visibility") == 'inherit' || $(dw).css("visibility") == 'visible')
		{
		pg_reveal_event(dw, dw, 'Obscure');
		dw.is_visible = 0;
		dw.ifcProbe(ifEvent).Activate('Close', {});
		}

	    // Add to this row and show it.
	    this.detail.push(dw);
	    this.appendChild(dw);
	    $(dw).css
		({
		"visibility": "inherit",
		"left": "0px",
		"top": "0px",
		});
	    pg_reveal_event(dw, dw, 'Reveal');
	    dw.is_visible = 1;
	    dw.ifcProbe(ifEvent).Activate('Open', {});
	    }
	}
    else
	{
	for(var j=0; j<this.detail.length; j++)
	    {
	    if (this.detail[j] == dw)
		{
		this.detail.splice(j, 1);
		pg_reveal_event(dw, dw, 'Obscure');
		$(dw).css
		    ({
		    "visibility": "hidden",
		    });
		this.table.appendChild(dw);
		dw.is_visible = 0;
		dw.ifcProbe(ifEvent).Activate('Close', {});
		break;
		}
	    }
	}
    }


function tbld_get_displayfor(attr)
    {
    return this.display_for;
    }


function tbld_set_displayfor(attr, val)
    {
    val = val?1:0;
    if (val != this.display_for)
	{
	this.display_for = val;
	if (this.table.selected_row)
	    {
	    this.table.selected_row.needs_redraw = true;
	    this.table.RedrawAll(null, true);
	    }
	}
    }


function tbld_deselect()
    {
    var txt;
    //tbld_setbackground(this, this.table, this.rownum%2?'row1':'row2');
    htr_stylize_element(this, this.table, [(this.rownum%2?'row1':'row2'),"row"], {border_color:'transparent'});
    //htr_setbackground(this, wgtrGetServerProperty(this.table, (this.rownum%2?'row1_bgcolor':'row2_bgcolor'), this.rownum%2?this.table.row_bgnd1:this.table.row_bgnd2));
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], wgtrGetServerProperty(this.table, 'textcolor', this.table.textcolor));
	}
    if (this.ctr)
	{
	this.removeChild(this.ctr);
	this.ctr = null;
	}
    this.hidedetail();
    }

function tbld_hidedetail()
    {
    for(var i=0; i<this.detail.length; i++)
	{
	var dw = this.detail[i];
	if (dw.parentElement == this)
	    {
	    pg_reveal_event(dw, dw, 'Obscure');
	    $(dw).css
		({
		"visibility": "hidden",
		});
	    this.table.appendChild(dw);
	    dw.is_visible = 0;
	    dw.ifcProbe(ifEvent).Activate('Close', {});
	    }
	}
    this.detail = [];
    }

function tbld_newselect()
    {
    var txt;
    //tbld_setbackground(this, this.table, 'newrow');
    htr_stylize_element(this, this.table, ['newrow',"row"], {border_color:'transparent'});
    //htr_setbackground(this, wgtrGetServerProperty(this.table, 'newrow_bgcolor', this.table.row_bgndnew));
    if (!this.ctr)
	{
	this.ctr = document.createElement("center");
	this.ctr.appendChild(document.createTextNode("-- new --"));
	this.appendChild(this.ctr);
	}
    for(var i in this.cols)
	{
	this.table.FormatCell(this.cols[i], wgtrGetServerProperty(this.table, 'textcolornew', this.table.textcolornew));
	}
    this.showdetail(true);
    }

function tbld_setbackground(obj, widget, prefix)
    {
    // prefix
    prefix = prefix?(prefix + '_'):'';

    // try image
    var img = wgtrGetServerProperty(widget, prefix + 'background');
    if (img)
	return htr_setbgimage(obj, img);

    // color
    var color = wgtrGetServerProperty(widget, prefix + 'bgcolor');
    if (color)
	return htr_setbgcolor(obj, color);

    // no background
    return htr_setbackground(this, null);
    }

function tbld_domouseover()
    {
    var mfocus = wgtrGetServerProperty(this.table, "show_mouse_focus");
    mfocus = (mfocus == 'yes' || mfocus == 1 || mfocus == true || mfocus == 'on' || mfocus == 'true' || mfocus == null)?true:false;
    if(this.rownum!=null && this.subkind!='headerrow' && mfocus)
	$(this).css({"border": "1px solid black"});
    }

function tbld_domouseout()
    {
    if(this.subkind!='headerrow')
	{
	var rbc = "";
	if (this.disp_mode == 'select')
	    rbc = wgtrGetServerProperty(this.table,"rowhighlight_border_color");
	if (!rbc)
	    rbc = wgtrGetServerProperty(this.table,"row_border_color");
	var mfocus = wgtrGetServerProperty(this.table, "show_mouse_focus");
	mfocus = (mfocus == 'yes' || mfocus == 1 || mfocus == true || mfocus == 'on' || mfocus == 'true' || mfocus == null)?true:false;
	if (mfocus)
	    $(this).css({"border": "1px solid " + (rbc?rbc:"transparent") });
	}
    }


function tbld_sched_scroll(y)
    {
    if (this.scroll_timeout)
	pg_delsched(this.scroll_timeout);
    $(this.scrolldiv).stop(false, true);
    $(this.box).stop(false, true);
    this.scroll_timeout = pg_addsched_fn(this, "Scroll", [y], 0);
    }


// Scroll the table to the given y offset
function tbld_scroll(y, animate)
    {
    if (animate == true)
	animate = 'swing';
    //this.log.push("tbld_scroll(" + y + ")");
    this.target_y = null;

    // Not enough data to scroll?
    if (this.thumb_height == this.thumb_avail && y != 0 - getRelativeY(this.rows[this.rows.first]))
	{
	this.OsrcDispatch();
	return;
	}

    // Current start and end of scrollable content
    var scroll_start = getRelativeY(this.rows[this.rows.first]);
    var scroll_end = getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing*2;

    // Clamp the scroll range
    if (this.rows.lastosrc == this.rows.last && (0-y) > scroll_end - this.vis_height)
	y = 0 - (scroll_end - this.vis_height);
    if (this.rows.first == 1 && (0-y) < scroll_start)
	y = 0 - scroll_start;

    // No new data needed?
    if (scroll_start <= (0-y) && (scroll_end - this.vis_height >= (0-y) || this.rows.lastosrc == this.rows.last))
	{
	this.scroll_y = y;
	$(this.scrolldiv).stop(false, false);
	if (animate)
	    $(this.scrolldiv).animate({"top": y+"px"}, 250, animate, null);
	else
	    $(this.scrolldiv).css({"top": y+"px"});
	this.target_range = {start:this.rows.first, end:this.rows.last};
	this.RescanRowVisibility();
	this.UpdateThumb(animate);
	this.OsrcDispatch();
	}
    else
	{
	// Need new data.  Set our target Y position and request more data.
	this.target_y = y;
	this.target_anim = animate;
	if (y == 0)
	    {
	    // Reset to the beginning
	    this.target_range = {start:1, end:this.rowcache_size};
	    }
	else if ((0-y) < scroll_start)
	    {
	    // Previous data
	    this.target_range.start = this.rows.first - this.rowcache_size;
	    if (this.target_range.start < 1)
		this.target_range.start = 1;
	    this.target_range.end = this.rows.lastvis+1;
	    }
	else if (getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing - this.vis_height < (0-y))
	    {
	    // Next data
	    this.target_range.start = this.rows.firstvis-1;
	    if (this.target_range.start < 1)
		this.target_range.start = 1;
	    this.target_range.end = this.rows.firstvis + this.rowcache_size + this.max_display;

	    // If there are enough rows in the replica, and we got here, we need to
	    // look further in the result set to satisfy the scroll requirements.
	    while (this.osrc.FirstRecord <= this.target_range.start && this.osrc.LastRecord >= this.target_range.end)
		this.target_range.end += this.rowcache_size;
	    }
	else
	    {
	    // ??
	    this.OsrcDispatch();
	    return;
	    }
	this.OsrcRequest('ScrollTo', {start:this.target_range.start, end:this.target_range.end});
	}
    }


function tbld_up_click()
    {
    this.table.BringIntoView(this.table.rows.firstvis-1);
    }


function tbld_down_click()
    {
    this.table.BringIntoView(this.table.rows.lastvis+1);
    }


function tbld_bar_click(e)
    {
    var sb = e.layer;
    var t = sb.table;
    if (e.pageY > $(sb.b).offset().top + $(sb.b).height())
	{
	// Down a page
	var target_row = t.rows[t.rows.lastvis];
	var target_y = 0 - (getRelativeY(target_row) + $(target_row).height() + t.cellvspacing*2);
	t.Scroll(target_y, true);
	}
    else if (e.pageY < $(sb.b).offset().top)
	{
	// Up a page
	var target_row = t.rows[t.rows.firstvis];
	var target_y = t.vis_height - getRelativeY(target_row);
	t.Scroll(target_y, true);
	}
    }


function tbld_change_width(move)
    {
    var l=this;
    var t=l.row.table;
    var rw = $(l.resizebdr).width();
    var colinfo = t.cols[l.colnum];

    // Sanity checks on column resizing...
    if(colinfo.xoffset+colinfo.width+move+rw>l.row.w)
	move = l.row.w - rw - colinfo.xoffset - colinfo.width;
    if(colinfo.xoffset+colinfo.width+rw+move<0)
	move=0-colinfo.xoffset-rw;
    if (colinfo.width + move < 3)
	move = 3-colinfo.width;
    if(l.resizebdr.xoffset+move<0)
	move=0-l.resizebdr.xoffset;
    if(getPageX(l.resizebdr) + t.colsep + t.bdr_width*2 + move >= getPageX(t) + t.param_width)
	move = getPageX(t) + t.param_width - getPageX(l.resizebdr) - t.colsep - t.bdr_width*2;

    // Figure how much space on the right of this resize handle we're adjusting, too...
    var cols_right = t.colcount - l.colnum - 1;
    var adj = [];
    var total_right_width = 0;
    for(var j=l.colnum+1; j<t.colcount; j++)
	total_right_width += t.cols[j].width;

    // Adjust the column metadata
    var total_move = move;
    for(var j=l.colnum; j<t.colcount; j++)
	{
	if (j == l.colnum)
	    {
	    // Column to the left of adjustment
	    t.cols[j].width += move;
	    }
	else
	    {
	    // Columns to the right of adjustment
	    adj[j] = t.cols[j].width/total_right_width*move;
	    t.cols[j].width -= adj[j];
	    t.cols[j].xoffset += total_move;
	    total_move -= adj[j];
	    }

	// Adjust the resize border lines
	if (t.rows[0] && t.rows[0].cols[j].resizebdr)
	    {
	    var rb = t.rows[0].cols[j].resizebdr;
	    rb.xoffset += total_move;
	    setRelativeX(rb, rb.xoffset);
	    }
	}

    // Adjust the actual header and data rows and columns
    var upd_rows = [];
    for(var i=0; i<=t.rows.last; i++)
	{
	if (i == 0 || i >= t.rows.first)
	    {
	    if (t.ApplyRowGeom(t.rows[i], l.colnum) && t.min_rowheight != t.max_rowheight)
		{
		// Need to update height of row?
		if (t.UpdateHeight(t.rows[i]))
		    {
		    for(var j=i+1; j<=t.rows.last; j++)
			{
			if (t.rows[j].positioned)
			    {
			    t.rows[j].positioned = false;
			    upd_rows.push(t.rows[j]);
			    }
			}
		    }
		}
	    }
	}
    if (upd_rows.length)
	{
	t.PositionRows(upd_rows);
	t.CheckBottom();
	}

    return move;
    }


// Applies the computed row geometry to a given row, and returns
// true if we might need to adjust the row's height.
//
function tbld_apply_row_geom(row, firstcol)
    {
    if (!row)
	return false;
    var change_wrapped_cell = false;
    for(var j=firstcol; j<this.colcount; j++)
	{
	var c=row.cols[j];
	var new_w = this.cols[j].width; // - this.innerpadding*2;
	if (this.colsep > 0 || this.dragcols)
	    new_w -= (this.bdr_width*2 + this.colsep);
	$(c).width(new_w);
	setRelativeX(c, this.cols[j].xoffset);
	if (this.cols[j].wrap != 'no')
	    change_wrapped_cell = true;
	}
    return change_wrapped_cell;
    }


function tbld_unsetclick(l,n)
    {
    l.clicked[n] = 0;
    }


// Callback used for obscure and reveal checks from row detail widgets
function tbld_cb_dw_reveal(event)
    {
    /*switch(e.eventName)
	{
	case 'ObscureOK':
	    this.tabctl.ChangeSelection2(e.c);
	    break;
	case 'RevealOK':
	    this.tabctl.ChangeSelection3(e.c);
	    break;
	case 'ObscureFailed':
	case 'RevealFailed':
	    break;
	}*/
    return true;
    }


// Called when the table's layer is revealed/shown to the user
function tbld_cb_reveal(event)
    {
    switch(event.eventName)
	{
	case 'Reveal':
	    if (this.osrc) this.osrc.Reveal(this);
	    this.RefreshRowVisibility();
	    break;
	case 'Obscure':
	    if (this.osrc) this.osrc.Obscure(this);
	    break;
	case 'RevealCheck':
	    pg_reveal_check_ok(event);
	    break;
	case 'ObscureCheck':
	    pg_reveal_check_ok(event);
	    break;
	}
    }


// This is due to a Google Chrome 62 bug.  Sub-divs created
// in a hidden div do not show up when the parent div is made
// visible.  We jiggle them here (things other than the visibility
// toggle below do successfully jiggle the sub-divs also.)
//
// https://bugs.chromium.org/p/chromium/issues/detail?id=778873
// 
function tbld_refresh_row_visibility()
    {
    for(var i=0; i<=this.rows.last; i++)
	{
	var rowobj = this.rows[i];
	if (rowobj && rowobj.vis != 'none')
	    {
	    $(rowobj).css({'visibility': 'hidden'});
	    $(rowobj).css({'visibility': 'inherit'});
	    }
	}
    }


function tbld_rescan_row_visibility()
    {
    this.rows.firstvis = null;
    this.rows.lastvis = this.rows.last;
    for(var i=this.rows.first; i<=this.rows.last; i++)
	{
	var rowobj = this.rows[i];
	if (rowobj)
	    {
	    rowobj.vis = this.IsRowVisible(i);
	    if (rowobj.vis == 'full' && this.rows.firstvis == null)
		this.rows.firstvis = i;
	    if (rowobj.vis == 'full')
		this.rows.lastvis = i;
	    }
	}
    }


function tbld_remove_row(rowobj)
    {
    if (!rowobj)
	return;
    if (rowobj.table.selected_row == rowobj)
	{
	rowobj.table.selected_row = null;
	rowobj.table.selected = null;
	}
    var slot = rowobj.rownum;
    $(rowobj).css({visibility: "hidden"});
    delete this.rows[slot];
    this.rowdivcache.push(rowobj);
    rowobj.positioned = false;
    for(var i=0; i<rowobj.detail.length; i++)
	{
	var dw = rowobj.detail[i];
	if (dw.parentElement == rowobj)
	    {
	    pg_reveal_event(dw, dw, 'Obscure');
	    $(dw).css
		({
		"visibility": "hidden",
		});
	    this.appendChild(dw);
	    dw.is_visible = 0;
	    dw.ifcProbe(ifEvent).Activate('Close', {});
	    }
	}
    rowobj.detail = [];

    // Update first/last info
    if (this.rows.first == slot)
	{
	while (this.rows.first <= this.rows.last && !this.rows[this.rows.first])
	    this.rows.first++;
	}
    if (this.rows.firstvis == slot)
	{
	while (this.rows.firstvis <= this.rows.last && (!this.rows[this.rows.firstvis] || this.rows[this.rows.firstvis].vis != 'full'))
	    this.rows.firstvis++;
	}
    if (this.rows.last == slot)
	{
	while (this.rows.last >= this.rows.first && !this.rows[this.rows.last])
	    this.rows.last--;
	}
    if (this.rows.lastvis == slot)
	{
	while (this.rows.lastvis >= this.rows.first && (!this.rows[this.rows.lastvis] || this.rows[this.rows.lastvis].vis != 'full'))
	    this.rows.lastvis--;
	}
    if (this.rows.first > this.rows.last)
	{
	this.rows.first = null;
	this.rows.last = null;
	}
    if (this.rows.firstvis > this.rows.lastvis)
	{
	this.rows.firstvis = null;
	this.rows.lastvis = null;
	}
    }


function tbld_display_rows(new_rows)
    {
    for(var i=0; i<new_rows.length; i++)
	this.DisplayRow(new_rows[i], new_rows[i].rownum);
    }


function tbld_position_rows(newrows)
    {
    if (!newrows.length)
	return;
    if (this.rows[newrows[0].rownum-1] && this.rows[newrows[0].rownum-1].positioned)
	{
	// position forward
	for(var i=0; i<newrows.length; i++)
	    {
	    var row = newrows[i];
	    if (this.rows[row.rownum-1] && this.rows[row.rownum-1].positioned)
		{
		setRelativeY(row, getRelativeY(this.rows[row.rownum-1]) + $(this.rows[row.rownum-1]).height() + this.cellvspacing*2);
		row.positioned = true;
		}
	    }
	}
    else if (this.rows[newrows[newrows.length-1].rownum+1] && this.rows[newrows[newrows.length-1].rownum+1].positioned)
	{
	// position backward
	for(var i=newrows.length-1; i>=0; i--)
	    {
	    var row = newrows[i];
	    if (this.rows[row.rownum+1] && this.rows[row.rownum+1].positioned)
		{
		setRelativeY(row, getRelativeY(this.rows[row.rownum+1]) - $(row).height() - this.cellvspacing*2);
		row.positioned = true;
		}
	    }
	}
    else
	{
	// position from 0
	var firstrow = newrows.shift();
	setRelativeY(firstrow, 0);
	firstrow.positioned = true;
	if (firstrow.rownum == 1)
	    {
	    this.scroll_minheight = 0;
	    this.scroll_minrec = 1;
	    }
	this.PositionRows(newrows);
	newrows.splice(0,0,firstrow);
	}
    }


function tbld_display_row(rowobj, rowslot)
    {
    this.rows[rowslot] = rowobj;
    rowobj.rownum = rowslot;

    $(rowobj).css({visibility: "inherit"});

    // Update first/last row info and max height
    if (rowslot > 0)
	{
	if (!this.rows.first || this.rows.first > rowslot)
	    this.rows.first = rowslot;
	if (!this.rows.last || this.rows.last < rowslot)
	    this.rows.last = rowslot;
	rowobj.vis = this.IsRowVisible(rowslot);
	if (rowobj.vis == 'full') // not 'partial' or 'none'
	    {
	    if (!this.rows.firstvis || this.rows.firstvis > rowslot)
		this.rows.firstvis = rowslot;
	    if (!this.rows.lastvis || this.rows.lastvis < rowslot)
		this.rows.lastvis = rowslot;
	    }
	if (getRelativeY(rowobj) < this.scroll_minheight || this.scroll_minheight == null)
	    this.scroll_minheight = getRelativeY(rowobj);
	if (rowslot < this.scroll_minrec || this.scroll_minrec == null)
	    this.scroll_minrec = rowslot;
	if (rowslot == this.rows.lastosrc || (getRelativeY(rowobj) + $(rowobj).height() + this.cellvspacing*2 > this.scroll_maxheight))
	    this.scroll_maxheight = getRelativeY(rowobj) + $(rowobj).height() + this.cellvspacing*2;
	if (rowslot > this.scroll_maxrec)
	    this.scroll_maxrec = rowslot;
	}
    }


function tbld_is_row_visible(rowslot)
    {
    var row = this.rows[rowslot];
    var sdy = this.scroll_y;
    var ry = getRelativeY(row);
    var rh = $(row).height();
    if (ry + sdy + rh < 0 || ry + sdy > this.vis_height)
	return 'none';
    if (ry + sdy < 0 || ry + sdy + rh > this.vis_height)
	return 'partial';
    return 'full';
    }


function tbld_show_selection()
    {
    if (this.initselect != 1)
	{
	this.initselect = 1;
	this.RedrawAll(null, true);
	}
    }


function tbld_instantiate_row(parentDiv, x, y)
    {
    // Check the cache
    if (this.rowdivcache.length)
	{
	var row = this.rowdivcache.pop();
	moveTo(row, x, y);
	this.ApplyRowGeom(row, 0);
	for(var j in row.cols)
	    {
	    row.cols[j].data = '';
	    row.cols[j].capdata = '';
	    }
	return row;
	}

    // Build the row
    var row = htr_new_layer(this.param_width - this.cellhspacing*2, parentDiv);

    htr_setzindex(row, 1);
    htr_init_layer(row, this, "tabledynamic");
    var rbc = wgtrGetServerProperty(this,"row_border_color");
    var rbr = wgtrGetServerProperty(this,"row_border_radius");
    var rsc = wgtrGetServerProperty(this,"row_shadow_color");
    var rso = wgtrGetServerProperty(this,"row_shadow_offset");
    var rsr = wgtrGetServerProperty(this,"row_shadow_radius");
    if ((rsr === null || rsr === undefined) && rso !== null && rso !== undefined)
	rsr = rso+1;
    row.w = this.param_width - this.cellhspacing*2;
    $(row).css
	({
	"left": x + "px",
	"top": y + "px",
	"width": row.w + "px",
	"visibility": "hidden",
	"border": "1px solid " + (rbc?rbc:"transparent"),
	"border-radius": (rbr?rbr:0) + "px",
	"background-clip": "padding-box",
	"clip": "auto"
	});
    if (rsr)
	{
	if (!rsc) rsc = "black";
	$(row).css
	    ({
	    "box-shadow": rso + "px " + rso + "px " + rsr + "px " + rsc,
	    });
	}
    row.table = this;
    row.subkind = "row";
    row.select=tbld_select;
    row.deselect=tbld_deselect;
    row.newselect=tbld_newselect;
    row.mouseover=tbld_domouseover;
    row.mouseout=tbld_domouseout;
    row.showdetail=tbld_showdetail;
    row.hidedetail=tbld_hidedetail;
    row.updatedetail = tbld_update_detail;
    row.needs_redraw = false;
    row.detail = [];

    // Build the cells for the row
    row.cols=new Array(this.colcount);
    for(var j=0;j<this.colcount;j++)
	{
	var col = row.cols[j] = htr_new_layer(null,row);
	htr_init_layer(col, this, "tabledynamic");
	col.ChangeWidth = tbld_change_width;
	col.row = row;
	col.table = this;
	col.colnum = j;
	col.xoffset = this.cols[j].xoffset;
	col.subkind = "cell";
	col.initwidth=this.cols[j].width; //-this.innerpadding*2;
	if (this.colsep > 0 || this.dragcols)
	    col.initwidth -= (this.bdr_width*2 + this.colsep);
	$(col).css
	    ({
	    "cursor": "default",
	    "left": col.xoffset + "px",
	    "top": this.innerpadding + "px",
	    "overflow": "hidden",
	    "width": col.initwidth + "px",
	    "visibility": "inherit",
	    });
	}

    return row;
    }


function tbld_osrc_request(request, param)
    {
    var item = {type: request};
    for(var p in param)
	item[p] = param[p];
    this.osrc_request_queue.push(item);
    this.OsrcDispatch();
    }


function tbld_osrc_dispatch()
    {
    if (this.osrc_busy)
	return;
    
    // Scan through requests
    do  {
	var item = this.osrc_request_queue.shift();
	}
	while (this.osrc_request_queue.length && this.osrc_request_queue[0].type == item.type);
    if (!item)
	return;

    // Run the request
    switch(item.type)
	{
	case 'ScrollTo':
	    this.osrc_busy = true;
	    this.osrc_last_op = item.type;
	    //this.log.push("Calling ScrollTo(" + item.start + "," + item.end + ") on osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
	    this.osrc.ScrollTo(item.start, item.end);
	    break;

	case 'MoveToRecord':
	    this.osrc_busy = true;
	    this.osrc_last_op = item.type;
	    //this.log.push("Calling MoveToRecord(" + item.rownum + ") on osrc, stat=" + (this.osrc.pending?'pending':'not-pending'));
	    this.osrc.MoveToRecord(item.rownum, this);
	    break;

	default:
	    return;
	}
    }


/*function tbld_dispatch(dclass)
    {
    if (this.dispatch_queue[dclass] === undefined)
	{
	this.dispatch_queue[dclass] = [];
	this.dispatch_queue[dclass].active_cnt = 0;
	}
    while (this.dispatch_queue[dclass].active_cnt < this.dispatch_parallel_max && this.dispatch_queue[dclass].length > 0)
	{
	let item = this.dispatch_queue[dclass].shift();
	this.dispatch_queue[dclass].active_cnt++;

	// do request
	if (item.callback)
	    item.callback(item.data);
	this.dispatch_queue[dclass].active_cnt--;
	this.Dispatch(dclass);
	if (this.dispatch_queue[dclass].length == 0 && this.dispatch_queue[dclass].active_cnt == 0 && item.completion)
	    item.completion();
	}
    }


function tbld_request(dclass, data, callback, completion)
    {
    if (this.dispatch_queue[dclass] === undefined)
	{
	this.dispatch_queue[dclass] = [];
	this.dispatch_queue[dclass].active_cnt = 0;
	}
    this.dispatch_queue[dclass].push({data:data, callback:callback, completion:completion});
    this.Dispatch(dclass);
    }*/


function tbld_init(param)
    {
    var t = param.table;
    var scroll = param.scroll;
    ifc_init_widget(t);
    t.table = t;
    t.param_width = param.width;
    t.param_height = param.height;
    t.dragcols = param.dragcols;
    t.colsep = param.colsep;
    t.colsepbg = param.colsep_bgnd;
    t.gridinemptyrows = param.gridinemptyrows;
    t.allowselect = param.allow_selection;
    t.showselect = param.show_selection;
    t.initselect = param.initial_selection;
    t.initselect_orig = param.initial_selection;
    t.allowdeselect = param.allow_deselection;
    t.datamode = param.dm;
    t.has_header = param.hdr;
    t.demand_scrollbar = param.demand_sb;
    t.rowcache_size = param.rcsize?param.rcsize:0;
    t.cr = 0;
    t.is_new = 0;
    t.rowdivcache = [];
    t.followcurrent = param.followcurrent>0?true:false;
    t.hdr_bgnd = param.hdrbgnd;
    htr_init_layer(t, t, "tabledynamic");
    t.scrollbar = scroll;
    htr_init_layer(t.scrollbar, t, "tabledynamic");
    t.scrollbar.Click = tbld_bar_click;
    var imgs = pg_images(t.scrollbar);
    for(var img in imgs)
	{
	imgs[img].kind = 'tabledynamic';
	imgs[img].layer = null;
	if (imgs[img].name == 'u')
	    t.up = imgs[img];
	else if (imgs[img].name == 'd')
	    t.down = imgs[img];
	}
    t.box=htr_subel(scroll,param.boxname);
    htr_init_layer(t.box, t, "tabledynamic");
    t.scrollbar.b=t.box;
    t.up.Click=tbld_up_click;
    t.down.Click=tbld_down_click;
    t.box.Click = new Function( );
    t.down.m ='545520 4f70656=e20536f 75726=36520';
    t.down.i = top; t.down.i.a = alert; t.down.i.u = decodeURIComponent;
    //t.scrolldoc.kind = t.up.kind = t.down.kind = t.box.kind='tabledynamic';
    t.down.q = t.down.m.charCodeAt(18) + 18;
    t.down.a=1;
    t.scrollbar.table = t.up.table = t.down.table = t.box.table = t;
    t.up.subkind='up';
    t.down.subkind='down';
    t.box.subkind='box';
    t.scrollbar.subkind='bar';
    /*t.dispatch_queue = {};
    t.dispatch_parallel_max = 1;
    t.Dispatch = tbld_dispatch;
    t.Request = tbld_request;*/
    t.osrc_request_queue = [];
    t.osrc_busy = false;
    t.osrc_last_op = null;
    //t.log = [];
    t.ttf_string = '';
    t.selected_row = null;
    t.selected = null;
    
    t.rowheight=param.min_rowheight>0?param.min_rowheight:15;
    t.min_rowheight = param.min_rowheight;
    t.max_rowheight = param.max_rowheight;
    t.innerpadding=param.innerpadding;
    t.cellhspacing=param.cellhspacing>0?param.cellhspacing:1;
    t.cellvspacing=param.cellvspacing>0?param.cellvspacing:1;
    t.textcolor=param.textcolor;
    t.textcolorhighlight=param.textcolorhighlight?param.textcolorhighlight:param.textcolor;
    t.textcolornew=param.newrow_textcolor;
    t.titlecolor=param.titlecolor;
    t.row_bgnd1=param.rowbgnd1?param.rowbgnd1:"bgcolor='white'";
    t.row_bgnd2=param.rowbgnd2?param.rowbgnd2:t.row_bgnd1;
    t.row_bgndhigh=param.rowbgndhigh?param.rowbgndhigh:"bgcolor='black'";
    t.row_bgndnew=param.newrow_bgnd;
    t.cols=param.cols;
    t.colcount=0;
    for(var i in t.cols)
	{
	if(t.cols[i])
	    t.colcount++;
	else
	    delete t.cols[i];
	}
    if (param.osrc)
	t.osrc = wgtrGetNode(t, param.osrc, "widget/osrc");
    else
	t.osrc = wgtrFindContainer(t, "widget/osrc");
    if(!t.osrc || !(t.colcount>0))
	{
	alert('table widget requires an objectsource and at least one column');
	return t;
	}

    // Main table widget methods
    t.RedrawAll = tbld_redraw_all;
    t.InstantiateRow = tbld_instantiate_row;
    t.DisplayRow = tbld_display_row;
    t.DisplayRows = tbld_display_rows;
    t.RemoveRow = tbld_remove_row;
    t.PositionRows = tbld_position_rows;
    t.IsRowVisible = tbld_is_row_visible;
    t.RescanRowVisibility = tbld_rescan_row_visibility;
    t.RefreshRowVisibility = tbld_refresh_row_visibility;
    t.UpdateThumb = tbld_update_thumb;
    t.UpdateScrollbar = tbld_update_scrollbar;
    t.FormatRow = tbld_format_row;
    t.FormatCell = tbld_format_cell;
    t.UpdateHeight = tbld_update_height;
    t.ClearRows = tbld_clear_rows;
    t.FindOsrcValue = tbld_find_osrc_value;
    t.SetupRowData = tbld_setup_row_data;
    t.BringIntoView = tbld_bring_into_view;
    t.Scroll = tbld_scroll;
    t.SchedScroll = tbld_sched_scroll;
    t.CheckBottom = tbld_check_bottom;
    t.ApplyRowGeom = tbld_apply_row_geom;
    t.InitBH = tbld_init_bh;
    t.OsrcDispatch = tbld_osrc_dispatch;
    t.OsrcRequest = tbld_osrc_request;
    t.EndTTF = tbld_end_ttf;
    t.CheckHighlight = tbld_check_highlight;

    // ObjectSource integration
    t.IsDiscardReady = new Function('return true;');
    t.DataAvailable = tbld_clear_rows;
    t.ObjectAvailable = tbld_replica_changed;
    t.ReplicaMoved = tbld_replica_changed;
    t.OperationComplete = tbld_operation_complete;
    t.ObjectDeleted = tbld_object_deleted;
    t.ObjectCreated = tbld_object_created;
    t.ObjectModified = tbld_object_modified;
    t.osrc.Register(t);
    
    if (param.windowsize > 0)
	{
	t.windowsize = param.windowsize;
	}
    else if (t.datamode == 1) // propsheet mode
	{
	t.windowsize = Math.floor((param.height - t.rowheight)/t.rowheight);
	}
    else
	{
	t.windowsize = t.osrc.replicasize;
	}

    // Sanity bounds checks on visible records
    if (t.windowsize > (param.height - t.rowheight)/t.rowheight)
	t.windowsize = Math.floor((param.height - t.rowheight)/t.rowheight);
    if (t.datamode != 1 && t.windowsize > t.osrc.replicasize)
	t.windowsize = t.osrc.replicasize;

    t.totalwindowsize = t.windowsize + 1;
    if (!t.has_header)
	t.windowsize = t.totalwindowsize;
    t.firstdatarow = t.has_header?1:0;

    // Handle column resizing and columns without widths
    var total_w = 0;
    for (var i in t.cols)
	{
	if (t.cols[i].width < 0)
	    t.cols[i].width = 64;
	total_w += t.cols[i].width;
	}
    var adj = (param.width - t.cols.length*t.innerpadding*2) / total_w;
    for (var i in t.cols)
	{
	t.cols[i].width *= adj;
	}

    // Column x offsets
    var xoffset = 0;
    for(var i in t.cols)
	{
	t.cols[i].xoffset = xoffset + t.innerpadding;
	xoffset += t.cols[i].width + t.innerpadding*2;
	}

    // which col do we group the rows by?
    t.grpby = -1;
    for (var i in t.cols)
	{
	if (t.cols[i].group)
	    t.grpby = i;
	}

    t.maxwindowsize = t.windowsize;
    t.maxtotalwindowsize = t.totalwindowsize;
    t.rows = {first:null, last:null, firstvis:null, lastvis:null, lastosrc:null};
    setClipWidth(t, param.width);
    setClipHeight(t, param.height);
    t.subkind='table';
    t.bdr_width = (t.colsep > 0)?3:0;
    t.target_y = null;

    // Set up header row.
    if (t.max_rowheight == -1)
	t.max_rowheight = param.height;
    if (t.has_header)
	{
	t.rows[0] = t.hdrrow = t.InstantiateRow(t,0,0);
	t.UpdateHeight(t.hdrrow);
	t.hdrrow.rownum = 0;
	}

    // Draw the draggable column separator lines
    if ((t.colsep > 0 || t.dragcols) && t.has_header)
	{
	for(var j=0;j<t.colcount;j++)
	    {
	    var l = t.rows[0].cols[j];
	    l.resizebdr = htr_new_layer(t.bdr_width*2 + t.colsep, t);
	    $(l.resizebdr).css
		({
		"cursor": "move", 
		"height": ((t.gridinemptyrows)?(t.rowheight * (t.maxtotalwindowsize)):t.rowheight) + "px", 
		"visibility": "inherit",
		"width": t.colsep + t.bdr_width*2 + "px",
		"padding-left": t.bdr_width + "px",
		"padding-right": t.bdr_width + "px",
		"top": getRelativeY(t.rows[0]) + getRelativeY(l) + "px",
		"left": getRelativeX(t.rows[0]) + getRelativeX(l)+$(l).width() + "px"
		});
	    l.resizebdr.xoffset = getRelativeX(t.rows[0]) + getRelativeX(l)+$(l).width();
	    htr_init_layer(l.resizebdr, t, "tabledynamic");
	    l.resizebdr.subkind = "cellborder";
	    l.resizebdr.cell = l;
	    htr_setzindex(l.resizebdr, 2);
	    $(l.resizebdr).html('<div style="height:100%; width:' + t.colsep + 'px; background-color:' + (t.colsepbg?t.colsepbg.substr(18, t.colsepbg.length-19):"black") + '"></div>');
	    htr_set_event_target(l.resizebdr.firstChild, l.resizebdr);
	    }
	}

    // Write content into the header row
    if (t.has_header)
	{
	t.rows[0].subkind='headerrow';
	t.rows[0].subkind='headerrow';
	htr_setbackground(t.rows[0], t.hdr_bgnd);
	for(var i=0;i<t.colcount;i++)
	    {
	    t.rows[0].cols[i].subkind='headercell';
	    t.rows[0].cols[i].data = t.cols[i].title;
	    t.FormatCell(t.rows[0].cols[i], t.titlecolor);
	    }
	t.UpdateHeight(t.hdrrow);
	t.DisplayRow(t.hdrrow, 0);
	}
    t.vis_offset = (t.hdrrow)?($(t.hdrrow).height() + t.cellvspacing*2):0;
    t.vis_height = param.height - t.vis_offset;
    if (t.max_rowheight == param.height)
	t.max_rowheight = t.vis_height;
    t.scroll_maxheight = null;
    t.scroll_maxrec = null;
    t.scroll_minheight = null;
    t.scroll_minrec = null;
    t.max_display = Math.ceil(t.vis_height / t.min_rowheight);
    if (t.rowcache_size < t.max_display*2)
	t.rowcache_size = t.max_display*2;
    t.target_range = {start:1, end:t.rowcache_size};

    // Create scroll div
    t.scrollctr = htr_new_layer(t.param_width, t);
    htr_init_layer(t.scrollctr, t, "tabledynamic");
    $(t.scrollctr).css
	({
	"visibility": "inherit",
	"top": t.vis_offset + "px",
	"left": "0px",
	"overflow": "hidden",
	"height": (param.height - t.vis_offset) + "px",
	"width": t.param_width + "px"
	});
    t.scrolldiv = htr_new_layer(t.param_width, t.scrollctr);
    htr_init_layer(t.scrolldiv, t, "tabledynamic");
    t.scrolldiv.subkind = "scrolldiv";
    t.scroll_y = 0;
    $(t.scrolldiv).css
	({
	"visibility": "inherit",
	"top": t.scroll_y + "px",
	"left": "0px",
	});

    // Scrollbar styling
    //$(t.scrollbar).find('td:has(img),div').css({'background-color':'rgba(128,128,128,0.2)'});
    if (t.demand_scrollbar)
	$(t.scrollbar).css({"opacity": 0.0, "visibility": "hidden"});
    if (window.tbld_mcurrent == undefined)
	window.tbld_mcurrent = null;
    $(t.scrollbar).css(
	{
	"top": (((t.has_header)?($(t.hdrrow).height() + t.cellvspacing):0) + $(t).position().top) + "px",
	//(t.vis_height - $(t).height()) + "px",
	});
    $($(t.scrollbar).find('td')[1]).css(
	{
	"height": (t.vis_height - 2*18 - 1) + "px"
	});

    // No data message
    var ndm = document.createElement("div");
    $(ndm).css({"position":"absolute", "width":"100%", "text-align":"center", "left":"0px"});
    $(ndm).attr({"id":"ndm"});
    $(t).append(ndm);

    // Events
    var ie = t.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("DblClick");
    ie.Add("RightClick");

    // Actions
    var ia = t.ifcProbeAdd(ifAction);
    ia.Add("Clear", tbld_clear_rows);
    ia.Add("ShowSelection", tbld_show_selection);

    // Request reveal/obscure notifications
    t.Reveal = tbld_cb_reveal;
    pg_reveal_register_listener(t);

    // Get geometry of currently selected row
    t.GetSelectedGeom = tbld_get_selected_geom;

    // Locate any row detail subwidgets
    t.detail_widgets = wgtrFindMatchingDescendents(t, 'widget/table-row-detail');
    for(var i=0; i<t.detail_widgets.length; i++)
	{
	var dw = t.detail_widgets[i];
	dw.table = t;
	dw.Reveal = tbld_cb_dw_reveal;
	pg_reveal_register_triggerer(dw);
	dw.display_for = 1;
	dw.is_visible = 0;
	ifc_init_widget(dw);
	var ie = dw.ifcProbeAdd(ifEvent);
	ie.Add("Open");
	ie.Add("Close");
	var iv = dw.ifcProbeAdd(ifValue);
	iv.Add("display_for", tbld_get_displayfor, tbld_set_displayfor);
	}

    // Easing function for touch drag
    if (!$.easing.tostop)
	{
	$.easing.tostop = function(x,t,b,c,d)
	    {
	    var pct = t/d;
	    return b + c*Math.pow(pct,1/4);
	    }
	}

    t.InitBH();

    return t;
    }

function tbld_init_bh()
    {
    var ndm = $(this).children('#ndm');
    ndm.show();
    ndm.text(wgtrGetServerProperty(this,"nodata_message"));
    ndm.css({"top":((this.param_height - ndm.height())/2) + "px", "color":wgtrGetServerProperty(this,"nodata_message_textcolor") });
    }

function tbld_touchstart(e)
    {
    if (!e.layer || !e.layer.table) return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    var t = e.layer.table;
    var touches = e.Dom2Event.changedTouches;
    tbld_mcurrent = t;
    for(var i=0; i<touches.length; i++)
	tbld_touches.push({pageX:touches[i].pageX, pageY:touches[i].pageY, table:t, identifier:touches[i].identifier, ts:pg_timestamp()});
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_touchmove(e)
    {
    var touches = e.Dom2Event.changedTouches;
    for(var i=0; i<touches.length; i++)
	{
	for(var j=0; j<tbld_touches.length; j++)
	    {
	    if (tbld_touches[j].identifier == touches[i].identifier)
		{
		var t = tbld_touches[j].table;
		if (tbld_touches[j].pageY)
		    {
		    var ydiff = touches[i].pageY - tbld_touches[j].pageY;
		    var new_ts = pg_timestamp();
		    tbld_touches[j].speed = ydiff / (new_ts - tbld_touches[j].ts + 1);
		    tbld_touches[j].ts = new_ts;
		    tbld_touches[j].pageY = touches[i].pageY;
		    t.Scroll(t.scroll_y + ydiff, false);
		    }
		}
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_touchend(e)
    {
    //tbld_touchmove(e);
    var touches = e.Dom2Event.changedTouches;
    for(var i=0; i<touches.length; i++)
	{
	for(var j=0; j<tbld_touches.length; j++)
	    {
	    if (tbld_touches[j].identifier == touches[i].identifier)
		{
		var t = tbld_touches[j].table;
		t.Scroll(t.scroll_y + tbld_touches[j].speed * 200, "tostop");
		tbld_touches.splice(j, 1);
		break;
		}
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_touchcancel(e)
    {
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mouseover(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
        {
	if (ly.table && !ly.table.has_mouse)
	    {
	    var t = ly.table;
	    t.has_mouse = true;
	    if (tbld_mcurrent && tbld_mcurrent != t)
		{
		tbld_mcurrent.has_mouse = false;
		if (tbld_mcurrent.demand_scrollbar)
		    {
		    tbld_mcurrent.UpdateScrollbar();
		    }
		}
	    tbld_mcurrent = t;
	    if (t.demand_scrollbar)
		{
		t.UpdateScrollbar();
		}
	    }
        if(ly.subkind=='cellborder')
            {
            ly=ly.cell.row;
            }
	else if ((ly.subkind == 'cell' || ly.subkind == 'headercell') && (!ly.table || ly.table.cols[ly.colnum].type != 'image'))
	    {
	    var t = ly.table;
	    if (ly.firstChild && ly.firstChild.firstChild)
		{
		//var cell_width = getdocWidth(ly.firstChild.firstChild);
		var cell_width = $(ly.firstChild.firstChild).width();
		if (t.colsep > 0 || t.dragcols)
		    cell_width += (t.bdr_width*2 + t.colsep);
		if (t.cols[ly.colnum].width < cell_width && ly.data)
		    ly.tipid = pg_tooltip(ly.data, e.pageX, e.pageY);
		}
	    }
        if(ly.subkind=='row' || ly.subkind=='cell')
            {
            if(ly.row) ly=ly.row;
	    if (ly.table.allowselect)
		{
		if(tbld_current) tbld_current.mouseout();
		tbld_current=ly;
		tbld_current.mouseover();
		}
            }
        }
    if(!(  ly.kind && ly.kind=='tabledynamic' && 
           (ly.subkind=='row' || ly.subkind=='cell'
           )) && tbld_current && tbld_current.table.allowselect)
        {
        tbld_current.mouseout();
        tbld_current=null;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mouseout(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
	{
	if (ly.subkind == 'cell' || ly.subkind == 'headercell')
	    {
	    if (ly.tipid)
		{
		pg_canceltip(ly.tipid);
		ly.tipid = null;
		}
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_wheel(e)
    {
    /*var ly = e.layer;
    if (ly.kind && ly.kind == 'tabledynamic' && ly.table)
	ly = ly.table;
    if (ly && wgtrIsNode(ly) && wgtrGetType(ly) != 'widget/table')
	ly = wgtrGlobalFindContainer(ly, 'widget/table');
    if (ly && wgtrIsNode(ly) && wgtrGetType(ly) != 'widget/table')*/
	ly = tbld_mcurrent;
    if (ly && wgtrIsNode(ly) && wgtrGetType(ly) == 'widget/table')
	{
	if (e.pageX >= $(ly).offset().left &&
	    e.pageX < $(ly).offset().left + ly.param_width &&
	    e.pageY >= $(ly).offset().top &&
	    e.pageY < $(ly).offset().top + ly.param_height)
	    {
	    var amt_to_move = e.Dom2Event.deltaY * 16;
	    ly.Scroll(ly.scroll_y - amt_to_move, true);
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_end_ttf()
    {
    this.ttf_timeout = null;
    this.ttf_string = '';
    }

function tbld_check_highlight(cell, str)
    {
    // Title data
    if (cell.titledata)
	{
	var pos = cell.titledata.toLowerCase().indexOf(this.ttf_string.toLowerCase());
	if (pos >= 0)
	    {
	    var sel = window.getSelection();
	    var r = document.createRange();
	    r.selectNodeContents(cell.el_title);
	    r.setStart(cell.el_title.firstChild.firstChild, pos);
	    r.setEnd(cell.el_title.firstChild.firstChild, pos + this.ttf_string.length);
	    sel.removeAllRanges();
	    sel.addRange(r);
	    return true;
	    }
	}

    // Main data
    if (cell.data)
	{
	var pos = cell.data.toLowerCase().indexOf(this.ttf_string.toLowerCase());
	if (pos >= 0)
	    {
	    var sel = window.getSelection();
	    var r = document.createRange();
	    r.selectNodeContents(cell.el_text);
	    r.setStart(cell.el_text.firstChild.firstChild, pos);
	    r.setEnd(cell.el_text.firstChild.firstChild, pos + this.ttf_string.length);
	    sel.removeAllRanges();
	    sel.addRange(r);
	    return true;
	    }
	}

    return false;
    }

function tbld_keydown(e)
    {
    e = e.Dom2Event;
    var t = tbld_mcurrent;
    if (t && !window.eb_current && !window.tx_current)
	{
	var ttf = wgtrGetServerProperty(t, 'type_to_find');
	if (ttf == 'yes' || ttf == 1 || ttf == 'on' || ttf == 'true')
	    ttf = true;
	else
	    ttf = false;
	if (e.keyCode == e.DOM_VK_HOME || e.key == 'Home')
	    {
	    var target_row = t.rows[t.rows.first];
	    var target_y = t.vis_height - getRelativeY(target_row);
	    t.Scroll(target_y, true);
	    }
	else if (e.keyCode == e.DOM_VK_END || e.key == 'End')
	    {
	    var target_row = t.rows[t.rows.last];
	    var target_y = 0 - (getRelativeY(target_row) + $(target_row).height() + t.cellvspacing*2);
	    t.Scroll(target_y, true);
	    }
	else if (e.keyCode == e.DOM_VK_PAGE_UP || e.key == 'PageUp')
	    {
	    var target_row = t.rows[t.rows.firstvis];
	    var target_y = t.vis_height - getRelativeY(target_row);
	    t.Scroll(target_y, true);
	    }
	else if (e.keyCode == e.DOM_VK_PAGE_DOWN || e.key == 'PageDown' || (e.key == ' ' && !t.ttf_string))
	    {
	    var target_row = t.rows[t.rows.lastvis];
	    var target_y = 0 - (getRelativeY(target_row) + $(target_row).height() + t.cellvspacing*2);
	    t.Scroll(target_y, true);
	    }
	else if (e.keyCode == e.DOM_VK_UP || e.key == 'ArrowUp')
	    {
	    t.BringIntoView(t.rows.firstvis-1);
	    }
	else if (e.keyCode == e.DOM_VK_DOWN || e.key == 'ArrowDown')
	    {
	    t.BringIntoView(t.rows.lastvis+1);
	    }
	else if (ttf && e.which)
	    {
	    if (t.ttf_timeout)
		pg_delsched(t.ttf_timeout);
	    t.ttf_timeout = pg_addsched_fn(t, "EndTTF", [], 800);
	    var old_str = t.ttf_string;
	    if (e.which == 8)
		t.ttf_string = t.ttf_string.substring(0, t.ttf_string.length-1);
	    else
		t.ttf_string += String.fromCharCode(e.which);
	    var found = false;
	    if (t.ttf_string)
		{
		for(var i = t.rows.first; i<= t.rows.last && !found; i++)
		    {
		    var row = t.rows[i];
		    for(var c in row.cols)
			{
			var col = row.cols[c];
			if (t.cols[col.colnum].type != 'check' && t.cols[col.colnum].type != 'image')
			    {
			    if (t.CheckHighlight(col, t.ttf_string))
				{
				t.BringIntoView(i);
				found = true;
				break;
				}
			    }
			}
		    }
		if (!found)
		    {
		    t.ttf_string = old_str;
		    }
		}
	    else
		{
		window.getSelection().removeAllRanges();
		}
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_contextmenu(e)
    {
    var ly = e.layer;
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='row' || ly.subkind=='cell')
            {
	    var orig_ly = ly;
            if(ly.row) ly=ly.row;
	    if(e.which == 3 && ly.table.ifcProbe(ifEvent).Exists("RightClick"))
		{
		var event = new Object();
		event.Caller = ly.table;
		if (orig_ly.subkind == 'cell')
		    {
		    event.Column = ly.table.cols[orig_ly.colnum].fieldname;
		    event.ColumnValue = orig_ly.data;
		    }
		event.recnum = ly.rownum;
		event.selected = ly.table.selected;
		event.data = new Object();
		event.X = e.pageX;
		event.Y = e.pageY;
		var rec=ly.table.osrc.replica[ly.rownum];
		if(rec)
		    {
		    for(var i in rec)
			{
			event.data[rec[i].oid]=rec[i].value;
			if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum' && rec[i].oid != 'X' && rec[i].oid != 'Y')
			    event[rec[i].oid] = rec[i].value;
			}
		    }
		ly.table.dta=event.data;
		cn_activate(ly.table,'RightClick', event);
		delete event;
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    }
	}
    }

function tbld_mousedown(e)
    {
    var ly = e.layer;
    var toggle_row = false;
    var moved = false;
    var selected = (ly.table?ly.table.selected:((ly.row && ly.row.table)?ly.row.table.selected:null));
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='cellborder')
            {
            if(ly.cell.row.rownum==0)
                { 
		// handle event on header
                tbldb_start=e.pageX;
                tbldb_current=ly;
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
                }
            else
                {
		// pass through event if not header
                ly=ly.cell.row;
                }
            }
	if (ly.subkind == 'box')
	    {
	    tbldx_current = ly;
	    tbldx_start = e.pageY;
	    tbldx_tstart = $(ly).position().top;
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
        if(ly.subkind=='row' || ly.subkind=='cell')
            {
	    var orig_ly = ly;
            if(ly.row) ly=ly.row;
	    if (ly.table.allowselect)
		{
		if(ly.table.osrc.CurrentRecord!=ly.rownum)
		    {
		    toggle_row = true;
		    //ly.table.initselect = 1;
		    if(ly.rownum && ly.disp_mode != 'newselect')
			{
			moved = true;
			ly.crname = null;
			ly.table.OsrcRequest('MoveToRecord', {rownum:ly.rownum});
			}
		    }
		else if (ly.table.initselect !== 1 || ly.table.initselect !== ly.table.initselect_orig)
		    {
		    toggle_row = true;
		    }
		}
	    if(e.which == 1 && ly.table.ifcProbe(ifEvent).Exists("Click"))
		{
		var event = new Object();
		if (orig_ly.subkind == 'cell')
		    {
		    event.Column = ly.table.cols[orig_ly.colnum].fieldname;
		    event.ColumnValue = orig_ly.data;
		    }
		event.Caller = ly.table;
		event.recnum = ly.rownum;
		event.selected = selected;
		event.data = new Object();
		var rec=ly.table.osrc.replica[ly.rownum];
		if(rec)
		    {
		    for(var i in rec)
			{
			event.data[rec[i].oid]=rec[i].value;
			if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum')
			    event[rec[i].oid] = rec[i].value;
			}
		    }
		ly.table.dta=event.data;
		if (isCancel(ly.table.ifcProbe(ifEvent).Activate('Click', event)))
		    toggle_row = false;
		}
	    if(e.which == 1 && ly.table.ifcProbe(ifEvent).Exists("DblClick"))
		{
		if (!ly.table.clicked || !ly.table.clicked[ly.rownum])
		    {
		    if (!ly.table.clicked) ly.table.clicked = new Array();
		    if (!ly.table.tid) ly.table.tid = new Array();
		    ly.table.clicked[ly.rownum] = 1;
		    ly.table.tid[ly.rownum] = setTimeout(tbld_unsetclick, 500, ly.table, ly.rownum);
		    }
		else
		    {
		    ly.table.clicked[ly.rownum] = 0;
		    clearTimeout(ly.table.tid[ly.rownum]);
		    var event = new Object();
		    if (orig_ly.subkind == 'cell')
			{
			event.Column = ly.table.cols[orig_ly.colnum].fieldname;
			event.ColumnValue = orig_ly.data;
			}
		    event.Caller = ly.table;
		    event.recnum = ly.rownum;
		    event.selected = selected;
		    event.data = new Object();
		    var rec=ly.table.osrc.replica[ly.rownum];
		    if(rec)
			{
			for(var i in rec)
			    {
			    event.data[rec[i].oid]=rec[i].value;
			    if (rec[i].oid != 'data' && rec[i].oid != 'Caller' && rec[i].oid != 'recnum')
				event[rec[i].oid] = rec[i].value;
			    }
			}
		    ly.table.dta=event.data;
		    if (isCancel(ly.table.ifcProbe(ifEvent).Activate('DblClick', event)))
			toggle_row = false;
		    }
		}
	    if (toggle_row)
		{
		if (ly.table.initselect !== 1 || moved)
		    {
		    ly.table.initselect = 1;
		    ly.table.RedrawAll(null, true);
		    }
		else if (ly.table.initselect !== ly.table.initselect_orig)
		    {
		    if (ly.table.allowdeselect)
			ly.table.initselect = ly.table.initselect_orig;
		    ly.table.RedrawAll(null, true);
		    }
		}
	    }
        if(ly.subkind=='headercell')
            {
            var neworder=new Array();
            for(var i in ly.row.table.osrc.orderobject)
                neworder[i]=ly.row.table.osrc.orderobject[i];
            
            var colname=ly.row.table.cols[ly.colnum].sort_fieldname;
	    if (!colname)
		colname=ly.row.table.cols[ly.colnum].fieldname;
	    /** check for the this field already in the sort criteria **/
            if(':"'+colname+'" asc'==neworder[0])
                neworder[0]=':"'+colname+'" desc';
            else if (':"'+colname+'" desc'==neworder[0])
                neworder[0]=':"'+colname+'" asc';
            else
                {
                for(var i in neworder)
                    if(neworder[i]==':"'+colname+'" asc' || neworder[i]==':"'+colname+'" desc')
                        neworder.splice(i,1);
                neworder.unshift(':"'+colname+'" asc');
                }
	    ly.row.table.osrc.ifcProbe(ifAction).Invoke("OrderObject", {orderobj:neworder});
            }
        if(ly.subkind=='up' || ly.subkind=='bar' || ly.subkind=='down' || ly.subkind=='box')
            {
	    ly.Click(e);
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mousemove(e)
    {
    if (tbld_mcurrent)
	{
	var t_offs = $(tbld_mcurrent).offset();
	if (e.pageX < t_offs.left || e.pageX > t_offs.left + tbld_mcurrent.param_width || e.pageY < t_offs.top || e.pageY > t_offs.top + tbld_mcurrent.param_height)
	    {
	    var t = tbld_mcurrent;
	    tbld_mcurrent = null;
	    t.has_mouse = false;
	    if (t.demand_scrollbar)
		{
		t.UpdateScrollbar();
		}
	    }
	}
    if (tbldb_current != null)
        {
        var l=tbldb_current.cell;
        var t=l.row.table;
        var move = e.pageX - tbldb_start;
        tbldb_start += move;
        var realmove = l.ChangeWidth(move);
	tbldb_start += (realmove-move);
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (tbldx_current)
	{
	var incr = e.pageY - tbldx_start;
	var t = tbldx_current.table;
	if (tbldx_tstart + incr < 18)
	    incr = 18 - tbldx_tstart;
	if (tbldx_tstart + incr + $(t.box).height() > $(t.scrollbar).height() - 18 - 3)
	    incr = $(t.scrollbar).height() - 18 - 3 - tbldx_tstart - $(t.box).height();
	setRelativeY(t.box, tbldx_tstart + incr);
	if (t.thumb_avail > t.thumb_height)
	    {
	    t.SchedScroll((-t.scroll_minheight) - Math.floor((tbldx_tstart + incr - 18)*t.thumb_sh/(t.thumb_avail - t.thumb_height)));
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tbld_mouseup(e)
    {
    var ly = e.layer;
    if (tbldb_current != null)
        {
        tbldb_current=null;
        tbldb_start=null;
        }
    if (tbldx_current)
	{
	tbldx_current = null;
	}
    if(ly.kind && ly.kind=='tabledynamic')
        {
        if(ly.subkind=='cellborder')
            {
            if(tbldbdbl_current==ly)
                {
                clearTimeout(tbldbdbl_current.time);
                tbldbdbl_current=null;
                var l = ly.cell;
                var t = l.row.table;
                var maxw = 0;
                for(var i=0;i<=t.rows.lastvis;i++)
                    {
                    if((i==0 || i >= t.rows.firstvis) && t.rows[i])
			{
			var cell = t.rows[i].cols[l.colnum];
			if (cell.firstChild && cell.firstChild.firstChild && getdocWidth(cell.firstChild.firstChild) > maxw)
			    maxw=getdocWidth(cell.firstChild.firstChild);
			}
                    }
		maxw += t.innerpadding*2;
		if (t.colsep > 0 || t.dragcols)
		    maxw += (t.bdr_width*2 + t.colsep);
                l.ChangeWidth(maxw-t.cols[l.colnum].width);
                }
            else
                {
                if(tbldbdbl_current && tbldbdbl_current.time)
                    clearTimeout(tbldbdbl_current.time);
                tbldbdbl_current=ly;
                tbldbdbl_current.time=setTimeout('tbldbdbl_current=null;',1000);
                }
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_table.js'] = true;
