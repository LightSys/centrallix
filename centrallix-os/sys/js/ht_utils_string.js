/*
**  ht_utils_string.js
**  String manipulation functions
*/

function dt_strpad(str, pad, len) {
	str = new String(str);
	for (var i=0; i < len-str.length; i++)
		str = pad+str;
	return str;
}
