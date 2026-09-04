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

// JSON.stringify collapses NaN/Infinity to "null", drops undefined, and
// renders -0 as "0", which would make distinct edge-case rows share a test
// name; fmt renders those verbatim so names stay unique.
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

describe('cxjs_minus', () =>
    {
    // Numeric operands subtract. NaN propagates.
    for (const [ a, b, result ] of [
	// a            b           Result
	[ 5,            3,          2         ],
	[ 3,            5,         -2         ],
	[ 0,            0,          0         ],
	[ 10.5,         0.5,        10        ],
	[ -1,          -1,          0         ],
	[ -5,           3,         -8         ],
	[ 3,           -5,          8         ],
	[ Infinity,     1,          Infinity  ],
	[ 1,            Infinity,  -Infinity  ],
	[ Infinity,     Infinity,   NaN       ],
	[ -Infinity,   -Infinity,   NaN       ],
	[ NaN,          1,          NaN       ],
	[ 1,            NaN,        NaN       ],
	[ true,         1,          0         ],  // Booleans are not strings: true -> 1.
	[ false,        false,      0         ],  // false -> 0.
	[ 5,            0,          5         ],
	[ 0,            5,         -5         ],
	[ true,         false,      1         ],  // 1 - 0.
	[ 5e-324,       5e-324,     0         ],  // smallest denormals cancel to 0.
    ])	{
	test(`cxjs_minus(${fmt(a)}, ${fmt(b)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_minus(a, b), result);
	    });
	}

    // A null/undefined operand yields null.
    for (const [ a, b ] of [
	[ null,      5         ],
	[ 5,         null      ],
	[ null,      null      ],
	[ undefined, 5         ],
	[ 5,         undefined ],
    ])	{
	test(`cxjs_minus(${fmt(a)}, ${fmt(b)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_minus(a, b), null);
	    });
	}

    // When either operand is a string, both are coerced to strings and a
    // matching suffix b is stripped from the end of a.
    for (const [ a, b, result ] of [
	// a            b           Result
	[ 'hello',      'lo',       'hel'     ],
	[ 'hello',      'hello',    ''        ],
	[ 'hello',      'xyz',      'hello'   ],  // No match: unchanged.
	[ 'hello',      '',         'hello'   ],  // Empty suffix: unchanged.
	[ 'hello',      'HELLO',    'hello'   ],  // Case-sensitive: unchanged.
	[ 'abcabc',     'abc',      'abc'     ],  // Only the trailing match.
	[ '',           '',         ''        ],
	[ 'a',          'abc',      'a'       ],  // b longer than a: unchanged.
	[ 100,          '0',        '10'      ],  // Coercion: 100 -> '100'.
	[ '5',          5,          ''        ],
	[ 5,            '5',        ''        ],

	// Only a suffix is stripped: the match must sit at the very end.
	// a            b           Result
	[ 'abcabc',     'bc',       'abca'    ],  // trailing 'bc' removed (lastIndexOf is at the end).
	[ 'aXbXc',      'X',        'aXbXc'   ],  // 'X' occurs, but not at the end: unchanged.

	// Overlapping/repeated suffix.
	// a            b           Result
	[ 'aaa',        'aa',       'a'       ],  // lastIndexOf('aa') = 1 = len-2: strips one.
	[ 'aaaa',       'aa',       'aa'      ],  // lastIndexOf('aa') = 2 = len-2: strips one.
	[ '',           'abc',      ''        ],  // empty a, longer b: lastIndexOf = -1, but -1 != 0-3, so a unchanged ('').

	// Coercion by only one string.
	// a            b           Result
	[ true,         'e',        'tru'     ],
	[ 'true',       true,       ''        ],
	[ '12',         2,          '1'       ],
	[ NaN,          'N',        'Na'      ],
    ])	{
	test(`cxjs_minus(${fmt(a)}, ${fmt(b)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_minus(a, b), result);
	    });
	}

    // Signed-zero: subtracting equal values gives +0, even -0 - -0.
    // JSON.stringify(-0) is "0", so name these explicitly (Object.is
    // distinguishes -0 from +0 in assert/strict).
    test('cxjs_minus(-0, -0) = +0', () =>
	{
	assert.equal(env.cxjs_minus(-0, -0), 0);
	});
    });
