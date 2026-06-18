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

// JSON.stringify collapses NaN/Infinity to "null" and omits undefined, which
// would make distinct edge-case rows share a test name; fmt renders those
// values verbatim (and otherwise matches JSON.stringify) so names stay unique.
function fmt(v)
    {
    if (Array.isArray(v))
	return '[' + v.map(fmt).join(',') + ']';
    if (v !== null && typeof v === 'object')
	return '{' + Object.keys(v).map((k) => JSON.stringify(k) + ':' + fmt(v[k])).join(',') + '}';
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
    ])	{
	test(`cxjs_abs(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_abs(input), result);
	    });
	}
    });
