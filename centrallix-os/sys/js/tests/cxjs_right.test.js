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

describe('cxjs_right', () =>
    {
    for (const [ s, l, result ] of [
	// String          Length     Result
	[ 'hello',         2,         'lo'     ],
	[ 'hello',         1,         'o'      ],
	[ 'hello',         5,         'hello'  ], // l equals length: whole string
	[ 'hello',         10,        'hello'  ], // l exceeds length: whole string
	[ 'hello',         0,         ''       ], // zero length: empty
	[ 'hello',        -2,         ''       ], // negative length: empty
	[ 'hello',         2.5,       'llo'    ], // fractional length truncated to 2
	[ 'hello',         NaN,       'hello'  ], // NaN != null, so substr(NaN) -> substr(0)
	[ '',              3,         ''       ], // empty string stays empty
	[ '',              0,         ''       ],
	[ 'x',             1,         'x'      ], // single char
	[ '  ab ',         2,         'b '     ], // trailing whitespace preserved
	[ '  ab ',         4,         ' ab '   ], // preceding whitespace preserved
	[ 'abc',           null,      null     ], // null length
	[ 'abc',           undefined, null     ], // undefined length
	[ null,            2,         null     ], // null string
	[ undefined,       2,         null     ], // undefined string
	[ null,            null,      null     ],

	// l is coerced to a number by the subtraction s.length - l.
	[ 'hello',         '2',       'lo'     ], // numeric string length
	[ 'hello',         '2.5',     'llo'    ], // numeric string truncated
	[ 'hello',         'abc',     'hello'  ], // non-numeric string -> NaN -> substr(NaN)=substr(0)
	[ 'hello',         '',        ''       ], // ""    -> Number 0 -> substr(5) -> ""
	[ 'hello',         true,      'o'      ], // true  -> 1
	[ 'hello',         false,     ''       ], // false -> 0 -> substr(5) -> ""
	[ 'hello',         2.9,       'llo'    ], // fractional length: substr start truncated to 2
	[ 'hello',         Infinity,  'hello'  ], // length-Inf = -Inf -> substr(-Inf)=substr(0)
	[ 'hello',        -Infinity,  ''       ], // length+Inf = Inf -> substr past end -> ""
	[ 'hello',         [2],       'lo'     ], // [2]  -> Number 2
	[ 'hello',         [],        ''       ], // []   -> Number 0 -> substr(5) -> ""
	[ 'hello',         {},        'hello'  ], // {}   -> NaN -> substr(0) -> whole string

	// s is a string and is sliced by UTF-16 code units, not grapheme clusters.
	[ 'a😀',           1,         '\uDE00' ], // splits the surrogate pair: low half only
	[ 'a😀',           2,         '😀'     ], // both code units of the pair
	[ 'café',          1,         'é'      ], // composed code point (U+00E9)
	[ 'a\nb',          2,         '\nb'    ], // embedded newline preserved
    ])	{
	test(`cxjs_right(${fmt(s)}, ${fmt(l)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_right(s, l), result);
	    });
	}

    // A very long string is sliced correctly (substr handles the full length).
    test("cxjs_right(<1000 x's>+'abc', 3) = 'abc'", () =>
	{
	assert.equal(env.cxjs_right('x'.repeat(1000) + 'abc', 3), 'abc');
	});

    // Unlike cxjs_substring(), cxjs_right() never coerces s to a string.
    for (const s of [ 12345, true, 0, NaN, [ 1, 2, 3 ], {} ])
	{
	test(`cxjs_right(${fmt(s)}, 2) throws (no String coercion)`, () =>
	    {
	    // Note: This error originates in the vm sandbox, so it is an instance
	    // of the sandbox's TypeError, not this realm's, preventing instanceof checks.
	    assert.throws(() => env.cxjs_right(s, 2), (err) => err && err.name === 'TypeError');
	    });
	}
    });
