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

// JSON.stringify collapses NaN/Infinity to "null" and omits undefined, which
// would make distinct edge-case rows share a test name; fmt renders those
// values verbatim (and otherwise matches JSON.stringify) so names stay unique.
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

describe('cxjs_round', () =>
    {
    for (const [ args, result ] of [
	// Rounding to the nearest integer (default dec).
	// Exact halves round away from zero in both directions.
	// Args             Result
	[ [2.4],            2      ],
	[ [2.5],            3      ],
	[ [2.6],            3      ],
	[ [3],              3      ],
	[ [0.5],            1      ],
	[ [1.5],            2      ],
	[ [-2.4],          -2      ],
	[ [-2.5],          -3      ],
	[ [-2.6],          -3      ],
	[ [-3],            -3      ],
	[ [-0.5],          -1      ],
	[ [-1.5],          -2      ],
	
	// Rounding to dec decimal places.
	// Args             Result
	[ [3.14159, 2],     3.14   ],
	[ [3.14159, 4],     3.1416 ],
	[ [2.345, 2],       2.35   ],
	
	// Negative dec rounds to tens, hundreds, thousands, etc.
	// Args             Result
	[ [1234, -2],       1200   ],
	[ [1250, -2],       1300   ],
	[ [1251, -2],       1300   ],
	[ [1240, -1],       1240   ],
	[ [12345, -3],      12000  ],
	
	// A non-integer dec is rounded to the nearest integer before use.
	// Args             Result
	[ [12.345, 1.4],    12.3  ],  // dec -> 1
	[ [12.345, 1.5],    12.35 ],  // dec -> 2
	[ [1.5, 0.4],       2     ],  // dec -> 0
	[ [2.5, -0.5],      3     ],  // dec -> 0
	
	// null and undefined yield null (no coercion).
	// Args             Result
	[ [null],           null ],
	[ [undefined],      null ],
	[ [null, 2],        null ],
	
	// Special numeric values pass through.
	// Args             Result
	[ [Infinity],       Infinity  ],
	[ [Infinity, 2],    Infinity  ],
	[ [-Infinity],     -Infinity  ],
	[ [NaN],            NaN       ],
	[ [5, 400],         NaN       ], // dec exceeds scaling factor.
	
	// Sign of a zero: strictly positive inputs give +0, while zero and
	// negative inputs give -0.
	// Args        Result
	[ [0.1],       0  ],
	[ [0.4],       0  ],
	[ [0],        -0  ],
	[ [-0.1],     -0  ],
	[ [-0.4],     -0  ],
	
	// Values that are exact halves in decimal are not always representable in
	// binary, so they might be rounded down rather than up.
	// Args            Result
	[ [1.005, 2],      1    ],  // not 1.01
	[ [1.255, 2],      1.25 ],  // not 1.26
	
	// n is coerced to a number before rounding (matching JS arithmetic rules).
	// Args        Result
	[ ['2.5'],     3   ],
	[ ['foo'],     NaN ],  // non-numeric string coerces to NaN
	[ [''],       -0   ],  // empty string coerces to 0
	[ [true],      1   ],
	[ [false],    -0   ],
	[ [[5]],       5   ],  // single-element array coerces to its element
	[ [{}],        NaN ],  // object coerces to NaN
    ])	{
	test(`cxjs_round(${args.map(fmt).join(', ')}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_round(...args), result);
	    });
	}
    });
