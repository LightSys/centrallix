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

describe('cxjs_sum', () =>
    {
    for (const [ input, result ] of [
	// Input                       Result
	[ [],                          null      ],
	[ [0, 1],                      1         ],
	[ [0, 1, 2, 3, 4],             10        ],
	[ [3, 1, 2, 4],                10        ],
	[ [0.5, 0.25],                 0.75      ],
	[ [1.5, 2.5],                  4         ],
	[ [9, 10, 11],                 30        ],
	[ [-1, 0],                    -1         ],
	[ [-1.5, 1],                  -0.5       ],
	[ [-1, -2, -3],               -6         ],
	[ [-Infinity, 0],             -Infinity  ],
	[ [Infinity, 0],               Infinity  ],
	[ [Infinity, Infinity],        Infinity  ],
	[ [-Infinity, -Infinity],     -Infinity  ],
	[ [Infinity, -Infinity],       NaN       ],
	[ [undefined],                 null      ],
	[ [undefined, 0],              0         ],
	[ [null],                      null      ],
	[ [null, 5],                   5         ],
	[ [undefined, null, 3],        3         ],
	[ [NaN],                       null      ],
	[ [NaN, 2],                    2         ],
	[ [1, 2, undefined, 3],        6         ],
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    for (const [ input, result ] of [
	// Input                                  Result
	[ {},                                     null      ],
	[ { a: 0, b: 1 },                         1         ],
	[ { c: 0, d: 1, e: 2, f: 3, g: 4 },       10        ],
	[ { h: 3, i: 1, j: 2, k: 4 },             10        ],
	[ { l: 0.5, m: 0.25 },                    0.75      ],
	[ { n: 1.5, o: 2.5 },                     4         ],
	[ { p: 9, q: 10, r: 11 },                 30        ],
	[ { s: -1, t: 0 },                       -1         ],
	[ { u: -1.5, v: 1 },                     -0.5       ],
	[ { w: -1, x: -2, y: -3 },               -6         ],
	[ { z: -Infinity, A: 0 },                -Infinity  ],
	[ { B:  Infinity, C: 0 },                 Infinity  ],
	[ { D:  Infinity, E: Infinity },          Infinity  ],
	[ { F: -Infinity, G: -Infinity },        -Infinity  ],
	[ { H:  Infinity, I: -Infinity },         NaN       ],
	[ { J: undefined },                       null      ],
	[ { K: undefined, L: 0 },                 0         ],
	[ { M: null },                            null      ],
	[ { N: null, O: 5 },                      5         ],
	[ { P: undefined, Q: null, R: 3 },        3         ],
	[ { S: NaN },                             null      ],
	[ { T: NaN, U: 2 },                       2         ],
	[ { V: 1, W: 2, X: undefined, Y: 3 },     6         ],
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // Scalar (non-array, non-object) inputs hit.
    for (const [ input, result ] of [
	// Input        Result
	[ 5,            5         ],
	[ 0,            0         ],
	[ -3.5,        -3.5       ],
	[ Infinity,     Infinity  ],
	[ 'abc',        'abc'     ],  // returned as-is, not parsed
	[ true,         true      ],  // boolean pass through
	[ false,        false     ],
	[ NaN,          NaN       ],  // scalar NaN is kept (cnt is 1, not 0)
	[ null,         null      ],  // typeof null is "object", but v !== null is false
	[ undefined,    undefined ],
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // String and boolean coercion.
    for (const [ input, result ] of [
	// Input               Result
	[ ['5', 6],            '056'   ],  // 0 + '5' -> '05', then '05' + 6 -> '056'
	[ [1, '2'],            '12'    ],  // 0 + 1 -> 1, then 1 + '2' -> '12'
	[ [''],                '0'     ],  // '' is numeric (0); 0 + '' -> '0'
	[ ['', 5],             '05'    ],
	[ [true],              1       ],  // true coerces to 1
	[ [true, false],       1       ],  // false coerces to 0
	[ [1, true],           2       ],
	[ { a: '5', b: 6 },    '056'   ],  // same concatenation in object form
	[ { a: true },         1       ],
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // Order-dependent concatenation with type coercions.
    for (const [ input, result ] of [
	// Input              Result
	[ [1, '2', 3],        '123' ],  // 0+1->1, 1+'2'->'12', '12'+3->'123'
	[ ['a', 1, 2],        3     ],  // 'a' skipped, then 0+1+2
	[ [1, 2, 'a'],        3     ],  // 'a' skipped at the end
	[ ['x', 'y'],         null  ],  // all values are non-numeric
	[ ['x', true, 'y'],   1     ],  // wrapped coercion
	[ [true, true, 1],    3     ],  // booleans coerce to 1 and add numerically
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // All-null and all-NaN collections count as nothing.
    for (const [ input, result ] of [
	// Input                       Result
	[ [null, null],                null ],
	[ [NaN, NaN],                  null ],
	[ { a: null, b: null },        null ],
	[ { a: NaN, b: NaN },          null ],
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // Sign of zero and floating-point accumulation.
    for (const [ input, result ] of [
	// Input              Result
	[ [-0],               0                   ],  // 0 + -0 -> +0
	[ [-0, -0],           0                   ],
	[ [-0, 0],            0                   ],
	[ [0.1, 0.2],         0.30000000000000004 ],  // binary float rounding
	[ [1e308, 1e308],     Infinity            ],  // overflow to Infinity
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // Nested array behaviors.
    for (const [ input, result ] of [
	// Input              Result
	[ [[1], 2],           '012' ],  // 0+[1]->'01', '01'+2->'012'
	[ [[1], [2]],         '012' ],  // 0+[1]->'01', '01'+[2]->'012'
	[ [[], 5],            '05'  ],  // [] coerces to 0 but 0+[]->'0', '0'+5->'05'
	[ [[1, 2], 3],        3     ],  // [1,2] is NaN -> skipped, then 0+3
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    // Sparse arrays.
    for (const [ input, result ] of [
	// Input            Result
	[ [1, , 3],         4 ],  // hole between two values
	[ [, , 5],          5 ],  // two holes, one value
    ])	{
	test(`cxjs_sum(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}

    test('cxjs_sum(array with extra non-index prop) = 6', () =>
	{
	const a = [1, 2, 3];
	a.foo = 99;             // ignored: numeric-index loop only
	assert.equal(env.cxjs_sum(a), 6);
	});

    // Value on the prototype is summed alongside other properties.
    test('cxjs_sum(object with inherited enumerable prop) = 4', () =>
	{
	function Proto() { this.a = 3; }
	Proto.prototype.b = 1;
	assert.equal(env.cxjs_sum(new Proto()), 4);
	});
    });
