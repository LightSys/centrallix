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

describe('cxjs_condition', () =>
    {
    for (const [ c, vtrue, vfalse, result ] of [
	// null/undefined short-circuit to null before vtrue/vfalse.
	// Condition           vtrue  vfalse  Result
	[ null,                'T',       'F',    null      ],
	[ undefined,           'T',       'F',    null      ],

	// Truthy conditions yield vtrue.
	// Condition           vtrue  vfalse  Result
	[ true,                'T',       'F',    'T'       ],
	[ 1,                   'T',       'F',    'T'       ],
	[ -1,                  'T',       'F',    'T'       ],
	[ 3.14,                'T',       'F',    'T'       ],
	[ Infinity,            'T',       'F',    'T'       ],
	[ '0',                 'T',       'F',    'T'       ],  // nonempty string
	[ 'false',             'T',       'F',    'T'       ],
	[ ' ',                 'T',       'F',    'T'       ],
	[ [],                  'T',       'F',    'T'       ],  // empty array is truthy
	[ {},                  'T',       'F',    'T'       ],
	[ -Infinity,           'T',       'F',    'T'       ],  // any nonzero number is truthy
	[ [0],                 'T',       'F',    'T'       ],  // object identity, contents irrelevant
	[ 'NaN',               'T',       'F',    'T'       ],  // nonempty string, not the NaN number

	// Falsy conditions yield vfalse.
	// Condition           vtrue  vfalse  Result
	[ false,               'T',       'F',    'F'       ],
	[ 0,                   'T',       'F',    'F'       ],
	[ -0,                  'T',       'F',    'F'       ],
	[ NaN,                 'T',       'F',    'F'       ],
	[ '',                  'T',       'F',    'F'       ],

	// vtrue/vfalse pass through verbatim, regardless of type.
	// Condition           vtrue  vfalse  Result
	[ true,                42,        99,     42        ],
	[ false,               42,        99,     99        ],
	[ true,                null,      'F',    null      ],  // truthy c can still return null
	[ true,                undefined, 'F',    undefined ],
	[ false,               'T', undefined,    undefined ],
	[ true,                0,         1,      0         ],  // falsy values pass through too
	[ false,               1,         '',     ''        ],

	// Both branches omitted: a defined-but-truthy c yields undefined vtrue,
	// a falsy c yields undefined vfalse (null c is handled separately above).
	// Condition           vtrue  vfalse  Result
	[ true,             undefined, undefined, undefined ],
	[ false,            undefined, undefined, undefined ],
    ])	{
	test(`cxjs_condition(${fmt(c)}, ${fmt(vtrue)}, ${fmt(vfalse)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_condition(c, vtrue, vfalse), result);
	    });
	}

    // Signed-zero (JSON.stringify(-0) is "0", so name explicitly). -0 is a
    // number == null is false, so it reaches the ternary and is falsy -> vfalse;
    // and a -0 passed as a branch value comes back unchanged (verbatim).
    test('cxjs_condition(-0, "T", "F") = "F" (-0 is falsy)', () =>
	{
	assert.equal(env.cxjs_condition(-0, 'T', 'F'), 'F');
	});
    test('cxjs_condition(false, 1, -0) = -0 (branch value verbatim)', () =>
	{
	assert.equal(env.cxjs_condition(false, 1, -0), -0);
	});
    });
