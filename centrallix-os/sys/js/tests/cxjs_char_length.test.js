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

// cxjs_char_length() returns null for null/undefined, otherwise the
// String()-coerced length in UTF-16 code units (so surrogate pairs
// count as 2).

// JSON.stringify collapses NaN/Infinity to "null", omits undefined, and renders
// -0 as "0", which would make distinct edge-case rows share a test name; fmt
// renders those values verbatim, distinguishes -0 from 0, and otherwise matches
// JSON.stringify, so names stay unique.
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

describe('cxjs_char_length', () =>
    {
    for (const [ input, result ] of [
	// Input             Result
	[ null,              null ],   // null short-circuits to null
	[ undefined,         null ],   // == null also catches undefined
	[ '',                0    ],   // empty string is 0, not null
	[ 'a',               1    ],
	[ 'hello',           5    ],
	[ ' ',               1    ],   // spaces count
	[ '  ',              2    ],
	[ '\t\n ',           3    ],   // whitespace chars count
	[ 'a"b\\c',          5    ],   // quotes/backslashes are literal chars
	[ '😀',              2    ],   // surrogate pair = 2 code units
	[ 'café',            4    ],   // precomposed accent = 1 code unit
	[ 0,                 1    ],   // 0 != null, coerces to "0"
	[ 123,               3    ],
	[ -12,               3    ],   // sign counts
	[ 1.5,               3    ],   // decimal point counts
	[ false,             5    ],   // coerced: "false"
	[ true,              4    ],   // coerced: "true"
	[ NaN,               3    ],   // coerced: "NaN"
	[ Infinity,          8    ],   // coerced: "Infinity"
	[ -Infinity,         9    ],   // coerced: "-Infinity" (sign counts)
	[ [],                0    ],   // coerced: ""
	[ [1, 2],            3    ],   // coerced: "1,2"
	[ [1, [2, 3]],       5    ],   // nested array flattens: "1,2,3"
	[ {},                15   ],   // coerced: "[object Object]"
	// Additional edge cases.
	[ ' a ',             3    ],   // surrounding whitespace counts
	[ -0,                1    ],   // negative zero coerces to "0"
	[ 1e21,              5    ],   // large float uses exponent form "1e+21"
	[ '😀😀',             4    ],   // two surrogate pairs = 4 code units
	[ ['a'],             1    ],   // single-element array coerces to "a"
	[ ['a', 'b'],        3    ],   // coerced: "a,b"
	[ [null],            0    ],   // null element renders as "" -> length 0
	[ [undefined],       0    ],   // undefined element renders as "" -> length 0
	[ [null, null],      1    ],   // coerced: "," (one separator) -> length 1
    ])	{
	test(`cxjs_char_length(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_char_length(input), result);
	    });
	}
    });
