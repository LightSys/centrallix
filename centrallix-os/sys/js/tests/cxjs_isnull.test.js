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

// JSON.stringify collapses NaN/Infinity to "null", drops undefined, and
// renders -0 as "0", which would make distinct edge-case rows share a test
// name; fmt renders those verbatim so names stay unique.
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

describe('cxjs_isnull', () =>
    {
    // null/undefined values use the default.
    for (const [ value, default_value, result ] of [
	// Value       Default      Result
	[ null,        5,           5         ],
	[ undefined,   5,           5         ],
	[ null,        'default',   'default' ],
	[ undefined,   'default',   'default' ],
	[ null,        0,           0         ],
	[ null,        '',          ''        ],
	[ null,        false,       false     ],
	[ null,        Infinity,    Infinity  ],
	[ null,        NaN,         NaN       ],
	// The default may itself be null or undefined.
	[ null,        null,        null      ],
	[ null,        undefined,   undefined ],
	[ undefined,   null,        null      ],
	[ undefined,   undefined,   undefined ],
    ])	{
	test(`cxjs_isnull(${fmt(value)}, ${fmt(default_value)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_isnull(value, default_value), result);
	    });
	}

    // Non-null/undefined pass through unchanged. Falsy-but-defined values
    // (0, '', false, NaN) are NOT treated as null.
    for (const [ value, default_value, result ] of [
	// Value       Default      Result
	[ 0,           5,           0         ],
	[ '',          5,           ''        ],
	[ false,       5,           false     ],
	[ NaN,         5,           NaN       ],
	[ 42,          5,           42        ],
	[ -1.1,        5,          -1.1       ],
	[ Infinity,    5,           Infinity  ],
	[ -Infinity,   5,          -Infinity  ],
	[ 'foo',       'bar',       'foo'     ],
	[ true,        false,       true      ],
	// The value is returned even when the default is null/undefined.
	[ 0,           undefined,   0         ],
	[ '',          null,        ''        ],
    ])	{
	test(`cxjs_isnull(${fmt(value)}, ${fmt(default_value)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_isnull(value, default_value), result);
	    });
	}

    // Objects and arrays are treated as not null.
    for (const value of [
	{},
	{ a: 1 },
	[],
	[1, 2, 3],
    ])	{
	test(`cxjs_isnull(${fmt(value)}, 'default') returns the value itself`, () =>
	    {
	    assert.equal(env.cxjs_isnull(value, 'default'), value);
	    });
	}

    // Only the first argument is consulted; missing the default yields
    // undefined when the value is null.
    test('cxjs_isnull(null) = undefined (default omitted)', () =>
	{
	assert.equal(env.cxjs_isnull(null), undefined);
	});
    test('cxjs_isnull(0) = 0 (default omitted)', () =>
	{
	assert.equal(env.cxjs_isnull(0), 0);
	});

    // Signed zero is preserved in both positions.
    test('cxjs_isnull(-0, 5) = -0 (value not null, passes through)', () =>
	{
	assert.equal(env.cxjs_isnull(-0, 5), -0);
	});
    test('cxjs_isnull(null, -0) = -0 (default returned verbatim)', () =>
	{
	assert.equal(env.cxjs_isnull(null, -0), -0);
	});
    });
