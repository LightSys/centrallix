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

window.cht_touches = [];

function cht_data_available(fromobj, why) {this.update_soon = true}

function cht_object_available(dataobj, force_datafetch, why) {
    // update chart with new data only
    // if you want to update with focus change, remove the if statement
    if(this.update_soon) {
        this.columns = this.GetColumns();
        this.chart.data = this.GetChartData();
        this.update_soon = false;
    }
    this.Highlight(this.osrc.CurrentRecord - 1, this.osrc.CurrentRecord - 1);
    this.chart.update()
    this.osrc_busy = false;
}

function cht_replica_moved(dataobj) {this.update_soon = true;}
function cht_operation_complete(stat, osrc) {}
function cht_object_deleted(recnum) {this.update_soon = true;}
function cht_object_created(recnum) {this.update_soon = true;}
function cht_object_modified(current, dataobj) {this.update_soon = true;}

function cht_linear_x_axis() {
    // fancier logic in future
    return this.params.chart_type === "line" || this.params.chart_type === "scatter";
}

function cht_is_number_type(type) {
    return ["integer", "money", "double"].includes(type);
}

function cht_get_columns(){
    let columns = [];
    let a_row = this.osrc.replica[1];

    for (let col_idx in a_row){
        if (a_row[col_idx].hasOwnProperty("system") && !a_row[col_idx].system) {
            columns.push({
                name: a_row[col_idx].oid,
                type: this.IsNumberType(a_row[col_idx].type) ? "number" : a_row[col_idx].type
            });
        }
    }
    return columns;
}

function cht_get_col_values(col_name) {
    col_values = [];

    for (let row in this.osrc.replica) {
        for (let col in this.osrc.replica[row]) {
            let datum = this.osrc.replica[row][col];
            if (datum.oid === col_name && !datum.system){
               col_values.push(datum.value);
               break;
            }
        }
    }
    return col_values;
}

function cht_find_col_with_type(type) {
    let a_col = this.columns.filter(function (column) {return column.type === type})[0];
    return a_col?(a_col.name):null;
}

function cht_get_linear_data(series_idx) {
    if (!this.LinearXAxis()){
        return this.GetColValues(this.GetYColName(series_idx));
    } else {
       let x_vals = this.GetColValues(this.GetXColName(series_idx));
       let y_vals = this.GetColValues(this.GetYColName(series_idx));
       let zipped = [];

       for (let idx = 0; idx < Math.min(x_vals.length, y_vals.length); idx++) {
           zipped.push({x: x_vals[idx], y: y_vals[idx]});
       }
       return zipped.sort(function (a, b) {return a.x - b.x});
    }
}

function cht_get_x_col_name(series_idx) {
    let type = this.LinearXAxis() ? "number" : "string";
    let name = this.params.series[series_idx].x_column;

    if (this.columns.filter(function (col) {return col.name === name}).length){
        return name;
    } else {
        if (name)
            console.warn(name + " is not a valid column");
        return this.FindColWithType(type);
    }

}

function cht_get_y_col_name(series_idx) {
    let name = this.params.series[series_idx].y_column;

    if (this.columns.filter(function (col) {return col.name === name}).length){
        return name;
    } else {
        if (name)
            console.warn(name + " is not a valid column");
        return this.FindColWithType("number");
    }
}

function cht_get_series_label(series_idx) {
    return this.params.series[series_idx].label || this.GetYColName(series_idx);
}

function cht_get_categories(series_idx) {
    return this.GetColValues(this.GetXColName(series_idx));
}

function cht_highlight(index) {
    if (this.chart.data.datasets[0]) {
	let colorArray = this.chart.data.datasets[0].backgroundColor;

	// Reset all bar colors
	for (let i = 0; i < colorArray.length; i++) {
	    colorArray[i] = Color(colorArray[i]).alpha(0.5).rgbString();
	}
	
	// Highlight selected bar
	colorArray[index] = Color(colorArray[index]).alpha(0.95).rgbString();
    }
}

function cht_generate_rgba(id, alpha = 1) {
    if (id.toString().charAt(0) == '#')
	return Color(id).alpha(alpha).rgbString();
    let colors = {
        blue: 'rgb(54, 162, 235)',
        purple: 'rgb(153, 102, 255)',
        red: 'rgb(255, 99, 132)',
        orange: 'rgb(255, 159, 64)',
        yellow: 'rgb(255, 205, 86)',
        green: 'rgb(75, 192, 192)',
        grey: 'rgb(201, 203, 207)'
    };
    let color_names = Object.keys(colors);
    if (typeof id === 'string' && colors[id]) return Color(colors[id]).alpha(alpha).rgbString();
    else if (typeof id === 'number') return Color(colors[color_names[id % color_names.length]]).alpha(alpha).rgbString();
    else return Color('rgb(54, 162, 235)').alpha(alpha).rgbString();
}

function cht_choose_rgba(series_idx, alpha) {
    if (this.params.series[series_idx] && this.params.series[series_idx].color) return this.GenerateRgba(this.params.series[series_idx].color, alpha);
    else return this.GenerateRgba(Number.parseInt(series_idx), alpha);
}

function cht_get_datasets(){
    let datasets = [];
    for (let series_idx in this.params.series){
	let background_color = [];
	let border_color = [];
        let data = this.GetLinearData(series_idx);
	if (this.params.chart_type === "pie" || this.params.chart_type === "doughnut") {
	    for(let i=0; i<data.length; i++)
		background_color[i] = this.ChooseRgba(i, 0.5);
	    for(let i=0; i<data.length; i++)
		border_color[i] = this.ChooseRgba(i);
	} else {
	    background_color = this.ChooseRgba(series_idx, (this.params.chart_type === "line") ? 1 : 0.5);
	    border_color = this.ChooseRgba(series_idx);
	    background_color = new Array(data.length).fill(background_color);
	    border_color = new Array(data.length).fill(border_color);
	}
        datasets.push({
            data: data,
            label: this.GetSeriesLabel(series_idx),
            type: this.params.series[series_idx].chart_type,
            borderColor: border_color,
            backgroundColor: background_color,
            borderWidth: 2,
            fill: this.params.series[series_idx].fill
        });
    }
    return datasets;
}

function cht_get_chart_data(){
    return {
        labels: this.LinearXAxis() ? [] : this.GetCategories(0),
        datasets: this.GetDatasets()
    };
}

function cht_get_scale_label(x_y) {
    let axes = this.params.axes.filter(function (elem) {return elem.axis === x_y});
    if (axes.length === 0) return {};
    return {
        //display: (axes[0].label !== ""),
        display: axes[0].label?true:false,
        labelString: axes[0].label,
    }
}

function cht_get_scales() {
    if (this.params.chart_type === "pie" || this.params.chart_type === "doughnut") return {};
    return {
        xAxes: [{
            type: this.LinearXAxis() ? "linear" : "category",
            scaleLabel: this.GetScaleLabel('x'),
	    stacked: this.params.stacked?true:false,
        }],
        yAxes: [{
            ticks: {
                beginAtZero: this.params.start_at_zero,
            },
            scaleLabel: this.GetScaleLabel('y'),
	    stacked: this.params.stacked?true:false,
        }]
    };
}

function cht_chartjs_init() {
    let chart_wgt = this;
    let canvas = document.getElementById(this.params.canvas_id).getContext("2d");
    this.chart = new Chart(canvas, {
        type: this.params.chart_type,
        data: {},
        options: {
            scales: this.GetScales(),
            title: {
                display: this.params.title?true:false,
                text: this.params.title,
                fontColor: this.params.title_color,
                fontSize: this.params.title_size
            },
            legend: {
                position: this.params.legend_position
            },
            onClick: function(event, activeElements) {
                if (activeElements.length == 1) {
                    chart_wgt.OsrcRequest('MoveToRecord', {
                        rownum: activeElements[0]._index + 1
                    })
                }
            }
        },
    });
}

function cht_get_osrc(){
    if (this.params.osrc)
        this.osrc = wgtrGetNode(this, this.params.osrc, "widget/osrc");
    else
        this.osrc = wgtrFindContainer(this, "widget/osrc");

    if(!this.osrc) {
        throw 'The chart widget requires an objectsource and at least one column';
    }
}

function cht_osrc_request(request, param)
    {
    var item = {type: request};
    for (var p in param)
        item[p] = param[p];
    this.osrc_request_queue.push(item);
    this.OsrcDispatch();
    }


function cht_osrc_dispatch()
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

// Called when the chart's layer is revealed/shown to the user
function cht_cb_reveal(event)
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

function cht_register_osrc_functions(chart_wgt){
    chart_wgt.IsDiscardReady = new Function('return true;');
    chart_wgt.DataAvailable = cht_data_available; //Called when there is new data on the way
    chart_wgt.ObjectAvailable = cht_object_available; //Called when the object focus changes. This is kind of a catch-all
    chart_wgt.ReplicaMoved = cht_replica_moved; //Called when the set of available records changes
    chart_wgt.OperationComplete = cht_operation_complete; //Unknown call semantics
    chart_wgt.ObjectDeleted = cht_object_deleted;
    chart_wgt.ObjectCreated = cht_object_created;
    chart_wgt.ObjectModified = cht_object_modified;
    chart_wgt.OsrcRequest = cht_osrc_request;
    chart_wgt.OsrcDispatch = cht_osrc_dispatch;
    chart_wgt.osrc.Register(chart_wgt);
}

function cht_register_helper_functions(chart_wgt){
    chart_wgt.GetOsrc = cht_get_osrc;
    chart_wgt.ChartJsInit = cht_chartjs_init;
    chart_wgt.GetChartData = cht_get_chart_data;
    chart_wgt.LinearXAxis = cht_linear_x_axis;
    chart_wgt.GetLinearData = cht_get_linear_data;
    chart_wgt.GetCategories = cht_get_categories;
    chart_wgt.GetSeriesLabel = cht_get_series_label;
    chart_wgt.GetColumns = cht_get_columns;
    chart_wgt.GetColValues = cht_get_col_values;
    chart_wgt.FindColWithType = cht_find_col_with_type;
    chart_wgt.GetXColName = cht_get_x_col_name;
    chart_wgt.GetYColName = cht_get_y_col_name;
    chart_wgt.IsNumberType = cht_is_number_type;
    chart_wgt.GetDatasets = cht_get_datasets;
    chart_wgt.ChooseRgba = cht_choose_rgba;
    chart_wgt.GenerateRgba = cht_generate_rgba;
    chart_wgt.GetScales = cht_get_scales;
    chart_wgt.GetScaleLabel = cht_get_scale_label;
    chart_wgt.Highlight = cht_highlight;
}

function cht_init(params) {
    let chart_wgt = params.chart;
    delete params.chart;
    chart_wgt.params = params;

    cht_register_helper_functions(chart_wgt);
    chart_wgt.GetOsrc();
    cht_register_osrc_functions(chart_wgt);
    chart_wgt.osrc_request_queue = [];
    chart_wgt.osrc_busy = false;
    chart_wgt.osrc_last_op = null;

    // Request reveal/obscure notifications
    chart_wgt.Reveal = cht_cb_reveal;
    pg_reveal_register_listener(chart_wgt);

    this.update_soon = false; //see cht_object_available

    chart_wgt.ChartJsInit();
}

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_chart.js'] = true;
