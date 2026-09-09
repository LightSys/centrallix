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
// renders those values verbatim, distinguishes -0 from 0, and otherwise matches
// JSON.stringify, so names stay unique.
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

describe('cxjs_max', () =>
    {
    for (const [ input, result ] of [
	// Input                   Result
	[ [],                      undefined ],
	[ [0, 1],                  1         ],
	[ [0, 1, 2, 3, 4],         4         ],
	[ [3, 4, 2, 1],            4         ],
	[ [9, 10.1, 10.2],         10.2      ],
	[ [3.4, 3.3, 3.2],         3.4       ],
	[ [-1, 0],                 0         ],
	[ [-1.1, -1],             -1         ],
	[ [-Infinity, 0],          0         ],
	[ [Infinity, 0],           Infinity  ],
	[ [Infinity, Infinity],    Infinity  ],
	[ [-Infinity, -Infinity], -Infinity  ],
	[ [undefined],             undefined ],
	[ [undefined, 0],          0         ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}
	
    for (const [ input, result ] of [
	// Input                             Result
	[ {},                                undefined ],
	[ { a: 0, b: 1 },                    1         ],
	[ { c: 0, d: 1, e: 2, f: 3, g: 4 },  4         ],
	[ { h: 3, i: 4, j: 2, k: 1 },        4         ],
	[ { l: 9, m: 10.1, n: 10.2 },        10.2      ],
	[ { o: 3.4, p: 3.3, q: 3.2 },        3.4       ],
	[ { r: -1, s: 0 },                   0         ],
	[ { t: -1.1, u: -1 },               -1         ],
	[ { v: -Infinity, w: 0 },            0         ],
	[ { x:  Infinity, y: 0 },            Infinity  ],
	[ { z:  Infinity, A: Infinity },     Infinity  ],
	[ { B: -Infinity, C: -Infinity },   -Infinity  ],
	[ { D: undefined },                  undefined ],
	[ { E: undefined, F: 0 },            0         ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Scalar are returned as is.
    for (const [ input, result ] of [
	// Input        Result
	[ 5,            5         ],
	[ -3,          -3         ],
	[ 0,            0         ],
	[ Infinity,     Infinity  ],
	[ NaN,          NaN       ],
	[ 'foo',        'foo'     ],
	[ true,         true      ],
	[ null,         null      ],  // typeof null is "object", but v !== null is false
	[ undefined,    undefined ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // null and NaN within a collection are handled specially.
    for (const [ input, result ] of [
	// Input               Result
	[ [5],                 5         ],  // single element
	[ [NaN, 5],            5         ],  // leading NaN replaced by a real value
	[ [5, NaN],            5         ],  // trailing NaN never beats a real value
	[ [NaN],               NaN       ],  // all NaN: NaN survives
	[ [NaN, NaN],          NaN       ],
	[ [null, 5],           5         ],  // 5 > null (0), so 5 is the max
	[ [5, null],           5         ],
	[ [0, undefined],      0         ],  // undefined after a value is ignored
	[ { a: NaN, b: 5 },    5         ],
	[ { a: 5, b: NaN },    5         ],
	[ { a: NaN },          NaN       ],
	[ { a: null, b: 5 },   5         ],
	[ { a: 5, b: null },   5         ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Non-numeric strings.
    for (const [ input, result ] of [
	// Input                Result
	[ ['b', 'a', 'c'],      'c'      ],  // last element wins
	[ ['apple', 'banana'],  'banana' ],
	[ ['10', '9', '100'],   '9'      ],  // lexical compare: '100' < '10' < '9'
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Lists with NaNs.
    for (const [ input, result ] of [
	// Input                 Result
	[ [5, NaN, 3],           5   ],  // 5 retained, NaN never beats it, then 3<5
	[ [NaN, NaN, 5],         5   ],  // NaN survives until a real value replaces it
	[ [1, NaN, 2, NaN, 3],   3   ],  // running max walks 1 -> 2 -> 3 across the NaNs
	[ [3, NaN, 5, NaN, 1],   5   ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // null comparisons to 1 and -1.
    for (const [ input, result ] of [
	// Input          Result
	[ [1, null],      1     ],  // 1 > null(0), so 1 wins
	[ [null, 1],      1     ],
	[ [-1, null],     null  ],  // -1 not > null(0), so null wins and is preserved
	[ [null, -1],     null  ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Misc edge cases.
    for (const [ input, result ] of [
	// Input                       Result
	[ [Infinity, -Infinity],       Infinity  ],
	[ [-Infinity, Infinity],       Infinity  ],
	[ [-0, 0],                    -0         ],  // -0 < 0 is false, so -0 is kept
	[ [0, -0],                     0         ],  // 0 < -0 is false, so 0 is kept
	[ [-0],                       -0         ],
	[ [true, false],               true      ],  // true(1) > false(0)
	[ [false, true],               true      ],
	[ [2, true],                   2         ],  // 2 > true(1), returned as number
	[ [0, false],                  0         ],  // false(0) not > 0, so 0 kept
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Numbers mixed with numeric strings.
    for (const [ input, result ] of [
	// Input              Result
	[ [2, '10'],          '10' ],  // numeric compare: 10 > 2, kept as a string
	[ ['10', 2],          '10' ],
	[ [5, '3', 4],        5    ],  // 5 is numerically greatest
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    // Sparse arrays.
    for (const [ input, result ] of [
	// Input            Result
	[ [1, , 3],         3 ],  // 3 > 1, the hole is skipped
	[ [, , 5],          5 ],
    ])	{
	test(`cxjs_max(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}

    test('cxjs_max(array with extra non-index prop) = 3', () =>
	{
	const a = [1, 2, 3];
	a.foo = 99;             // ignored: numeric-index loop only
	assert.equal(env.cxjs_max(a), 3);
	});

    // Value on the prototype participates in the comparison.
    test('cxjs_max(object with inherited enumerable prop) = 3', () =>
	{
	function Proto() { this.a = 3; }
	Proto.prototype.b = 1;
	assert.equal(env.cxjs_max(new Proto()), 3);
	});
    });
