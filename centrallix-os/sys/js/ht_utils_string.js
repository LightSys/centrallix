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
    s = String(s);
    for(var i=0;i<s.length;i++) 
        {
	if (s.charAt(i) == '<') rs += '&lt;';	
	else if (s.charAt(i) == '>') rs += '&gt;';
	else if (s.charAt(i) == '&') rs += '&amp;';
	else if (s.charAt(i) == ' ') rs += '&nbsp;';
	else if (s.charAt(i) == "'") rs += '&#39;';
	else if (s.charAt(i) == "\"") rs += '&quot;';
	else rs += s.charAt(i);
        }
    return rs;
    }

function htutil_nlbr(s)
    {
    var re = /\n\r?/g;
    return String(s).replace(re, "<br>");
    }

function htutil_subst_last(str,subst) 
    {
    if(!str || !subst)
	return str;
    return str.substring(0,str.length-subst.length)+subst;
    }

function htutil_rtrim(str) 
    {
    str = new String(str);
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

function tohex16(n)
    {
    var digits = ['0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'];
    return '' + digits[(n/4096)&0xF] + digits[(n/256)&0xF] + digits[(n/16)&0xF] + digits[n&0xF];
    }

function htutil_escape(s)
    {
    var new_s = String(escape(s));
    var re = /\//g;
    var re2 = /\+/g;
    return new_s.replace(re, "%2f").replace(re2, "%2b");
    }

function htutil_obscure(s)
    {
    if (!obscure_data) return s;
    var new_s = String('');
    s = String(s);
    for (var i=0;i<s.length;i++)
	{
	if (s.charAt(i) >= '0' && s.charAt(i) <= '9')
	    new_s += String.fromCharCode(Math.random()*10+48);
	else if (s.charAt(i) >= 'A' && s.charAt(i) <= 'Z')
	    new_s += String.fromCharCode(Math.random()*26+65);
	else if (s.charAt(i) >= 'a' && s.charAt(i) <= 'z')
	    new_s += String.fromCharCode(Math.random()*26+97);
	else
	    new_s += s.charAt(i);
	}
    return new_s;
    }

