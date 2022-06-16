#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Core                                                      */
/*                                                                      */
/* Copyright (C) 1999-2007 LightSys Technology Services, Inc.           */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             */
/* 02111-1307  USA                                                      */
/*                                                                      */
/* A copy of the GNU General Public License has been included in this   */
/* distribution in the file "COPYING".                                  */
/*                                                                      */
/* Module:      htdrv_chart.c                                           */
/* Author:      Andrew Blomenberg                                       */
/* Creation:    March 19, 2019                                          */
/* Description:                                                         */
/************************************************************************/


#define HTTBL_MAX_COLS (32)

/** globals **/
static struct
{
    int idcnt;
} HTCHT;


int htchtSetup(pHtSession s)
	{
	htrAddStylesheetItem(s, "	div.wcht { POSITION:absolute; VISIBILITY:inherit; }\n");
	}


int
htchtCheckBrowserSupport(pHtSession session)
    {
        if (!session->Capabilities.Dom0NS && !session->Capabilities.Dom1HTML) {
            mssError(1, "HTTBL", "Netscape 4 DOM or W3C DOM support required");
            return -1;
        }
        return 0;
    }


int
htchtGetIntValue(pWgtrNode tree, char* prop_name, int default_val)
    {
    int out_val;
        if (wgtrGetPropertyValue(tree,prop_name,DATA_T_INTEGER,POD(&out_val)) != 0)
            return default_val;
        else
            return out_val;
    }


int
htchtGetStrValue(pWgtrNode tree, char* prop_name, char* default_val, char* buf, size_t buf_len)
    {
    int rval;
    char* ptr;

        /** We're not passing the buf into this function call because of convoluted c reasons **/
        rval = wgtrGetPropertyValue(tree,prop_name,DATA_T_STRING,POD(&ptr));

        if(rval == 0)
            {
            strtcpy(buf, ptr, buf_len);
            return 0;
            }
        else
            {
            strtcpy(buf, default_val, buf_len);
            return -1;
            }
    }


int
htchtGetWidth(pWgtrNode tree){return htchtGetIntValue(tree, "width", -1);}

int
htchtGetX(pWgtrNode tree){return htchtGetIntValue(tree, "x", -1);}

int
htchtGetY(pWgtrNode tree){return htchtGetIntValue(tree, "y", -1);}

int
htchtGetHeight(pWgtrNode tree)
    {
    int height;
        height = htchtGetIntValue(tree, "height", -1);
        if (height == -1)
            {
            mssError(1, "HTCHT", "'height' property is required");
            return -1;
            }

        return height;
    }

int
htchtGetName(pWgtrNode tree, char* buf, size_t buflen) {return htchtGetStrValue(tree, "name", "", buf, buflen);}

int
htchtGetTitle(pWgtrNode tree, char* buf, size_t buflen) {return htchtGetStrValue(tree, "title", "", buf, buflen);}

int
htchtGetType(pWgtrNode tree, char* buf, size_t buflen)
    {
    char* chart_types[16] = {"line", "bar", "scatter", "pie", "doughnut", NULL};
    char err_msg[256];
    int rval;
    int found = 0;
    int type_idx;

        rval = htchtGetStrValue(tree, "chart_type", "bar", buf, buflen);

	/** Verify we have a valid chart type **/
        for (type_idx = 0; chart_types[type_idx]; type_idx++)
            {
            if (strcmp(buf, chart_types[type_idx]) == 0)
		{
                found = 1;
		break;
		}
	    }

	if (found)
	    return 0;

	err_msg[0] = '\0';
        for (type_idx = 0; chart_types[type_idx]; type_idx++)
            {
            strcat(err_msg, chart_types[type_idx]);
            strcat(err_msg, " ");
            }

	mssError(1, "HTCHT", "%s is not a valid chart type. Supported types are: %s", buf, err_msg);

    return -1;
    }

int
htchtGetTitleColor(pWgtrNode tree, char* buf, size_t buflen) {return htchtGetStrValue(tree, "titlecolor", "", buf, buflen);}

int
htchtGetLegendPosition(pWgtrNode tree, char* buf, size_t buflen) {return htchtGetStrValue(tree, "legend_position", "", buf, buflen);}

int
htchtGetObjectSource(pWgtrNode tree, char* buf, size_t buflen)
    {
        /**  The ObjectSource can be provided in the configuration for this widget,
         **  but it is usually determined by the javascript driver.
         **  Setting ObjectSource to an empty string indicates that the javascript should determine the ObjectSource.
         **/
        return htchtGetStrValue(tree, "objectsource", "", buf, buflen);
    }

void
htchtGetCanvasId(pWgtrNode tree, char* buf, size_t buflen) { snprintf(buf, buflen, "cht%dcanvas", HTCHT.idcnt); }

void
htchtAddSeriesProperties(pHtSession session, pWgtrNode tree)
    {
    int num_series;
    int series_idx;
    pWgtrNode series[32];
    pWgtrNode sub_tree;
    char label[32];
    char color[32];
    char x_column[32];
    char y_column[32];
    char chart_type[32];

        num_series = wgtrGetMatchingChildList(tree, "widget/chart-series", series, sizeof(series)/sizeof(pWgtrNode));
        for (series_idx = 0; series_idx < num_series; series_idx++)
            {
	    sub_tree = series[series_idx];

            /** It is actually possible for the subwidgets to be in another namespace, so we check that **/
            htrCheckNSTransition(session, tree, sub_tree);

            htchtGetStrValue(sub_tree, "label", "", label, sizeof(label));
            htchtGetStrValue(sub_tree, "color", "", color, sizeof(color));
            htchtGetStrValue(sub_tree, "x_column", "", x_column, sizeof(x_column));
            htchtGetStrValue(sub_tree, "y_column", "", y_column, sizeof(y_column));
            htchtGetStrValue(sub_tree, "chart_type", "", chart_type, sizeof(chart_type));

            htrAddScriptInit_va(session, "    chartobj.series.push({ label: \"%STR&JSSTR\", color: \"%STR&JSSTR\", fill: %INT, x_column: \"%STR&JSSTR\", y_column: \"%STR&JSSTR\", chart_type: \"%STR&JSSTR\" });\n",
                                   label,
                                   color,
                                   htrGetBoolean(tree, "fill", 1),
                                   x_column,
                                   y_column,
                                   chart_type);

            htrCheckNSTransitionReturn(session, tree, sub_tree);
            }

    return;
    }


void
htchtAddAxesProperties(pHtSession session, pWgtrNode tree)
    {
    int num_axes;
    int axes_idx;
    pWgtrNode axes[32];
    pWgtrNode sub_tree;
    char label[32];
    char axis[8];

        num_axes = wgtrGetMatchingChildList(tree, "widget/chart-axis", axes, sizeof(axes)/sizeof(pWgtrNode));
        for (axes_idx = 0; axes_idx < num_axes; axes_idx++)
            {
	    sub_tree = axes[axes_idx];

	    /** It is actually possible for the subwidgets to be in another namespace, so we check that **/
	    htrCheckNSTransition(session, tree, sub_tree);

	    htchtGetStrValue(sub_tree, "label", "", label, sizeof(label));
	    htchtGetStrValue(sub_tree, "axis", "x", axis, sizeof(axis));

	    htrAddScriptInit_va(session, "    chartobj.axes.push({ label: \"%STR&JSSTR\", axis: \"%STR&JSSTR\", });\n", label, axis);

	    htrCheckNSTransitionReturn(session, tree, sub_tree);
            }

        return;
    }


int
htchtInitCall(pHtSession session, pWgtrNode tree)
    {
    char object_source[64];
    char name[64];
    char title[64];
    char chart_type[32];
    char canvas_id[32];
    char title_color[32];
    char legend_position[32];

        htchtGetObjectSource(tree, object_source, sizeof(object_source));
        htchtGetName(tree, name, sizeof(name));
        htchtGetTitle(tree, title, sizeof(title));
        htchtGetType(tree, chart_type, sizeof(chart_type));
        htchtGetCanvasId(tree, canvas_id, sizeof(canvas_id));
        htchtGetTitleColor(tree, title_color, sizeof(title_color));
        htchtGetLegendPosition(tree, legend_position, sizeof(legend_position));

        htrAddScriptInit_va(session, "    var chartobj = {"
                    "x_pos: %INT, "
                    "y_pos: %INT, "
                    "width: %INT, "
                    "height: %INT, "
                    "title_size: %INT, "
                    "start_at_zero: %INT,"
                    "stacked: %INT,"
                    "chart: wgtrGetNodeRef(ns,\"%STR&SYM\"), "
                    "chart_type: '%STR&JSSTR', "
                    "canvas_id: '%STR&SYM', "
                    "osrc: '%STR&JSSTR', "
                    "title: '%STR&JSSTR', "
                    "title_color: '%STR&JSSTR', "
                    "legend_position: '%STR&JSSTR', "
		    "axes: [], "
		    "series: [], "
		    "};\n",
                    htchtGetX(tree),
                    htchtGetY(tree),
                    htchtGetWidth(tree),
                    htchtGetHeight(tree),
                    htchtGetIntValue(tree, "title_size", 12),
                    htrGetBoolean(tree, "start_at_zero", 1),
                    htrGetBoolean(tree, "stacked", 0),
                    name,
                    chart_type,
                    canvas_id,
                    object_source,
                    title,
                    title_color,
                    legend_position
        );

        htchtAddAxesProperties(session, tree);
        htchtAddSeriesProperties(session, tree);
	htrAddScriptInit_va(session, "    cht_init(chartobj);\n");

    return 0;
    }


void
htchtScriptInclude(pHtSession session)
    {
        htrAddScriptInclude(session, "/sys/js/htdrv_chart.js", 0);
        htrAddScriptInclude(session, "/sys/js/chartjs/Chart.js", 0);
        htrAddScriptInclude(session, "/sys/js/ht_utils_string.js", 0);
    }


void
htchtGenHTML(pHtSession session, pWgtrNode tree, int z)
    {
    char buf[32];

        htchtGetCanvasId(tree, buf, sizeof(buf));

        htrAddBodyItem_va(session,"<DIV ID=\"%STR&SYMdiv\"><CANVAS ID=\"%STR&SYM\" width=\"%POS\" height=\"%POS\">\n",
		buf,
		buf,
		htchtGetWidth(tree),
		htchtGetHeight(tree)
	);

        htrAddBodyItem(session,"<P>CHART HERE</P>\n");
        htrAddBodyItem(session,"</CANVAS></DIV>\n");

	htrAddStylesheetItem_va(session, "\t#%STR&SYMdiv { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; } \n",
		buf,
		htchtGetX(tree),
		htchtGetY(tree),
		htchtGetWidth(tree),
		htchtGetHeight(tree),
		z
	);
    }

int
htchtRender(pHtSession session, pWgtrNode tree, int z)
    {
    char* ptr;
    char buf[32];

        wgtrGetPropertyValue(tree,"outer_type",DATA_T_STRING,POD(&ptr));
        if (strcmp(ptr, "widget/chart") != 0)
            return 0;

        if (htchtCheckBrowserSupport(session) != 0) return -1;

        /** Verify that certain parameters are provided **/
        if (htchtGetHeight(tree) == -1) return -1;
        if (htchtGetName(tree, buf, sizeof(buf)) != 0) return -1;

        htchtGenHTML(session, tree, z);
        htchtScriptInclude(session);
        if (htchtInitCall(session, tree) < 0)
	    return -1;

        /** Set the ID for the next chart **/
        HTCHT.idcnt++;

        return 0;
    }


/*** htchtInitialize - register with the ht_render module.
 ***/
int
htchtInitialize()
    {
    pHtDriver drv;

        /** Allocate the driver **/
        drv = htrAllocDriver();
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name, "DHTML Chart Driver");
        strcpy(drv->WidgetName, "chart");
        drv->Render = htchtRender;
        xaAddItem(&(drv->PseudoTypes), "chart-axis");
        xaAddItem(&(drv->PseudoTypes), "chart-series");

        htrAddEvent(drv, "Click");
        htrAddEvent(drv, "DblClick");

        /** Register. **/
        htrRegisterDriver(drv);

        htrAddSupport(drv, "dhtml");

        HTCHT.idcnt = 0;

    return 0;
    }

