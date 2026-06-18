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

describe('cxjs_sqrt', () =>
    {
    for (const [ input, result ] of [
	// Input        Result
	[ 4,            2          ],
	[ 9,            3          ],
	[ 0.25,         0.5        ],
	[ 2,            Math.SQRT2 ],
	[ 1,            1          ],
	[ 0,            0          ],
	[ -0,          -0          ],  // preserved: Math.sqrt(-0) is -0, not NaN
	[ Infinity,     Infinity   ],
    ])	{
	test(`cxjs_sqrt(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sqrt(input), result);
	    });
	}

    // Negative inputs and NaN have no real root and yield null.
    for (const input of [ -1, -5.5, -Infinity, NaN ])
	{
	test(`cxjs_sqrt(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_sqrt(input), null);
	    });
	}

    // null and undefined yield null (no coercion).
    for (const input of [ null, undefined ])
	{
	test(`cxjs_sqrt(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_sqrt(input), null);
	    });
	}

    // Non-number inputs are coerced to numbers.
    // If they yield NaN, sqrt() returns null.
    for (const [ input, result ] of [
	// Input        Result
	[ '4',          2     ],
	[ '2.25',       1.5   ],
	[ '',           0     ],  // empty string coerces to 0
	[ 'foo',        null  ],  // non-numeric string coerces to NaN
	[ true,         1     ],
	[ false,        0     ],
	[ [],           0     ],  // empty array coerces to 0
	[ [4],          2     ],  // single-element array coerces to its element
	[ [4, 9],       null  ],  // multi-element array coerces to NaN
	[ {},           null  ],  // object coerces to NaN
    ])	{
	test(`cxjs_sqrt(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sqrt(input), result);
	    });
	}
    });
