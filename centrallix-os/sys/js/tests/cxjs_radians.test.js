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
// renders those values verbatim (and -0 distinctly from 0) while otherwise
// matching JSON.stringify, so names stay unique.
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
	[ -90,         -Math.PI / 2   ],
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

    // Extreme magnitudes follow the exact source expression: a very large value
    // overflows to Infinity, a very small one stays finite (no underflow to 0).
    for (const input of [ 1e200, Number.MAX_VALUE, 1e-200, -90, Number.MIN_VALUE ])
	{
	test(`cxjs_radians(${fmt(input)}) = (degrees*PI)/180`, () =>
	    {
	    assert.equal(env.cxjs_radians(input), (input * Math.PI) / 180.0);
	    });
	}

    // radians(degrees(x)) = x (within float tolerance).
    for (const x of [ 1, Math.PI / 4, 2 * Math.PI, -1.5 ])
	{
	test(`cxjs_radians(cxjs_degrees(${fmt(x)})) ~= ${fmt(x)}`, () =>
	    {
	    assert.ok(Math.abs(env.cxjs_radians(env.cxjs_degrees(x)) - x) < 1e-9);
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

    // Non-number inputs are coerced to numbers.
    // Those that coerce to NaN yield NaN (null is never used).
    for (const [ input, result ] of [
	// Input        Result
	[ '180',        Math.PI        ],
	[ '-90',       -Math.PI / 2    ],  // negative string coerces to -90
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
