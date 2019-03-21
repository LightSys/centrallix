#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
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
htchtGetStrValue(pWgtrNode tree, char* prop_name, char* default_val, char* buf)
    {
    int rval;
    char* ptr;

        /** We're not passing the buf into this function call because of convoluted c reasons **/
        rval = wgtrGetPropertyValue(tree,prop_name,DATA_T_STRING,POD(&ptr));

        if(rval == 0)
            {
            strtcpy(buf, ptr, sizeof(buf));
            return 0;
            }
        else
            {
            strtcpy(buf, default_val, sizeof(buf));
            return -1;
            }
    }


int
htchtGetWidth(pWgtrNode tree){return htchtGetIntValue(tree, "w", -1);}

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
htchtGetName(pWgtrNode tree, char* buf) {return htchtGetStrValue(tree, "name", "", buf);}

int
htchtGetType(pWgtrNode tree, char* buf)
    {
    char chart_types[16][16] = {"line", "bar"};
    int rval;
    char err_msg[256];
    int found = 0;
    int type_idx;

        rval = htchtGetStrValue(tree, "chart_type", "", buf);

        if (rval != 0)
            return rval;

        sprintf(err_msg, "%s is not a valid chart type. Supported types are: ", buf);

        for (type_idx = 0; type_idx < 16; type_idx++)
            {
            if (strcmp(buf, chart_types[type_idx]) == 0)
                found = 1;

            strcat(err_msg, chart_types[type_idx]);
            strcat(err_msg, ", ");
            }

        if (found) return 0;
        else
            {
            mssError(1, "HTCHT", err_msg);
            strcpy(buf, "");
            return -1;
            }
    }

int
htchtGetTextColor(pWgtrNode tree, char* buf) {return htchtGetStrValue(tree, "textcolor", "", buf);}

int
htchtGetObjectSource(pWgtrNode tree, char* buf)
    {
        /**  The ObjectSource can be provided in the configuration for this widget,
         **  but it is usually determined by the javascript driver.
         **  Setting ObjectSource to an empty string indicates that the javascript should determine the ObjectSource.
         **/
        return htchtGetStrValue(tree, "objectsource", "", buf);
    }

void
htchtGetCanvasId(pWgtrNode tree, char* buf){sprintf(buf, "cht%dcanvas", HTCHT.idcnt);}

void
htchtGetSeriesProperties(pHtSession session, pWgtrNode tree, char* buf)
    {
    int num_series;
    int series_idx;
    pWgtrNode series[32];
    pWgtrNode sub_tree;
    char series_object[256];

    char label[32];
    char color[32];

        buf[0] = '\0';
        strcat(buf, "[");

        num_series = wgtrGetMatchingChildList(tree, "widget/chart-series", series, sizeof(series)/sizeof(pWgtrNode));
        for (series_idx = 0; series_idx < num_series; series_idx++)
            {
           sub_tree = series[series_idx];

            /** It is actually possible for the subwidgets to be in another namespace, so we check that **/
            htrCheckNSTransition(session, tree, sub_tree);

            htchtGetStrValue(sub_tree, "label", "", label);
            htchtGetStrValue(sub_tree, "color", "", color);

            htrCheckNSTransitionReturn(session, tree, sub_tree);

            sprintf(series_object, "{"
                                       "label: \"%s\", "
                                       "color: \"%s\""
                                   "},",
                                   label,
                                   color);

            strcat(buf, series_object);
            }
        strcat(buf, "]");

        return;
    }


void
htchtGetAxesProperties(pHtSession session, pWgtrNode tree, char* buf)
    {
    int num_axes;
    int axes_idx;
    pWgtrNode axes[32];
    pWgtrNode sub_tree;
    char axis_object[256];

    char label[32];
    int bold;

        buf[0] = '\0';
        strcat(buf, "[");

        num_axes = wgtrGetMatchingChildList(tree, "widget/chart-axis", axes, sizeof(axes)/sizeof(pWgtrNode));
        for (axes_idx = 0; axes_idx < num_axes; axes_idx++)
            {
                sub_tree = axes[axes_idx];

                /** It is actually possible for the subwidgets to be in another namespace, so we check that **/
                htrCheckNSTransition(session, tree, sub_tree);

                htchtGetStrValue(sub_tree, "label", "", label);
                bold = htrGetBoolean(tree, "bold", 0);

                htrCheckNSTransitionReturn(session, tree, sub_tree);

                sprintf(axis_object, "{"
                                     "label: \"%s\", "
                                       "bold: \"%d\""
                                       "},",
                        label,
                        bold);

                strcat(buf, axis_object);
            }
        strcat(buf, "]");

        return;
    }



void
htchtInitCall(pHtSession session, pWgtrNode tree)
    {
    char object_source[64];
    char name[64];
    char chart_type[64];
    char canvas_id[64];
    char text_color[64];
    char axes_properties[2048];
    char series_properties[2048];

        htchtGetObjectSource(tree, object_source);
        htchtGetName(tree, name);
        htchtGetType(tree, chart_type);
        htchtGetCanvasId(tree, canvas_id);
        htchtGetTextColor(tree, text_color);
        htchtGetAxesProperties(session, tree, axes_properties);
        htchtGetSeriesProperties(session, tree, series_properties);

        htrAddScriptInit_va(session, "    "
                "cht_init({"
                    "x_pos: %INT,"
                    "y_pos: %INT,"
                    "width: %INT,"
                    "height: %INT,"
                    "chart: wgtrGetNodeRef(ns,\"%STR&SYM\"),"
                    "chart_type: '%STR',"
                    "canvas_id: '%STR&SYM',"
                    "osrc: '%STR',"
                    "text_color: '%STR',"
                    "axes: %STR,"
                    "series: %STR"
                "});\n",
                    htchtGetX(tree),
                    htchtGetY(tree),
                    htchtGetWidth(tree),
                    htchtGetHeight(tree),
                    name,
                    chart_type,
                    canvas_id,
                    object_source,
                    text_color,
                    axes_properties,
                    series_properties
        );
    }


void
htchtScriptInclude(pHtSession session)
    {
        htrAddScriptInclude(session, "/sys/js/htdrv_chart.js", 0);
        htrAddScriptInclude(session, "/sys/js/chartjs/Chart.js", 0);
        htrAddScriptInclude(session, "/sys/js/ht_utils_string.js", 0);
    }


void
htchtGenHTML(pHtSession session, pWgtrNode tree)
    {
    char buf[32];
        htchtGetCanvasId(tree, buf);

        htrAddBodyItem_va(session,"<DIV> <CANVAS ID=\"%STR&SYM\">\n", buf);
        htrAddBodyItem(session,"<P>CHART HERE</P>\n");
        htrAddBodyItem(session,"</CANVAS> </DIV>\n");
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
        if (htchtGetName(tree, buf) != 0) return -1;

        htchtGenHTML(session, tree);
        htchtScriptInclude(session);
        htchtInitCall(session, tree);

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
        strcpy(drv->Name,"DHTML Chart Driver");
        strcpy(drv->WidgetName,"chart");
        drv->Render = htchtRender;
        xaAddItem(&(drv->PseudoTypes), "chart-axis");
        xaAddItem(&(drv->PseudoTypes), "chart-series");

        htrAddEvent(drv,"Click");
        htrAddEvent(drv,"DblClick");

        /** Register. **/
        htrRegisterDriver(drv);

        htrAddSupport(drv, "dhtml");

        HTCHT.idcnt = 0;

        return 0;
}
