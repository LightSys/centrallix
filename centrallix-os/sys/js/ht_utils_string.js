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

function htutil_strpad(str, pad, len) {
	str = new String(str);
	tmp = '';
	for (var i=0; i < len-str.length; i++)
		tmp = pad+tmp;
	return tmp+str;
}

function htutil_encode(s) {
	rs = '';
	for(i=0;i<s.length;i++) {
		if (s[i] == '<') rs += '&lt;';
		else if (s[i] == '>') rs += '&gt;';
		else if (s[i] == '&') rs += '&amp;';
		else if (s[i] == ' ') rs += '&nbsp;';
		else rs += s[i];
	}
	return rs;
}

// vim: set ts=4 :
