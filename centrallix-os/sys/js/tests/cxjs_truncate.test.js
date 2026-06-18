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

describe('cxjs_truncate', () =>
    {
    for (const [ args, result ] of [
	// Truncating to an integer (default dec): values truncate toward
	// zero in both directions.
	// Args             Result
	[ [2.4],            2         ],
	[ [2.5],            2         ],
	[ [2.9],            2         ],
	[ [3],              3         ],
	[ [0.9],            0         ],
	[ [-2.4],          -2         ],
	[ [-2.5],          -2         ],
	[ [-2.9],          -2         ],
	[ [-3],            -3         ],

	// Truncating to dec decimal places (digits past dec are dropped).
	// Args             Result
	[ [3.14159, 2],     3.14      ],
	[ [3.14159, 4],     3.1415    ],  // not rounded up to 3.1416
	[ [-3.14159, 2],   -3.14      ],
	[ [2.349, 2],       2.34      ],  // not rounded up to 2.35

	// Negative dec truncates to tens, hundreds, thousands, etc.
	// Args             Result
	[ [1234, -2],       1200      ],
	[ [1299, -2],       1200      ],
	[ [5678, -3],       5000      ],

	// A non-integer dec is rounded to the nearest integer before use.
	// Args             Result
	[ [12.345, 1.4],    12.3      ],  // dec -> 1
	[ [12.345, 1.5],    12.34     ],  // dec -> 2
	[ [1234.5, -1.6],   1200      ],  // dec -> -2

	// null and undefined yield null (no coercion).
	// Args             Result
	[ [null],           null      ],
	[ [undefined],      null      ],
	[ [null, 2],        null      ],

	// Special numeric values pass through.
	// Args             Result
	[ [Infinity],       Infinity  ],
	[ [Infinity, 2],    Infinity  ],
	[ [-Infinity],     -Infinity  ],
	[ [NaN],            NaN       ],
	[ [5, 400],         NaN       ],  // dec exceeds scaling factor

	// Sign of a zero: a negative input under 1 truncates to -0, while zero
	// and positive inputs give +0.
	// Args             Result
	[ [0],              +0        ],
	[ [0.1],            +0        ],
	[ [-0.1],           -0        ],
	[ [-0.9],           -0        ],
	[ [-0],             -0        ],  // -0 input passes through
	[ [-Number.MIN_VALUE], -0     ],  // tiny negative subnormal -> -0

	// Float traps.
	// Args             Result
	[ [1.005, 2],       1         ],  // stored just under 1.005 -> 1.00
	[ [2.349999, 2],    2.34      ],

	// Precision is lost before truncation when n*factor exceeds 2^53, and a
	// large n*factor can overflow to Infinity..
	// Args                        Result
	[ [123456789012.3456, 2],      123456789012.34  ],
	[ [Number.MAX_VALUE, 2],       Infinity         ],  // n*100 overflows
	[ [Number.MAX_VALUE],          Number.MAX_VALUE ],
	[ [Number.MIN_VALUE],          0                ],  // smallest subnormal -> +0

	// factor = 10**Math.round(dec) underflows to 0 for a large negative dec,
	// so n*0 = 0 then 0/0 = NaN (the large positive dec case is covered above).
	// Args             Result
	[ [5, -400],        NaN       ],  // 10**-400 underflows to 0

	// dec coercion.
	// Args             Result
	[ [1.5, '2'],       1.5       ],  // dec "2" -> 2, no change
	[ [1.555, '2'],     1.55      ],  // truncated, not rounded
	[ [1.5, NaN],       NaN       ],
	[ [1.5, Infinity],  NaN       ],
	[ [1.5, -Infinity], NaN       ],

	// n is coerced to a number before truncating (matching JS arithmetic).
	// Args             Result
	[ ['2.9'],          2         ],
	[ ['foo'],          NaN       ],  // non-numeric string coerces to NaN
	[ [''],             0         ],  // empty string coerces to 0
	[ [true],           1         ],
	[ [false],          0         ],
	[ [[5]],            5         ],  // single-element array coerces to its element
	[ [{}],             NaN       ],  // object coerces to NaN
    ])	{
	test(`cxjs_truncate(${args.map(fmt).join(', ')}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_truncate(...args), result);
	    });
	}
    });
