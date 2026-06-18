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

// replicate concatenates n copies of s, returning null if s or n is
// null/undefined or if n is negative.
describe('cxjs_replicate', () =>
    {
    for (const [ s, n, result ] of [
	// Str     N      Result
	[ 'ab',    3,     'ababab'    ],
	[ 'x',     1,     'x'         ], // single copy
	[ 'ab',    0,     ''          ], // zero copies
	[ '',      5,     ''          ], // empty str stays empty
	[ '',      0,     ''          ],
	[ 'a b',   2,     'a ba b'    ], // whitespace preserved
	[ 'abc',   3,     'abcabcabc' ],

	// n is floored toward zero's neighbor, so fractions truncate down.
	[ 'ab',    3.9,   'ababab'    ],
	[ 'ab',    0.5,   ''          ], // floors to 0
	[ 'ab',    2.0,   'abab'      ],

	// Non-string s is coerced to string.
	[ 5,       3,     '555'       ],
	[ true,    2,     'truetrue'  ],
	[ 0,       2,     '00'        ],

	// NaN n yields "".
	[ 'ab',    NaN,   ''          ],
	[ 'ab',    'abc', ''          ], // String->NaN
    ])	{
	test(`cxjs_replicate(${JSON.stringify(s)}, ${String(n)}) = ${JSON.stringify(result)}`, () =>
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
	test(`cxjs_replicate(${s}, ${n}) = null`, () =>
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
	test(`cxjs_replicate(${JSON.stringify(s)}, ${String(n)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_replicate(s, n), null);
	    });
	}

    // n is capped at 255 copies.
    for (const n of [ 255, 256, 300, Infinity ])
	{
	test(`cxjs_replicate('ab', ${String(n)}) caps at 255 copies`, () =>
	    {
	    assert.equal(env.cxjs_replicate('ab', n), 'ab'.repeat(255));
	    });
	}
    });
