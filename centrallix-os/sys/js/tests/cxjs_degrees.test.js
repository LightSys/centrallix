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

describe('cxjs_degrees', () =>
    {
    // Radians to degrees. Some multiples of PI land exactly; others (e.g. PI/6)
    // carry floating-point error.
    for (const [ input, result ] of [
	// Input           Result
	[ 0,               0                   ],
	[ -0,             -0                   ],  // sign of zero is preserved
	[ Math.PI,         180                 ],
	[ -Math.PI,       -180                 ],
	[ Math.PI / 2,     90                  ],
	[ Math.PI / 4,     45                  ],
	[ 2 * Math.PI,     360                 ],
	[ 1,               180 / Math.PI       ],  // one radian
	[ Math.PI / 6,     29.999999999999996  ],  // not exactly 30
	[ 3 * Math.PI / 2, 270                 ],
	[ Infinity,        Infinity            ],
	[ -Infinity,      -Infinity            ],
	[ NaN,             NaN                 ],
    ])	{
	test(`cxjs_degrees(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_degrees(input), result);
	    });
	}

    // Extreme magnitudes follow the exact source expression: a very large value
    // overflows to Infinity, a very small one stays finite (no underflow to 0).
    for (const input of [ 1e200, Number.MAX_VALUE, 1e-200, -1, Number.MIN_VALUE ])
	{
	test(`cxjs_degrees(${fmt(input)}) = (radians*180)/PI`, () =>
	    {
	    assert.equal(env.cxjs_degrees(input), (input * 180.0) / Math.PI);
	    });
	}

    // degrees(radians(x)) = x (within float tolerance).
    for (const x of [ 1, 45, 123, 360, -90 ])
	{
	test(`cxjs_degrees(cxjs_radians(${fmt(x)})) ~= ${fmt(x)}`, () =>
	    {
	    assert.ok(Math.abs(env.cxjs_degrees(env.cxjs_radians(x)) - x) < 1e-9);
	    });
	}

    // null and undefined yield null (no coercion).
    for (const input of [ null, undefined ])
	{
	test(`cxjs_degrees(${fmt(input)}) = null`, () =>
	    {
	    assert.equal(env.cxjs_degrees(input), null);
	    });
	}

    // Non-number inputs are coerced to numbers.
    // Those that coerce to NaN yield NaN (null is never used).
    for (const [ input, result ] of [
	// Input            Result
	[ '1',              180 / Math.PI ],
	[ '-1',            -180 / Math.PI ],  // negative string coerces to -1
	[ '',               0             ],  // empty string coerces to 0
	[ 'foo',            NaN           ],  // non-numeric string coerces to NaN
	[ true,             180 / Math.PI ],
	[ false,            0             ],
	[ [],               0             ],  // empty array coerces to 0
	[ [Math.PI],        180           ],  // single-element array coerces to its element
	[ [1, 2],           NaN           ],  // multi-element array coerces to NaN
	[ {},               NaN           ],  // object coerces to NaN
    ])	{
	test(`cxjs_degrees(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_degrees(input), result);
	    });
	}
    });
