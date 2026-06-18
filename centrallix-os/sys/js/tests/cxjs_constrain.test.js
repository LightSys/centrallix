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
// values verbatim (and distinguishes -0 from +0) so names stay unique.
function fmt(v)
    {
    if (Object.is(v, -0))
	return '-0';
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

describe('cxjs_constrain', () =>
    {
    for (const [ n, min, max, result ] of [
	// Both bounds: clamp into [min, max], and bounds are inclusive.
	// n          min        max        Result
	[ 5,          0,         10,        5         ],  // within range
	[ 0,          0,         10,        0         ],  // lower bound
	[ 10,         0,         10,        10        ],  // upper bound
	[ -5,         0,         10,        0         ],  // below -> min
	[ 15,         0,         10,        10        ],  // above -> max
	[ 0.5,        0,         10,        0.5       ],  // fractional, within
	[ 5,          5,         5,         5         ],  // min == max

	// A 0 bound is a real bound, not skipped like null/undefined.
	[ -1,         0,         10,        0         ],

	// Negative ranges and fractional bounds.
	[ -5,        -10,       -1,        -5         ],  // within
	[ -15,       -10,       -1,        -10        ],  // below -> min
	[ 0,         -10,       -1,        -1         ],  // above -> max
	[ 2.5,        1.5,       3.5,       2.5       ],  // within

	// null/undefined min leaves the result unbounded below.
	[ -100,       null,      10,        -100      ],
	[ 5,          null,      10,        5         ],
	[ -100,       undefined, 10,        -100      ],

	// null/undefined max leaves the result unbounded above.
	[ 100,        0,         null,      100       ],
	[ 5,          0,         null,      5         ],
	[ 100,        0,         undefined, 100       ],

	// Both bounds absent: n is unchanged.
	[ 5,          null,      null,      5         ],
	[ -100,       null,      null,      -100      ],
	[ 5,          undefined, undefined, 5         ],

	// null/undefined n always yields null, regardless of bounds.
	[ null,       0,         10,        null      ],
	[ undefined,  0,         10,        null      ],
	[ null,       null,      null,      null      ],

	// A NaN bounds are ignored, so it is effectively ignored;
	// the opposite bound (if any) still applies.
	[ 5,          NaN,       10,        5         ],
	[ 15,         NaN,       10,        10        ],  // max still clamps
	[ 5,          0,         NaN,       5         ],
	[ -5,         0,         NaN,       0         ],  // min still clamps
	[ 5,          NaN,       NaN,       5         ],  // both ignored

	// NaN propagates.
	[ NaN,        0,         10,        NaN       ],

	// Infinity clamps like any other value; an Infinite n survives an
	// absent bound on its side.
	[ Infinity,   0,         10,        10        ],
	[ -Infinity,  0,         10,        0         ],
	[ Infinity,   0,         null,      Infinity  ],
	[ -Infinity,  null,      10,        -Infinity ],
	[ 5,         -Infinity,  Infinity,  5         ],

	// Inverted bounds (min > max): min is checked first, so n < min returns
	// min; otherwise n > max returns max.
	[ 5,          10,        0,         10        ],
	[ 100,        10,        0,         0         ],
	[ -5,         10,        0,         10        ],

	// Inverted bounds with n equal to a bound: equality is not "<" or ">",
	// so n == min still falls through to the max check (and vice versa).
	[ 10,         10,        0,         0         ],  // n == min, then 10 > 0 -> max
	[ 0,          10,        0,         10        ],  // n == max, but 0 < 10 -> min

	// Sign of zero: comparisons treat -0 and +0 as equal, so neither bound
	// fires and the original (signed) n passes through unchanged.
	[ -0,         0,         10,        -0        ],  // -0 < 0 false, -0 > 10 false
	[ -0,         0,         0,         -0        ],  // both bounds == -0 numerically
	[ 0,          -0,        10,        0         ],
	// A signed-zero bound is returned verbatim when n falls outside it.
	[ -5,         -0,        10,        -0        ],  // -5 < -0 -> returns min -0

	// Bounds are returned without coercion, so a numeric-string bound that
	// clamps is returned as the original string (compared numerically).
	[ 3,          '5',       10,        '5'       ],  // 3 < "5" -> returns "5"
	[ 7,          '5',       10,        7         ],  // 7 < "5" false -> n
	[ 15,         0,         '10',      '10'      ],  // 15 > "10" -> returns "10"
	[ 5,          0,         '10',      5         ],

	// Infinite bounds and n: an infinity within infinite bounds is unclamped.
	[ Infinity,  -Infinity,  Infinity,  Infinity  ],
	[ -Infinity, -Infinity,  Infinity,  -Infinity ],
	[ Infinity,   0,         Infinity,  Infinity  ],

	// Both bounds NaN with NaN n: NaN bounds never fire and NaN passes through.
	[ NaN,        NaN,       NaN,       NaN       ],

	// Extreme finite magnitudes clamp like any other value.
	[ Number.MAX_VALUE, 0,   1,         1         ],
	[ 1e308,      0,         1e308,     1e308     ],
    ])	{
	test(`cxjs_constrain(${fmt(n)}, ${fmt(min)}, ${fmt(max)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_constrain(n, min, max), result);
	    });
	}
    });
