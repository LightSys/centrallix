// Copyright (C) 1998-2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function htutil_strpad(str, pad, len) 
    {
    str = new String(str);
    var tmp = '';
    for (var i=0; i < len-str.length; i++)
	tmp = pad+tmp;
    return tmp+str;
    }

function htutil_unpack(str) 
    {
    str = new String(str);
    var ret="";
    for(var i=0;i<str.length;i+=2)
	{
	ret+=unescape("%"+str.substr(i,2));
	}
    return ret;
    }

function htutil_encode(s) 
    {
    var rs = '';
    for(var i=0;i<s.length;i++) 
        {
	if (s.charAt(i) == '<') rs += '&lt;';	
	else if (s.charAt(i) == '>') rs += '&gt;';
	else if (s.charAt(i) == '&') rs += '&amp;';
	else if (s.charAt(i) == ' ') rs += '&nbsp;';
	else rs += s.charAt(i);
        }
    return rs;
    }

function htutil_subst_last(str,subst) 
    {
    if(!str || !subst)
	return str;
    return str.substring(0,str.length-subst.length)+subst;
    }

function htutil_rtrim(str) 
    {
    for (var i=str.length-1; i>=0 && (str.charAt(i) == ' ' || str.charAt(i)=='\t' || str.charAt(i)=='\xCA'); i--);
    return str.substring(0, i+1);
    }

/** return common part of string **/
function htutil_common(str1,str2) 
    {
    var cnt = 0;
    if (!str1 || !str2) return "";
    for (var i=0;i<str1.length;i++) if (str1.charAt(i) != str2.charAt(i)) break; else cnt++;
    return str1.substring(0,cnt);
    }

/** compare two URL's for same server **/
function htutil_url_cmp(str1,str2) 
    {
    var common = htutil_common(str1,str2);
    var cmp_re = /^http:\/+[a-zA-Z._0-9:]+\//i;
    if (common.search(cmp_re) >= 0)
	return true;
    else
	return false;
    }

