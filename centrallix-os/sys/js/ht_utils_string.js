/*
**  ht_utils_string.js
**  String manipulation functions
*/

function htutil_strpad(str, pad, len) {
	str = new String(str);
	tmp = '';
	for (var i=0; i < len-str.length; i++)
		tmp = pad+tmp;
	return tmp+str;
}
