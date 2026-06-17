// Copyright (C) 2026 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

'use strict';
const { describe, test } = require('node:test');
const assert             = require('node:assert/strict');
const env                = require('./_setup');

// Run cxjs_getdate() at a fixed instant: swap in a fake Date pinned to epochMs
// that answers in UTC, so results don't depend on the host timezone.
function getdateAt(epochMs, clockoffset)
    {
    class FakeDate
	{
	constructor()      { this._d = new Date(epochMs); }
	getMilliseconds()  { return this._d.getUTCMilliseconds(); }
	setMilliseconds(v) { return this._d.setUTCMilliseconds(v); }
	getSeconds()       { return this._d.getUTCSeconds(); }
	getMinutes()       { return this._d.getUTCMinutes(); }
	getHours()         { return this._d.getUTCHours(); }
	getDate()          { return this._d.getUTCDate(); }
	getMonth()         { return this._d.getUTCMonth(); }
	getFullYear()      { return this._d.getUTCFullYear(); }
	}

    env.Date = FakeDate;
    env.pg_clockoffset = clockoffset;
    try
	{
	return env.cxjs_getdate();
	}
    finally
	{
	delete env.Date;
	env.pg_clockoffset = 0;
	}
    }

// Build epoch milliseconds from UTC calendar fields (month is 1-based here).
function utc(year, month, day, hour, min, sec, ms)
    {
    return Date.UTC(year, month - 1, day, hour, min, sec, ms || 0);
    }

describe('cxjs_getdate', () =>
    {
    // Format is "M/D/YYYY H:MM:SS". Hour is unpadded,
    // but minute and second are always zero-padded.

    // Test dates with no pg_clockoffset.
    for (const [ when, result ] of [
	// When                              Result
	[ utc(2026,  1,  1,  0,  5,  9),     '1/1/2026 0:05:09'    ],  // midnight; single-digit min/sec padded, hour not
	[ utc(2026, 12, 25, 23, 59, 59),     '12/25/2026 23:59:59' ],  // two-digit fields left unpadded
	[ utc(2026,  6, 15, 10, 10, 10),     '6/15/2026 10:10:10'  ],  // 10 is the pad boundary: no leading zero
	[ utc(2026,  7,  4, 12,  0,  0),     '7/4/2026 12:00:00'   ],  // zero min/sec render as "00"
	[ utc(2026,  3,  9,  7, 30, 45),     '3/9/2026 7:30:45'    ],  // single-digit hour stays unpadded
	[ utc(1999, 12, 31, 23, 59, 59),     '12/31/1999 23:59:59' ],  // four-digit year from a different century
	[ utc(2024,  2, 29, 12,  0,  0),     '2/29/2024 12:00:00'  ],  // leap day
	[ utc(2026,  6, 15, 12, 30, 45, 500),'6/15/2026 12:30:45'  ],  // milliseconds never appear in the output
	[ utc(2026,  1, 31,  0,  0,  0),     '1/31/2026 0:00:00'   ],  // January is month 1 (getMonth() + 1)
	[ utc(2026, 10,  5,  9,  9,  9),     '10/5/2026 9:09:09'   ],  // two-digit month; 9 is just below the pad boundary
    ])	{
	test(`cxjs_getdate() = "${result}"`, () =>
	    {
	    assert.equal(getdateAt(when, 0), result);
	    });
	}

    // Test dates with pg_clockoffset, which shifts the time backward in ms (negative = forward).
    for (const [ when, offset, result ] of [
	// When                            Offset    Result
	[ utc(2026,  6, 15, 12,  0, 30),    60000,  '6/15/2026 11:59:30'  ],  // back 60s, crossing the minute and hour
	[ utc(2026,  6, 15, 12,  0, 30),   -90000,  '6/15/2026 12:02:00'  ],  // forward 90s; minute padded back to "02"
	[ utc(2026,  6, 15,  0,  0, 30),    60000,  '6/14/2026 23:59:30'  ],  // back across the day boundary
	[ utc(2026,  1,  1,  0,  0, 30),    60000,  '12/31/2025 23:59:30' ],  // back across the month and year boundary
	[ utc(2026,  6, 15,  0,  0,  0),      500,  '6/14/2026 23:59:59'  ],  // sub-second offset still rolls the day back
	[ utc(2026,  6, 15, 23, 59, 59),    -2000,  '6/16/2026 0:00:01'   ],  // forward across the day boundary
	[ utc(2025, 12, 31, 23, 59, 59),    -1000,  '1/1/2026 0:00:00'    ],  // forward 1s across the year boundary
    ])	{
	test(`cxjs_getdate() with pg_clockoffset ${offset}ms = "${result}"`, () =>
	    {
	    assert.equal(getdateAt(when, offset), result);
	    });
	}
    });
