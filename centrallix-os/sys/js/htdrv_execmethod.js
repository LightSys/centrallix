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


/** Set timer **/
function ex_action_domethod(aparam)
    {
    o = aparam.Objname?aparam.Objname:this.Objname;
    m = aparam.Method?aparam.Method:this.Methodname;
    p = aparam.Parameter?aparam.Parameter:this.Methodparam;
    if (!o || !m || !p) return false;
    this.HiddenLayer.load(o + '?ls__mode=execmethod&ls__methodname=' + (escape(m).replace(/\//g,'%2f')) + '&ls__methodparam=' + (escape(p).replace(/\//g,'%2f')), 64);
    return true;
    }

/** Timer initializer **/
function ex_init(o,m,p)
    {
    ex = new Object();
    ex.Objname = o;
    ex.Methodname = m;
    ex.Methodparam = p;
    ex.ActionExecuteMethod = ex_action_domethod;
    ex.HiddenLayer = new Layer(64);
    return ex;
    }

