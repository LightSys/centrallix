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

function htutil_days_in_month(d) {
	switch (d.getMonth()) {
		case 0: return 31;
		case 1: return (htutil_is_leapyear(d)?29:28);
		case 2: return 31;
		case 3: return 30;
		case 4: return 31;
		case 5: return 30;
		case 6: return 31;
		case 7: return 31;
		case 8: return 30;
		case 9: return 31;
		case 10: return 30;
		case 11: return 31;
	}
}

function htutil_is_leapyear(d) {
	var yr = d.getYear()+1900;
	if (yr % 4 == 0) {
		if (yr % 100 == 0 && yr % 400 != 0) {
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

// Load indication
if (window.pg_scripts) pg_scripts['ht_utils_date.js'] = true;
