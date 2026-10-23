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

// charindex returns the 1-based position of needle in haystack, 0 when
// absent, and null when either argument is strictly null.
describe('cxjs_charindex', () =>
    {
    for (const [ needle, haystack, result ] of [
	// Needle          Haystack      Result
	[ 'h',             'hello',      1    ], // at start
	[ 'lo',            'hello',      4    ], // multi-char, mid/end
	[ 'o',             'hello',      5    ], // at end
	[ 'l',             'hello',      3    ], // first of several matches
	[ 'hello',         'hello',      1    ], // needle equals haystack
	[ 'z',             'hello',      0    ], // absent
	[ 'hellox',        'hello',      0    ], // needle longer than haystack
	[ 'H',             'hello',      0    ], // case-sensitive: no match
	[ '',              'hello',      1    ], // empty needle matches at start
	[ '',              '',           1    ], // empty needle, empty haystack
	[ 'a',             '',           0    ], // empty haystack, non-empty needle
	[ ' ',             'a b',        2    ], // whitespace needle
	[ '$',             'a$b',        2    ], // special character
	[ '😀',            'a😀b',       2    ], // surrogate pair needle
	[ '2',             123,          2    ], // numeric haystack coerced to string
	[ 2,               '123',        2    ], // numeric needle coerced to string
	[ 2,               123,          2    ], // multiple coercions
	[ null,            'hello',      null ], // null needle
	[ 'lo',            null,         null ], // null haystack
	[ null,            null,         null ], // both null

	// undefined is not strictly null, so it is coerced, not short-circuited.
	// Needle          Haystack      Result
	[ undefined,       'hello',      0    ], // searches for 'undefined': absent
	[ 'u',             undefined,    1    ], // haystack becomes 'undefined'
	[ 'x',             undefined,    0    ], // 'x' absent from 'undefined'
	[ undefined,       undefined,    1    ], // both -> 'undefined'; found at start

	// A null in EITHER position short-circuits first, even when the other
	// argument is undefined (which would otherwise be coerced).
	// Needle          Haystack      Result
	[ undefined,       null,         null ], // null haystack wins
	[ null,            undefined,    null ], // null needle wins

	// Non-string needle/haystack coercion.
	// Needle          Haystack      Result
	[ true,            'xtrueb',     2    ], // boolean needle coerced to 'true'
	[ 'b',             true,         0    ], // boolean haystack coerced to 'true'; 'b' absent
	[ false,           'xfalseb',    2    ], // boolean needle coerced to 'false'
	[ 0,               'a0b',        2    ], // numeric-zero needle coerced to '0'
	[ NaN,             'aNaNb',      2    ], // NaN needle coerced to 'NaN'
	[ 'N',             NaN,          1    ], // NaN haystack coerced to 'NaN'
	[ Infinity,        'aInfinityb', 2    ], // Infinity needle coerced to 'Infinity'

	// Array needle/haystack coerce to comma-joined string.
	// Needle          Haystack      Result
	[ [],              'a',          1    ], // [] needle -> '' matches at start
	[ [ 'a' ],         'bab',        2    ], // single-element array -> 'a'
	[ [ 'a', 'b' ],    'a,bc',       1    ], // multi-element array -> 'a,b' (with comma)
	[ 'o',             [ 'hello' ],  5    ], // single-element array haystack -> 'hello'

	// Plain object needle/haystack coerce to '[object Object]'.
	// Needle          Haystack      Result
	[ 'c',             {},           6    ], // 'c' of '[obje[c]t Object]'
	[ 'Object',        {},           9    ], // 'Object' substring of '[object Object]'

	// Surrogate-pair (UTF-16) indexing: positions count code units, and a
	// lone surrogate half can match inside an emoji.
	// Needle          Haystack      Result
	[ '😀',            '😀x',        1    ], // emoji needle found at start
	[ 'x',             '😀x',        3    ], // 'x' after a 2-code-unit emoji
	[ '\uDE00',        '😀',         2    ], // trailing surrogate half matches at unit 2
    ])	{
	test(`cxjs_charindex(${JSON.stringify(needle)}, ${JSON.stringify(haystack)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_charindex(needle, haystack), result);
	    });
	}
    });
