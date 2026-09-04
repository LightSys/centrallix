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

// JSON.stringify collapses NaN/Infinity to "null", omits undefined, and renders
// -0 as "0", which would make distinct edge-case rows share a test name; fmt
// renders those values verbatim (and -0 distinctly from 0) while otherwise
// matching JSON.stringify, so names stay unique.
function fmt(v)
    {
    if (Array.isArray(v))
	return '[' + v.map(fmt).join(',') + ']';
    if (v !== null && typeof v === 'object')
	return '{' + Object.keys(v).map((k) => JSON.stringify(k) + ':' + fmt(v[k])).join(',') + '}';
    if (Object.is(v, -0))
	return '-0';
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

describe('cxjs_abs', () =>
    {
    for (const [ input, result ] of [
	// Input        Result
	[ 5,            5         ],
	[ 5.5,          5.5       ],
	[ -5,           5         ],
	[ -5.5,         5.5       ],
	[ 0,            0         ],
	[ -0,           0         ],
	[ Infinity,     Infinity  ],
	[ -Infinity,    Infinity  ],
	[ NaN,          NaN       ],

	// Extreme finite magnitudes survive unchanged (no overflow/underflow).
	// Input                    Result
	[ Number.MAX_VALUE,         Number.MAX_VALUE ],
	[ -Number.MAX_VALUE,        Number.MAX_VALUE ],
	[ Number.MIN_VALUE,         Number.MIN_VALUE ],  // smallest subnormal
	[ -Number.MIN_VALUE,        Number.MIN_VALUE ],
    ])	{
	test(`cxjs_abs(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_abs(input), result);
	    });
	}

    // null and undefined yield null instead of being coerced.
    for (const input of [ null, undefined ])
	{
	test(`cxjs_abs(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_abs(input), null);
	    });
	}

    // Non-number inputs are coerced to number before taking the magnitude.
    for (const [ input, result ] of [
	// Input        Result
	[ '5',          5         ],
	[ '-5',         5         ],
	[ '3.14',       3.14      ],
	[ '',           0         ],  // empty string coerces to 0
	[ 'foo',        NaN       ],  // non-numeric string coerces to NaN
	[ true,         1         ],
	[ false,        0         ],
	[ [],           0         ],  // empty array coerces to 0
	[ [5],          5         ],  // single-element array coerces to its element
	[ [5, 6],       NaN       ],  // multi-element array coerces to NaN
	[ {},           NaN       ],  // object coerces to NaN

	// String coercion follows JS Number() rules: surrounding whitespace is
	// stripped, hex and exponential literals parse, and all-whitespace is 0.
	// Input        Result
	[ ' 5 ',        5         ],  // surrounding whitespace stripped
	[ '  3.14  ',   3.14      ],
	[ '0x10',       16        ],  // hex literal
	[ '1e3',        1000      ],  // exponential literal
	[ '-0',         0         ],  // string negative zero -> +0 magnitude
	[ '  ',         0         ],  // all-whitespace coerces to 0
	[ ['5'],        5         ],  // single string-element array coerces to its element
	[ [' '],        0         ],  // single whitespace-element array coerces to 0
    ])	{
	test(`cxjs_abs(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_abs(input), result);
	    });
	}
    });
