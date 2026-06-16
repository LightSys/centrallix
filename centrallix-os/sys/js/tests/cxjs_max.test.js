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
	test(`cxjs_max(${input}) = ${result}`, () =>
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
	test(`cxjs_max(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_max(input), result);
	    });
	}
    });
