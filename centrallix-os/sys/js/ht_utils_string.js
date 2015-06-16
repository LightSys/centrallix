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

function htutil_encode(s, allowbrk) 
    {
    var rs = '';
    if (s === null) return s;
    s = String(s);
    for(var i=0;i<s.length;i++) 
        {
	if (s.charAt(i) == '<') rs += '&lt;';	
	else if (s.charAt(i) == '>') rs += '&gt;';
	else if (s.charAt(i) == '&') rs += '&amp;';
	else if (s.charAt(i) == ' ' && !allowbrk) rs += '&nbsp;';
	else if (s.charAt(i) == "'") rs += '&#39;';
	else if (s.charAt(i) == "\"") rs += '&quot;';
	else if (s.charAt(i) == "-" && !allowbrk) rs += '&#8209;';
	else if (s.charAt(i) == "," && s.charAt(i+1) != " " && allowbrk) rs += ',&#8203;';
	else rs += s.charAt(i);
        }
    return rs;
    }

function htutil_nlbr(s)
    {
    if (s === null) return s;
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

function htutil_escape_cssval(s)
    {
    var escchars = "!$%&()*+,.:=?@[]^`|~;{}<>/\\\"'";
    if (s == null)
	return '';
    s = String(s);
    if (s.match(/expression/i))
	return '';
    if (s.match(/javascript/i))
	return '';
    var new_s = '';
    for(var i=0;i<s.length;i++) 
        {
	var c = s.charAt(i);
	if (escchars.indexOf(c) >= 0)
	    new_s += '\\';
	new_s += c;
	}
    return new_s;
    }

function htutil_obscure(s)
    {
    if (!obscure_data) return s;
    if (s === null) return s;
    var new_s = String('');
    s = String(s);
    
    for (var i=0;i<s.length;i++)
	{
	// initial scrambling
	if (s.charAt(i) >= '0' && s.charAt(i) <= '9')
	    new_s += String.fromCharCode(Math.random()*10+48);
	else if (s.charAt(i) >= 'A' && s.charAt(i) <= 'Z')
	    new_s += String.fromCharCode(Math.random()*26+65);
	else if (s.charAt(i) >= 'a' && s.charAt(i) <= 'z')
	    new_s += String.fromCharCode(Math.random()*26+97);
	else
	    new_s += s.charAt(i);

	// add/remove a character
	if (Math.random() < 0.1 && new_s.length > 1)
	    {
	    var idx = new_s.length - 2;
	    if (new_s.charAt(idx) >= '0' && new_s.charAt(idx) <= '9' && new_s.charAt(idx+1) >= '0' && new_s.charAt(idx+1) <= '9')
		new_s = new_s.substring(0, new_s.length - 1);
	    else if (new_s.charAt(idx) >= 'a' && new_s.charAt(idx) <= 'z' && new_s.charAt(idx+1) >= 'a' && new_s.charAt(idx+1) <= 'z')
		new_s = new_s.substring(0, new_s.length - 1);
	    else if (new_s.charAt(idx) >= 'A' && new_s.charAt(idx) <= 'Z' && new_s.charAt(idx+1) >= 'A' && new_s.charAt(idx+1) <= 'Z')
		new_s = new_s.substring(0, new_s.length - 1);
	    }
	if (Math.random() < 0.1 && new_s.length > 1)
	    {
	    var idx = new_s.length - 2;
	    if (new_s.charAt(idx) >= '0' && new_s.charAt(idx) <= '9' && new_s.charAt(idx+1) >= '0' && new_s.charAt(idx+1) <= '9')
		new_s += String.fromCharCode(Math.random()*10+48);
	    else if (new_s.charAt(idx) >= 'a' && new_s.charAt(idx) <= 'z' && new_s.charAt(idx+1) >= 'a' && new_s.charAt(idx+1) <= 'z')
		new_s += String.fromCharCode(Math.random()*26+97);
	    else if (new_s.charAt(idx) >= 'A' && new_s.charAt(idx) <= 'Z' && new_s.charAt(idx+1) >= 'A' && new_s.charAt(idx+1) <= 'Z')
		new_s += String.fromCharCode(Math.random()*26+65);
	    }
	}

    return new_s;
    }


// stylize -- wrap a string with a <span> that contains formatting
// for font face, font size, color, bold, italic, shadow,
// underlining, etc.
//
function htutil_getstyle(widget, prefix, defaults)
    {
    // prefixing?
    if (prefix)
	prefix += "_";
    else
	prefix = "";

    // text color
    var color = wgtrGetServerProperty(widget, prefix + "textcolor");
    if (!color)
	{
	color = wgtrGetServerProperty(widget, prefix + "fgcolor");
	if (!color && defaults)
	    color = defaults.textcolor;
	}

    // visibility
    var visib = wgtrGetServerProperty(widget, prefix + "visible");
    if (!visib && defaults)
	visib = defaults.visible;

    // style
    var style = wgtrGetServerProperty(widget, prefix + "style");
    if (!style && defaults)
	style = defaults.style;

    // font size
    var font_size = wgtrGetServerProperty(widget, prefix + "font_size");
    if (!font_size && defaults)
	font_size = defaults.font_size;

    // font
    var font = wgtrGetServerProperty(widget, prefix + "font");
    if (!font && defaults)
	font = defaults.font;

    // background color
    var bgcolor = wgtrGetServerProperty(widget, prefix + "bgcolor");
    if (!bgcolor && defaults)
	bgcolor = defaults.bgcolor;

    // padding
    var padding = wgtrGetServerProperty(widget, prefix + "padding");
    if (!padding && defaults)
	padding = defaults.padding;

    // radius
    var radius = wgtrGetServerProperty(widget, prefix + "border_radius");
    if (!radius && defaults)
	radius = defaults.border_radius;

    // border color
    var bcolor = wgtrGetServerProperty(widget, prefix + "border_color");
    if (!bcolor && defaults)
	bcolor = defaults.border_color;

    // shadow information
    var scolor = wgtrGetServerProperty(widget, prefix + "shadow_color");
    if (!scolor && defaults)
	scolor = defaults.shadow_color;
    var sradius = wgtrGetServerProperty(widget, prefix + "shadow_radius");
    if (!sradius && defaults)
	sradius = defaults.shadow_radius;
    var soffset = wgtrGetServerProperty(widget, prefix + "shadow_offset");
    if (!soffset && defaults)
	soffset = defaults.shadow_offset;
    var sangle = wgtrGetServerProperty(widget, prefix + "shadow_angle");
    if (!sangle && defaults)
	sangle = defaults.shadow_angle;
    var sloc = wgtrGetServerProperty(widget, prefix + "shadow_location");
    if (!sloc && defaults)
	sloc = defaults.shadow_location;

    // alignment
    var align = wgtrGetServerProperty(widget, prefix + "align");
    if (!align && defaults)
	align = defaults.align;

    // wrapping
    var wrap = wgtrGetServerProperty(widget, prefix + "wrap");
    if (!wrap && defaults)
	wrap = defaults.wrap;

    // Assemble the text.
    var str = '';
    if (color)
	str += 'color:' + htutil_escape_cssval(color) + '; ';
    if (font_size)
	str += 'font-size:' + htutil_escape_cssval(font_size) + 'px; ';
    if (style == 'italic')
	str += 'font-style:italic; ';
    if (style == 'bold')
	str += 'font-weight:bold; ';
    if (style == 'underline')
	str += 'text-decoration:underline; ';
    if (font)
	str += 'font-family:"' + htutil_escape_cssval(font) + '"; ';
    if (bgcolor)
	str += 'background-color:' + htutil_escape_cssval(bgcolor) + '; ';
    if (padding)
	str += 'padding:' + htutil_escape_cssval(padding) + 'px; ';
    if (radius)
	str += 'border-radius:' + htutil_escape_cssval(radius) + 'px; ';
    if (bcolor)
	str += 'border: 1px solid ' + htutil_escape_cssval(bcolor) + '; ';
    if (wrap == 'no')
	str += 'white-space:no-wrap; ';
    if (visib == 'no')
	str += 'visibility:hidden; ';
    if (align)
	str += 'text-align:' + htutil_escape_cssval(align) + '; ';
    if (scolor && sradius)
	{
	str += 'box-shadow:' + ((sloc=='inside')?'inset ':'') + 
	    htutil_escape_cssval(Math.round(Math.sin(sangle*Math.PI/180)*soffset*10)/10) + 'px ' +
	    htutil_escape_cssval(Math.round(Math.cos(sangle*Math.PI/180)*(-soffset)*10)/10) + 'px ' +
	    htutil_escape_cssval(sradius) + 'px ' + 
	    htutil_escape_cssval(scolor) + '; ';
	}
    //span = '<span style="' + htutil_encode(span,true) + '">' + str + '</span>';
    return str;
    }


// Load indication
if (window.pg_scripts) pg_scripts['ht_utils_string.js'] = true;
