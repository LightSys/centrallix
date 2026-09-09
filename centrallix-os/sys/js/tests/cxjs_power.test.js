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
// renders those values verbatim (and recurses into arrays/objects) so names
// stay unique.
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

describe('cxjs_power', () =>
    {
    for (const [ n, p, result ] of [
	// n        p        Result
	[  2,       3,        8          ],
	[  5,       2,        25         ],
	[  2,       10,       1024       ],
	[  7,       1,        7          ],  // identity exponent
	[  9,       0,        1          ],  // zero exponent
	[  0,       0,        1          ],  // 0^0 is defined as 1
	[  0,       5,        0          ],
	[  1,       100,      1          ],
	[  2,      -1,        0.5        ],  // negative exponent
	[  2,      -2,        0.25       ],
	[  10,     -2,        0.01       ],
	[  4,       0.5,      2          ],  // fractional exponent (root)
	[  2,       0.5,      Math.SQRT2 ],
	[  27,      1 / 3,    3          ],
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // Negative base: integer exponents stay real, but a fractional exponent
    // has no real root and yields NaN.
    for (const [ n, p, result ] of [
	// n      p        Result
	[ -2,     2,       4    ],
	[ -2,     3,      -8    ],
	[ -3,     0,       1    ],
	[ -2,    -2,       0.25 ],
	[ -8,     1 / 3,   NaN  ],
	[ -1,     0.5,     NaN  ],
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // A null or undefined for either argument short-circuits to null before
    // any exponentiation (and so never coerces to NaN).
    for (const [ n, p ] of [
	[ null,      2         ],
	[ 2,         null      ],
	[ null,      null      ],
	[ undefined, 2         ],
	[ 2,         undefined ],
	[ undefined, undefined ],
	[ null,      undefined ],
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), null);
	    });
	}

    // Infinities and the IEEE-754 corner cases of exponentiation: a zero
    // exponent always wins (1), a base of 1 or -1 against an infinite exponent
    // is NaN, and 0^negative is a signed infinity.
    for (const [ n, p, result ] of [
	// n           p           Result
	[ Infinity,    2,          Infinity  ],
	[ Infinity,   -1,          0         ],
	[ Infinity,    0,          1         ],
	[ 2,           Infinity,   Infinity  ],
	[ 2,          -Infinity,   0         ],
	[ 0.5,         Infinity,   0         ],
	[ 0,           Infinity,   0         ],
	[ 0,          -1,          Infinity  ],
	[ 0,          -2,          Infinity  ],
	[ 1,           Infinity,   NaN       ],
	[ -1,          Infinity,   NaN       ],
	[ 1,          -Infinity,   NaN       ],
	[ -1,         -Infinity,   NaN       ],
	[ 0.5,        -Infinity,   Infinity  ],  // |base|<1 with -Infinity exp blows up
	[ -Infinity,   2,          Infinity  ],
	[ -Infinity,   3,         -Infinity  ],
	[ -Infinity,  -1,         -0         ],
	[ -Infinity,  -2,          0         ],  // even negative exp gives +0
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // A base of -0 keeps the sign only for odd positive exponents; negative
    // exponents give a signed infinity (odd -> -Infinity, even -> +Infinity).
    for (const [ n, p, result ] of [
	// n     p     Result
	[ -0,    2,    0          ],  // even exponent -> +0
	[ -0,    3,   -0          ],  // odd exponent keeps the sign
	[ -0,   -1,   -Infinity   ],  // odd negative exponent
	[ -0,   -2,    Infinity   ],  // even negative exponent
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // Overflow saturates to Infinity and a fractional exponent on a negative
    // base has no real value (NaN), regardless of the base's magnitude.
    for (const [ n, p, result ] of [
	// n                 p      Result
	[ 2,                 1024,  Infinity ],  // overflows to Infinity
	[ Number.MAX_VALUE,  2,     Infinity ],
	[ 2,                -1074,  5e-324   ],  // smallest positive subnormal
	[ -0.5,              0.5,   NaN      ],  // negative fractional base -> NaN
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // NaN propagates.
    for (const [ n, p, result ] of [
	// n     p         Result
	[ NaN,   2,        NaN ],
	[ 2,     NaN,      NaN ],
	[ NaN,   NaN,      NaN ],
	[ NaN,   Infinity, NaN ],
	[ NaN,   0,        1   ], // Exception: 0 exponent yields 1.
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}

    // Arguments that pass the null/undefined guard are coerced to numbers.
    for (const [ n, p, result ] of [
	// n      p      Result
	[ '2',    '3',   8   ],  // numeric strings coerce
	[ '4',    0.5,   2   ],
	[ '',     2,     0   ],  // empty string coerces to 0
	[ 2,      '',    1   ],  // ...so this is 2^0
	[ 'foo',  2,     NaN ],  // non-numeric string coerces to NaN
	[ 2,      'foo', NaN ],
	[ true,   3,     1   ],  // true coerces to 1
	[ false,  0,     1   ],  // false coerces to 0, then 0^0 is 1
	[ 2,      true,  2   ],
	[ [],     2,     0   ],  // empty array coerces to 0
	[ [3],    2,     9   ],  // single-element array coerces to its element
	[ 2,      [3],   8   ],
	[ {},     2,     NaN ],  // object coerces to NaN
    ])	{
	test(`cxjs_power(${fmt(n)}, ${fmt(p)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_power(n, p), result);
	    });
	}
    });
