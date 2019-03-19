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
    }
function cht_object_available(dataobj, force_datafetch, why)
    {
        //update chart with new datums
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
        console.log(params);
        canvas = document.getElementById(params.canvas_id).getContext("2d");
        return new Chart(canvas, {
            type: "bar",
            data: {
                datasets: [{
                  data: [1, 2, 3],
                  label: "Horse",
                  background_color: 'blue',
                  border_color: 'black'
                }],
                labels:["small", "big", "huge"]
            },
        })
    }

function cht_init(params)
    {
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

        chart.IsDiscardReady = new Function('return true;');
        chart.DataAvailable = cht_data_available;
        chart.ObjectAvailable = cht_object_available;
        chart.ReplicaMoved = cht_replica_moved;
        chart.OperationComplete = cht_operation_complete;
        chart.ObjectDeleted = cht_object_deleted;
        chart.ObjectCreated = cht_object_created;
        chart.ObjectModified = cht_object_modified;
        chart.osrc.Register(chart);

        cht_chartjs_init(params);
    	return chart
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_chart.js'] = true;
