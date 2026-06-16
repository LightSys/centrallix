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

describe('cxjs_min', () =>
    {
    for (const [ input, result ] of [
	// Input                   Result
	[ [],                      undefined ],
	[ [0, 1],                  0         ],
	[ [0, 1, 2, 3, 4],         0         ],
	[ [3, 1, 2, 4],            1         ],
	[ [9, 10.1, 10.2],         9         ],
	[ [3.4, 3.3, 3.2],         3.2       ],
	[ [-1, 0],                -1         ],
	[ [-1.1, 1],              -1.1       ],
	[ [-Infinity, 0],         -Infinity  ],
	[ [Infinity, 0],           0         ],
	[ [Infinity, Infinity],    Infinity  ],
	[ [-Infinity, -Infinity], -Infinity  ],
	[ [undefined],             undefined ],
	[ [undefined, 0],          0         ],
    ])	{
	test(`cxjs_min(${input}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_min(input), result);
	    });
	}
	
    for (const [ input, result ] of [
	// Input                             Result
	[ {},                                undefined ],
	[ { a: 0, b: 1 },                    0         ],
	[ { c: 0, d: 1, e: 2, f: 3, g: 4 },  0         ],
	[ { h: 3, i: 1, j: 2, k: 4 },        1         ],
	[ { l: 9, m: 10.1, n: 10.2 },        9         ],
	[ { o: 3.4, p: 3.3, q: 3.2 },        3.2       ],
	[ { r: -1, s: 0 },                  -1         ],
	[ { t: -1.1, u: 1 },                -1.1       ],
	[ { v: -Infinity, w: 0 },           -Infinity  ],
	[ { x:  Infinity, y: 0 },            0         ],
	[ { z:  Infinity, A: Infinity },     Infinity  ],
	[ { B: -Infinity, C: -Infinity },   -Infinity  ],
	[ { D: undefined },                  undefined ],
	[ { E: undefined, F: 0 },            0         ],
    ])	{
	test(`cxjs_min(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_min(input), result);
	    });
	}
    });
