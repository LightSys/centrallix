// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function tm_expire(tm)
    {
    if (tm.autoreset)
	tm.timerid = setTimeout(tm_expire,tm.msec,tm);
    else
	tm.timerid = null;
    if (tm.EventExpire != null)
	{
	eparam = new Object();
	eparam.Caller = tm;
	cn_activate(tm, 'Expire', eparam);
	delete eparam;
	}
    }

function tm_action_settimer(aparam)
    {
    if (this.timerid != null) clearTimeout(this.timerid)
    if (aparam.AutoReset != null) this.autoreset = aparam.AutoReset;
    this.timerid = setTimeout(tm_expire, aparam.Time, this);
    }

function tm_action_canceltimer(aparam)
    {
    if (this.timerid != null) clearTimeout(this.timerid)
    this.timerid = null;
    }

function tm_init(param)
    {
    tm = new Object();
    tm.autostart = param.autostart;
    tm.autoreset = param.autoreset;
    tm.msec = param.time;
    tm.ActionSetTimer = tm_action_settimer;
    tm.ActionCancelTimer = tm_action_canceltimer;
    if (param.autostart)
	tm.timerid = setTimeout(tm_expire, param.time, tm);
    else
	tm.timerid = null;
    return tm;
    }
