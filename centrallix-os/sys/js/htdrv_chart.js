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

function cht_data_available(fromobj, why)
    {
        //Called when there is new data on the way
        //Since this is a chart, do nothing
    }
function cht_object_available(dataobj, force_datafetch, why)
    {
        //update chart with new datums
        this.ParseOsrcData();
        if (this.chart) this.chart.update();
        else console.log("no chart")
    }
function cht_replica_moved(dataobj, force_datafetch)
    {
        //Called when the set of available records changes
    }
function cht_operation_complete(stat, osrc)
    {
        //Unknown
    }
function cht_object_deleted(recnum)
    {
        //Called when an object is deleted
    }

function cht_object_created(recnum)
    {
        //Called when an object is created
    }

function cht_object_modified(current, dataobj)
    {
        //Called when an object is modified
    }
function cht_chartjs_init(params)
    {
        //console.log(params);
        canvas = document.getElementById(params.canvas_id).getContext("2d");
        this.chart = new Chart(canvas, {
            type: "bar",
            data: {},
            options: {
                maintainAspectRatio: true
            },
        });
    }

function cht_parse_osrc_data() {
    let category = "";
    let cols = {};
    // assert that there's only one string column later...
    for (let r in this.osrc.replica) {
        for (let c in this.osrc.replica[r]) {
            if (this.osrc.replica[r][c].hasOwnProperty("system") && !this.osrc.replica[r][c].system) {
                if (!cols.hasOwnProperty(this.osrc.replica[r][c].oid)) {
                    cols[this.osrc.replica[r][c].oid] = [];
                    if (this.osrc.replica[r][c].type === "string") category = this.osrc.replica[r][c].oid;
                }
                cols[this.osrc.replica[r][c].oid].push(this.osrc.replica[r][c].value);
            }
        }
    }
    // now re-sort into datasets
    let datas = [];
    if (category) this.chart.data.labels = cols[category];
    for (let i in cols) {
        if (i !== category) {
            datas.push({
                data: cols[i],
                label: i
            });
/*            datas.push({
                data: cols[i].slice(2,7),
                label: "double"
            });*/
        }
    }
    this.chart.data.datasets = datas;
}

function cht_init(params)
    {
        console.log("params: ", params);
        var chart = params.chart;

        if (params.osrc)
            chart.osrc = wgtrGetNode(chart, params.osrc, "widget/osrc");
        else
	        chart.osrc = wgtrFindContainer(chart, "widget/osrc");

        if(!chart.osrc)
	        {
	        alert('The chart widget requires an objectsource and at least one column');
	        return chart;
	        }

        // osrc callback stuff
        chart.IsDiscardReady = new Function('return true;');
        chart.DataAvailable = cht_data_available;
        chart.ObjectAvailable = cht_object_available;
        chart.ReplicaMoved = cht_replica_moved;
        chart.OperationComplete = cht_operation_complete;
        chart.ObjectDeleted = cht_object_deleted;
        chart.ObjectCreated = cht_object_created;
        chart.ObjectModified = cht_object_modified;
        chart.osrc.Register(chart);

        // private functions and attributes
        chart.ChartJsInit = cht_chartjs_init;
        chart.ParseOsrcData = cht_parse_osrc_data; // pls rename later
        chart.data = {};

        chart.ChartJsInit(params);
    	return chart
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_chart.js'] = true;
