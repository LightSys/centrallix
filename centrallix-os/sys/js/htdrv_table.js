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

function tbld_format_cell(cell, color)
    {
    var txt,captxt;
    var style = 'margin:0px; padding:0px; ';
    var capstyle = 'font-size:80%; margin:2px 0px 0px 0px; padding:0px; ';
    var colinfo = this.cols[cell.colnum];
    if (cell.subkind != 'headercell' && colinfo.type != 'check' && colinfo.type != 'image')
	var str = htutil_encode(String(htutil_obscure(cell.data)), colinfo.wrap != 'no');
    else
	var str = htutil_encode(String(cell.data));
    if (colinfo.wrap != 'no')
	str = htutil_nlbr(str);
    if (cell.subkind != 'headercell' && colinfo.type == 'check')
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
	if (str.indexOf(':') >= 0 || str.indexOf('//') >= 0 || str.charAt(0) != '/')
	    txt = '';
	else
	    {
	    txt = '<img src="' + htutil_encode(String(cell.data),true) + '"';
	    if (colinfo.image_maxwidth || colinfo.image_maxheight)
		{
		txt += ' style="';
		if (colinfo.image_maxwidth) txt += 'max-width:' + colinfo.image_maxwidth + 'px; ';
		if (colinfo.image_maxheight) txt += 'max-height:' + colinfo.image_maxheight + 'px; ';
		txt += '"';
		}
	    txt += ">";
	    }
	}
    else
	{
	// Text
	txt = '<span>' + str + '</span>';
	}
    style += htutil_getstyle(wgtrFindDescendent(this,colinfo.name,colinfo.ns), null, {textcolor: color});
    if (cell.capdata)
	{
	// Caption (added to any of the above types)
	captxt = '<span>' + htutil_encode(htutil_obscure(cell.capdata), colinfo.wrap != 'no') + '</span>';
	if (colinfo.wrap != 'no')
	    captxt = htutil_nlbr(captxt);
	capstyle += htutil_getstyle(wgtrFindDescendent(this,colinfo.name,colinfo.ns), "caption", {textcolor: color});
	}

    // If style or content has changed, then update it.
    if (txt != cell.content || captxt != cell.capcontent || style != cell.cxstyle || capstyle != cell.cxcapstyle)
	{
	var p = document.createElement('p');
	p.style = style;
	$(p).append(txt);
	if (captxt)
	    {
	    var c_p = document.createElement('p');
	    c_p.style = capstyle;
	    $(c_p).append(captxt);
	    }
	$(cell).empty();
	$(cell).append(p);
	if (captxt)
	    $(cell).append(c_p);
	cell.content = txt;
	cell.capcontent = captxt;
	cell.cxstyle = style;
	cell.cxcapstyle = capstyle;

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
		t.UpdateHeight(row);
		if (pre_h != $(row).height() && row.positioned)
		    {
		    for(var i=row.rownum+1; i<=t.rows.lastvis+1; i++)
			{
			if (t.rows[i])
			    {
			    t.rows[i].positioned = false;
			    upd_rows.push(t.rows[i]);
			    }
			}
		    t.PositionRows(upd_rows);
		    t.DisplayRows(upd_rows);
		    }
		});
	    }
	}
    }

function tbld_attr_cmp(a, b)
    {
    if (a.oid > b.oid)
	return 1;
    else if (a.oid < b.oid)
	return -1;
    else
	return 0;
    }


function tbld_redraw_all(dataobj, force_datafetch)
    {
    var new_rows = [];

    // Creating a new record?  Give indication if so.
    this.was_new = this.is_new;
    this.is_new = (dataobj && dataobj.length == 0 && this.row_bgndnew)?1:0;
    if (this.was_new && !this.is_new && this.rows.last)
	{
	var recnum = this.rows.last;
	this.scroll_maxheight -= ($(this.rows[recnum]).height() + this.cellvspacing*2);
	this.scroll_maxrec--;
	}

    // Note the current record.
    this.cr = this.osrc.CurrentRecord;
    if (this.is_new)
	{
	this.cr = this.rows.last + 1;
	this.target_range = {start:this.rows.first, end:this.rows.last+1};
	}

    // Presentation mode -- rows or propsheet?
    if (this.datamode == 1)
	{
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
	    this.SetupRowData(i);
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
	this.Scroll(this.target_y);
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
    if (this.cr < this.rows.firstvis || this.cr > this.rows.lastvis)
	{
	this.BringIntoView(this.cr);
	return;
	}

    // space at bottom of table?
    if (this.CheckBottom())
	return;
    }

// Handle deletion or new row cancel cases where blank space is now visible
// even though there are enough rows to fill the visible area.
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
	    this.Scroll(sy + y_space);
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


function tbld_setup_row_data(rowslot)
    {
    var row = this.rows[rowslot];
    var changed = false;

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
	    row.cols[j].data=txt;
	    }
	}
    else
	{
	// Normal
	// main value
	for(var j in row.cols)
	    {
	    if (this.cols[j].fieldname)
		var txt = this.FindOsrcValue(rowslot, this.cols[j].fieldname);
	    else
		var txt = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "value");
	    if (typeof row.cols[j].data == 'undefined' || (row.cols[j].data == null && txt) || txt != row.cols[j].data)
		changed = true;
	    row.cols[j].data = txt;
	    }
	row.name = this.FindOsrcValue(rowslot, 'name');

	// caption value
	for(var j in row.cols)
	    {
	    if (this.cols[j].caption_fieldname)
		var txt = this.FindOsrcValue(rowslot, this.cols[j].caption_fieldname);
	    else
		var txt = wgtrGetServerProperty(wgtrFindDescendent(this, this.cols[j].name, this.cols[j].ns), "caption_value");
	    if (typeof row.cols[j].capdata == 'undefined' || (row.cols[j].capdata == null && txt) || txt != row.cols[j].capdata)
		changed = true;
	    row.cols[j].capdata = txt;
	    }
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
	    this.rows[id].newselect();
	    break;
	case 'select':
	    this.rows[id].select();
	    break;
	case 'deselect':
	    this.rows[id].deselect();
	    break;
	}
    if (this.UpdateHeight(this.rows[id]))
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
    this.bring_into_view = null;

    // Clamp the requested row to the available range
    if (rownum < 1)
	rownum = 1;
    if (this.rows.lastosrc && rownum > this.rows.lastosrc)
	rownum = this.rows.lastosrc;

    // Already visible?
    if (rownum >= this.rows.firstvis && rownum <= this.rows.lastvis)
	return;

    // If row is in current set, just scroll backward.
    if (rownum >= this.rows.first && rownum < this.rows.firstvis)
	{
	this.Scroll(0 - getRelativeY(this.rows[rownum]));
	return;
	}

    // Likewise, if row is in current set, scroll forward.
    if (rownum <= this.rows.last && rownum > this.rows.lastvis)
	{
	this.Scroll(this.vis_height - (getRelativeY(this.rows[rownum]) + $(this.rows[rownum]).height() + this.cellvspacing*2));
	return;
	}

    // Out of range
    if (rownum < this.rows.first && this.rows.first != null)
	{
	this.bring_into_view = rownum;
	//this.target_range = {start:rownum, end:this.rows.lastvis+1};
	this.target_range = {start:rownum, end:rownum + this.max_display*2};
	if (this.rows.lastosrc && this.target_range.end > this.rows.lastosrc)
	    this.target_range.end = this.rows.lastosrc;
	if (this.target_range.start > 1 && this.target_range.end - this.target_range.start < this.max_display*2)
	    this.target_range.start = this.target_range.end - this.max_display*2;
	if (this.target_range.start < 1)
	    this.target_range.start = 1;
	this.osrc.ScrollTo(this.target_range.start, this.target_range.end);
	}
    else if (rownum > this.rows.last && this.rows.last != null)
	{
	this.bring_into_view = rownum;
	//this.target_range = {start:this.rows.firstvis-1, end:rownum};
	this.target_range = {start:rownum - this.max_display*2, end:rownum};
	if (this.target_range.start < 1)
	    this.target_range.start = 1;
	this.osrc.ScrollTo(this.target_range.start, this.target_range.end);
	}
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
	$(this.scrollbar.b).animate({"top": (18+Math.round(this.thumb_pos))+"px", "height": (Math.round(this.thumb_height)-2)+"px"}, 250, "swing", null);
	}
    else
	{
	setRelativeY(this.scrollbar.b, 18 + Math.round(this.thumb_pos));
	$(this.scrollbar.b).height(Math.round(this.thumb_height) - 2);
	}

    // Set scrollbar visibility
    if (this.demand_scrollbar)
	{
	$(this.scrollbar).stop(false, true);
	$(this.scrollbar).animate({"opacity": (this.thumb_height == this.thumb_avail)?0.0:(this.has_mouse?1.0:0.33)}, 150, "linear", null);
	}
	//$(this.scrollbar).css({"opacity": (this.thumb_height == this.thumb_avail)?0.0:1.0});
	//htr_setvisibility(this.scrollbar, (this.thumb_height == this.thumb_avail)?"hidden":"inherit");
    }


function tbld_object_created(recnum)
    {
    if (this.rows.lastosrc && recnum > this.rows.lastosrc)
	this.rows.lastosrc = recnum;
    if (recnum < this.rows.first)
	this.target_range = {start:recnum, end:this.rows.last};
    else if (recnum > this.rows.last)
	this.target_range = {start:this.rows.first, end:recnum};
    this.RedrawAll(null, true);
    }

function tbld_object_deleted(recnum)
    {
    if (this.rows.lastosrc && this.rows.lastosrc == recnum)
	this.rows.lastosrc--;
    if (this.rows[recnum] && this.scroll_maxheight)
	{
	this.scroll_maxheight -= ($(this.rows[recnum]).height() + this.cellvspacing*2);
	this.scroll_maxrec--;
	}
    this.RedrawAll(null, true);
    }

function tbld_object_modified(current, dataobj)
    {
    this.RedrawAll(null, true);
    }

function tbld_clear_rows(fromobj, why)
    {
    if (why == 'refresh')
	this.crname = (this.cr && this.rows[this.cr])?this.rows[this.cr].name:null;
    else
	this.crname = null;
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
    this.target_range = {start:1, end:this.max_display*2};
    this.UpdateThumb(false);
    if (why != 'refresh')
	this.initselect = this.initselect_orig;
    }

function tbld_select()
    {
    var txt;
    tbld_setbackground(this, this.table, 'rowhighlight');
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
    for(var i=0; i<this.table.detail_widgets.length; i++)
	{
	var dw = this.table.detail_widgets[i];
	if (wgtrGetServerProperty(dw, 'display_for', 1))
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
		if ($(dw).css("visibility") == 'inherit')
		    pg_reveal_event(dw, dw, 'Obscure');

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
		    break;
		    }
		}
	    }
	}
    }

function tbld_deselect()
    {
    var txt;
    tbld_setbackground(this, this.table, this.rownum%2?'row1':'row2');
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
	    }
	}
    this.detail = [];
    }

function tbld_newselect()
    {
    var txt;
    tbld_setbackground(this, this.table, 'newrow');
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
    if(this.rownum!=null && this.subkind!='headerrow')
	$(this).css({"border": "1px solid black"});
    }

function tbld_domouseout()
    {
    if(this.subkind!='headerrow')
	{
	var rbc = wgtrGetServerProperty(this.table,"row_border_color");
	$(this).css({"border": "1px solid " + (rbc?rbc:"transparent") });
	}
    }


function tbld_sched_scroll(y)
    {
    if (this.scroll_timeout)
	pg_delsched(this.scroll_timeout);
    $(this.scrolldiv).stop(false, true);
    $(this.box).stop(false, true);
    pg_addsched_fn(this, "Scroll", [y], 0);
    }


// Scroll the table to the given y offset
function tbld_scroll(y)
    {
    this.target_y = null;

    // Not enough data to scroll?
    if (this.thumb_height == this.thumb_avail)
	return;

    // Clamp the scroll range
    if (this.rows.first == 1 && (0-y) < getRelativeY(this.rows[this.rows.first]))
	y = 0 - getRelativeY(this.rows[this.rows.first]);
    if (this.rows.lastosrc == this.rows.last && (0-y) > (getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing*2 - this.vis_height))
	y = 0 - (getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing*2 - this.vis_height);

    // No new data needed?
    if (getRelativeY(this.rows[this.rows.first]) <= (0-y) && getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing*2 - this.vis_height >= (0-y))
	{
	this.scroll_y = y;
	$(this.scrolldiv).stop(false, false);
	$(this.scrolldiv).animate({"top": y+"px"}, 250, "swing", null);
	this.target_range = {start:this.rows.first, end:this.rows.last};
	this.RescanRowVisibility();
	this.UpdateThumb(true);
	}
    else
	{
	// Need new data.  Set our target Y position and request more data.
	this.target_y = y;
	if (y == 0)
	    {
	    // Reset to the beginning
	    this.target_range = {start:1, end:this.max_display*2};
	    }
	else if ((0-y) < getRelativeY(this.rows[this.rows.first]))
	    {
	    // Previous data
	    this.target_range.start = this.rows.first - this.max_display*2;
	    if (this.target_range.start < 1)
		this.target_range.start = 1;
	    this.target_range.end = this.rows.lastvis+1;
	    }
	else if (getRelativeY(this.rows[this.rows.last]) + $(this.rows[this.rows.last]).height() + this.cellvspacing - this.vis_height < (0-y))
	    {
	    // Next data
	    this.target_range.start = this.rows.firstvis-1;
	    this.target_range.end = this.rows.firstvis + this.max_display*3;
	    }
	else
	    {
	    // ??
	    return;
	    }
	this.osrc.ScrollTo(this.target_range.start, this.target_range.end);
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
	t.Scroll(target_y);
	}
    else if (e.pageY < $(sb.b).offset().top)
	{
	// Up a page
	var target_row = t.rows[t.rows.firstvis];
	var target_y = t.vis_height - getRelativeY(target_row);
	t.Scroll(target_y);
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
	var new_w = this.cols[j].width - this.innerpadding*2;
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
    // we don't do obscure/reveal checks yet, so we just return true here.
    return true;
    }


// Called when the table's layer is revealed/shown to the user
function tbld_cb_reveal(event)
    {
    switch(event.eventName)
	{
	case 'Reveal':
	    if (this.osrc) this.osrc.Reveal(this);
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


function tbld_rescan_row_visibility()
    {
    this.rows.firstvis = null;
    this.rows.lastvis = this.rows.last;
    for(var i=this.rows.first; i<=this.rows.last; i++)
	{
	var rowobj = this.rows[i];
	rowobj.vis = this.IsRowVisible(i);
	if (rowobj.vis == 'full' && this.rows.firstvis == null)
	    this.rows.firstvis = i;
	if (rowobj.vis == 'full')
	    this.rows.lastvis = i;
	}
    }


function tbld_remove_row(rowobj)
    {
    if (!rowobj)
	return;
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


function tbld_instantiate_row(parentDiv, x, y)
    {
    // Check the cache
    if (this.rowdivcache.length)
	{
	var row = this.rowdivcache.pop();
	moveTo(row, x, y);
	this.ApplyRowGeom(row, 0);
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
	col.initwidth=this.cols[j].width-this.innerpadding*2;
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


function tbld_init(param)
    {
    var t = param.table;
    var scroll = param.scroll;
    ifc_init_widget(t);
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
    t.datamode = param.dm;
    t.has_header = param.hdr;
    t.demand_scrollbar = param.demand_sb;
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
    t.scrollbar.table = t.up.table = t.down.table = t.box.table = t;
    t.up.subkind='up';
    t.down.subkind='down';
    t.box.subkind='box';
    t.scrollbar.subkind='bar';
    
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
    t.UpdateThumb = tbld_update_thumb;
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

    // ObjectSource integration
    t.IsDiscardReady = new Function('return true;');
    t.DataAvailable = tbld_clear_rows;
    t.ObjectAvailable = tbld_redraw_all;
    t.ReplicaMoved = tbld_redraw_all;
    t.OperationComplete = new Function();
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
    var adj = param.width / total_w;
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
    t.target_range = {start:1, end:t.max_display*2};

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
	$(t.scrollbar).css({"opacity": 0.0, "visibility": "inherit"});
    if (window.tbld_mcurrent == undefined)
	window.tbld_mcurrent = null;

    // Events
    var ie = t.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("DblClick");
    ie.Add("RightClick");

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
	}

    return t;
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
		$(tbld_mcurrent.scrollbar).stop(false, true);
		$(tbld_mcurrent.scrollbar).animate({"opacity": (tbld_mcurrent.thumb_height == tbld_mcurrent.thumb_avail)?0.0:(tbld_mcurrent.has_mouse?1.0:0.33)}, 150, "linear", null);
		}
	    tbld_mcurrent = t;
	    if (t.demand_scrollbar)
		{
		$(t.scrollbar).stop(false, true);
		$(t.scrollbar).animate({"opacity": (t.thumb_height == t.thumb_avail)?0.0:(t.has_mouse?1.0:0.33)}, 150, "linear", null);
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
		var cell_width = getdocWidth(ly.firstChild.firstChild);
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
	    ly.Scroll(ly.scroll_y - amt_to_move);
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
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
		    ly.table.initselect = true;
		    if(ly.rownum)
			{
			ly.crname = null;
			ly.table.osrc.MoveToRecord(ly.rownum);
			}
		    }
		else if (!ly.table.initselect)
		    {
		    ly.table.initselect = true;
		    ly.table.RedrawAll(null, true);
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
		cn_activate(ly.table,'Click', event);
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
		    cn_activate(ly.table,'DblClick', event);
		    }
		}
	    }
        if(ly.subkind=='headercell')
            {
            var neworder=new Array();
            for(var i in ly.row.table.osrc.orderobject)
                neworder[i]=ly.row.table.osrc.orderobject[i];
            
            var colname=ly.row.table.cols[ly.colnum].fieldname;
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
		$(t.scrollbar).stop(false, true);
		$(t.scrollbar).animate({"opacity": (t.thumb_height == t.thumb_avail)?0.0:(t.has_mouse?1.0:0.33)}, 150, "linear", null);
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
