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

function cht_data_available(fromobj, why) {}

function cht_object_available(dataobj, force_datafetch, why) {
    //update chart with new data
    this.columns = this.GetColumns();
    this.chart.data = this.GetChartData();
    this.chart.update();
}

function cht_replica_moved(dataobj, force_datafetch) {}
function cht_operation_complete(stat, osrc) {}
function cht_object_deleted(recnum) {}
function cht_object_created(recnum) {}
function cht_object_modified(current, dataobj) {}

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
            if (datum.oid === col_name){
               col_values.push(datum.value);
               break;
            }
        }
    }
    return col_values;
}

function cht_find_col_with_type(type) {
    let a_col = this.columns.filter(function (column) {return column.type === type})[0];
    return a_col.name;
}

function cht_get_linear_data(series_idx) {
    if (!this.LinearXAxis()){
        return this.GetColValues(this.GetYColName(series_idx));
    } else {
       let x_vals = this.GetColValues(this.GetXColName(series_idx));
       let y_vals = this.GetColValues(this.GetYColName(series_idx));
       console.log(x_vals, y_vals);
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

function cht_get_chart_data(){
    return {
        labels: this.LinearXAxis() ? [] : this.GetCategories(0),
        datasets: [{
            data: this.GetLinearData(0),
            label: this.GetSeriesLabel(0)
        }]
    };
}

function cht_chartjs_init() {
    let canvas = document.getElementById(this.params.canvas_id).getContext("2d");
    this.chart = new Chart(canvas, {
        type: this.params.chart_type,
        data: {},
        options: {},
    });
    if (this.params.chart_type != "pie")
        this.chart.options.scales.xAxes[0].type = this.LinearXAxis() ? "linear" : "";
}

function cht_get_osrc(){
    if (this.params.osrc)
        this.osrc = wgtrGetNode(this, params.osrc, "widget/osrc");
    else
        this.osrc = wgtrFindContainer(this, "widget/osrc");

    if(!this.osrc) {
        throw 'The chart widget requires an objectsource and at least one column';
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
}

function cht_init(params) {
    console.log("params: ", params);
    var chart_wgt = params.chart;
    delete params.chart;
    chart_wgt.params = params;

    cht_register_helper_functions(chart_wgt);
    chart_wgt.GetOsrc();
    cht_register_osrc_functions(chart_wgt);

    chart_wgt.ChartJsInit();
}

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_chart.js'] = true;
