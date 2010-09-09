// Copyright (C) 1998-2010 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function uw_action_open(aparam)
    {
    }

function uw_action_close(aparam)
    {
    }

function uw_init(l, wparam)
    {
    ifc_init_widget(l);

    // Params
    //   shared (boolean) - allow same window to be referenced by multiple .cmp or .app
    //   multi (boolean) - allow multiple instances of window
    //   routing (enum int) - 0=mostrecent, 1=topmost, 2=all
    //   path (string) - path to .app for new window
    //   w (int) - width of new window
    //   h (int) - height of new window
    l.param = wparam;

    // internal
    l.windowlist = [];

    // Callbacks

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Loaded");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("Open", uw_action_open);
    ia.Add("Close", uw_action_close);

    // Values
    var iv = l.ifcProbeAdd(ifValue);

    return l;
    }
