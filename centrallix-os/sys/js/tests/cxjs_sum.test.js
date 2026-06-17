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
	test(`cxjs_sum(${JSON.stringify(input)}) = ${result}`, () =>
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
	test(`cxjs_sum(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_sum(input), result);
	    });
	}
    });
