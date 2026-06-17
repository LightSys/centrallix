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
    ])	{
	test(`cxjs_minus(${JSON.stringify(a)}, ${JSON.stringify(b)}) = ${JSON.stringify(result)}`, () =>
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
	test(`cxjs_minus(${JSON.stringify(a)}, ${JSON.stringify(b)}) = null`, () =>
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
    ])	{
	test(`cxjs_minus(${JSON.stringify(a)}, ${JSON.stringify(b)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_minus(a, b), result);
	    });
	}
    });
