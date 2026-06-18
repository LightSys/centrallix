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
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

describe('cxjs_radians', () =>
    {
    for (const [ input, result ] of [
	// Input        Result
	[ 0,            0             ],
	[ 180,          Math.PI       ],
	[ 90,           Math.PI / 2   ],
	[ 45,           Math.PI / 4   ],
	[ 360,          Math.PI * 2   ],
	[ 270,          Math.PI * 1.5 ],
	[ -180,        -Math.PI       ],
	[ -0,          -0             ],  // sign preserved
	[ Infinity,     Infinity      ],
	[ -Infinity,   -Infinity      ],
	[ NaN,          NaN           ],  // NaN propagates (no coercion).
    ])	{
	test(`cxjs_radians(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_radians(input), result);
	    });
	}

    // Only null and undefined yield null (no coercion).
    for (const input of [ null, undefined ])
	{
	test(`cxjs_radians(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_radians(input), null);
	    });
	}

    // Non-number inputs are coerced to numbers by the arithmetic.
    // Those that coerce to NaN yield NaN (null is never used).
    for (const [ input, result ] of [
	// Input        Result
	[ '180',        Math.PI        ],
	[ '',           0              ],  // empty string coerces to 0
	[ 'foo',        NaN            ],  // non-numeric string coerces to NaN
	[ true,         Math.PI / 180  ],
	[ false,        0              ],
	[ [],           0              ],  // empty array coerces to 0
	[ [180],        Math.PI        ],  // single-element array coerces to its element
	[ [1, 2],       NaN            ],  // multi-element array coerces to NaN
	[ {},           NaN            ],  // object coerces to NaN
    ])	{
	test(`cxjs_radians(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_radians(input), result);
	    });
	}
    });
