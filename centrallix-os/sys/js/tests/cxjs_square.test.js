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

describe('cxjs_square', () =>
    {
    for (const [ input, result ] of [
	// Input            Result
	[ 4,                16        ],
	[ 3,                9         ],
	[ 0.5,              0.25      ],
	[ 1,                1         ],
	[ 0,                0         ],
	[ -0,              +0         ],  // (-0)^2 is +0
	[ -2,               4         ],  // negatives square to positives
	[ -1.5,             2.25      ],
	[ Infinity,         Infinity  ],
	[ -Infinity,        Infinity  ],
	[ 1e200,            Infinity  ],  // overflows to Infinity
	[ Number.MAX_VALUE, Infinity  ],  // also overflows
	[ 1e-200,           0         ],  // underflows to 0
	[ Number.MIN_VALUE, 0         ],  // also underflows
    ])	{
	test(`cxjs_square(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_square(input), result);
	    });
	}

    // null and undefined yield null (no coercion).
    for (const input of [ null, undefined ])
	{
	test(`cxjs_square(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_square(input), null);
	    });
	}

    // Unlike sqrt(), square() does not filter NaN: NaN squares to NaN, not null.
    test(`cxjs_square(${fmt(NaN)}) = ${fmt(NaN)}`, () =>
	{
	assert.equal(env.cxjs_square(NaN), NaN);
	});

    // Non-number inputs are coerced to numbers; those coercing to NaN square
    // to NaN (null is reserved for an explicit null/undefined input).
    for (const [ input, result ] of [
	// Input        Result
	[ '4',          16    ],
	[ '2.5',        6.25  ],
	[ '-2',         4     ],  // negative string coerces to -2, squares positive
	[ '',           0     ],  // empty string coerces to 0
	[ 'foo',        NaN   ],  // non-numeric string coerces to NaN
	[ true,         1     ],
	[ false,        0     ],
	[ [],           0     ],  // empty array coerces to 0
	[ [4],          16    ],  // single-element array coerces to its element
	[ [4, 9],       NaN   ],  // multi-element array coerces to NaN
	[ {},           NaN   ],  // object coerces to NaN
    ])	{
	test(`cxjs_square(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_square(input), result);
	    });
	}
    });
