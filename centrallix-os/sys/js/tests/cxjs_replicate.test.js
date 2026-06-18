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

// String(n)/JSON.stringify collapse distinct values into the same test name
// (e.g. the number 3 and the string "3", or NaN/Infinity/-0); fmt renders them
// verbatim (with quotes for strings, "-0" for negative zero) so names stay unique.
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

// replicate concatenates n copies of s, returning null if s or n is
// null/undefined or if n is negative.
describe('cxjs_replicate', () =>
    {
    for (const [ s, n, result ] of [
	// Str     N      Result
	[ 'ab',    3,     'ababab'                         ],
	[ 'x',     1,     'x'                              ], // single copy
	[ 'ab',    0,     ''                               ], // zero copies
	[ '',      5,     ''                               ], // empty str stays empty
	[ '',      0,     ''                               ],
	[ 'a b',   2,     'a ba b'                         ], // whitespace preserved
	[ 'abc',   3,     'abcabcabc'                      ],

	// n is floored toward zero's neighbor, so fractions truncate down.
	[ 'ab',    3.9,   'ababab'                         ],
	[ 'ab',    0.5,   ''                               ], // floors to 0
	[ 'ab',    2.0,   'abab'                           ],

	// n is coerced to a number before flooring.
	[ 'ab',    '3',   'ababab'                         ], // numeric string
	[ 'ab',    '2.9', 'abab'                           ], // numeric string floored
	[ 'ab',    '',    ''                               ], // ""    -> Number 0
	[ 'ab',    '  ',  ''                               ], // blank -> Number 0
	[ 'ab',    true,  'ab'                             ], // true  -> 1 copy
	[ 'ab',    false, ''                               ], // false -> 0 copies

	// Non-string s is coerced to string.
	[ 5,       3,     '555'                            ],
	[ true,    2,     'truetrue'                       ],
	[ 0,       2,     '00'                             ],
	[ false,   2,     'falsefalse'                     ], // boolean false coerced
	[ NaN,     2,     'NaNNaN'                         ], // NaN value coerced to "NaN"
	[ [1,2],   2,     '1,21,2'                         ], // array via Array.toString
	[ {},      2,     '[object Object][object Object]' ], // object via Object.toString
	[ '😀',    3,     '😀😀😀'                         ], // multi-byte s repeated whole

	// NaN n yields "".
	[ 'ab',    NaN,   ''                               ],
	[ 'ab',    'abc', ''                               ], // String->NaN
    ])	{
	test(`cxjs_replicate(${fmt(s)}, ${fmt(n)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_replicate(s, n), result);
	    });
	}

    // null/undefined s or n short-circuits to null.
    for (const [ s, n ] of [
	[ null,      3         ],
	[ undefined, 3         ],
	[ 'x',       null      ],
	[ 'x',       undefined ],
	[ null,      null      ],
    ])	{
	test(`cxjs_replicate(${fmt(s)}, ${fmt(n)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_replicate(s, n), null);
	    });
	}

    // Negative n returns null.
    for (const [ s, n ] of [
	[ 'ab', -1        ],
	[ 'ab', -5        ],
	[ 'ab', -0.1      ], // Floors away from 0 (-0.1 -> -1), resulting in null.
	[ 'ab', -Infinity ],
    ])	{
	test(`cxjs_replicate(${fmt(s)}, ${fmt(n)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_replicate(s, n), null);
	    });
	}

    // Just below the cap: 254.9 floors to 254 copies (one short of the cap).
    test("cxjs_replicate('a', 254.9) = 254 copies", () =>
	{
	assert.equal(env.cxjs_replicate('a', 254.9), 'a'.repeat(254));
	});

    // n is capped at 255 copies.
    for (const n of [ 255, 255.9, 256, 300, Infinity ])
	{
	test(`cxjs_replicate('ab', ${fmt(n)}) caps at 255 copies`, () =>
	    {
	    assert.equal(env.cxjs_replicate('ab', n), 'ab'.repeat(255));
	    });
	}
    });
