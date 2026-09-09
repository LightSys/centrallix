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

describe('cxjs_ltrim', () =>
    {
    for (const [ input, result ] of [
	// Input           Result
	[ '  hello',       'hello'            ], // leading spaces stripped
	[ 'hello',         'hello'            ], // no leading spaces
	[ 'hello  ',       'hello  '          ], // trailing spaces preserved
	[ '  a b  ',       'a b  '            ], // interior/trailing spaces preserved
	[ ' ',             ''                 ], // single space
	[ '   ',           ''                 ], // all spaces collapse to empty
	[ '',              ''                 ], // empty stays empty
	[ ' \thello',      '\thello'          ], // leading space stripped, tab kept
	[ '\t hello',      '\t hello'         ], // leading tab is not a space: unchanged
	[ '\nhi',          '\nhi'             ], // newline is not a space: unchanged
	[ 42,              '42'               ], // non-string is coerced
	[ 0,               '0'                ], // falsy but not null: coerced, not dropped
	[ true,            'true'             ], // boolean coerced to string
	[ NaN,             'NaN'              ], // NaN coerced to string
	[ null,            null               ], // null returns null
	[ undefined,       null               ], // undefined returns null

	// Only the ASCII space (U+0020) is stripped; every other whitespace
	// character is left in place by the / */ regex.
	// Input           Result
	[ '\rhi',          '\rhi'             ], // carriage return is not a space: unchanged
	[ '\fhi',          '\fhi'             ], // form feed is not a space: unchanged
	[ '\vhi',          '\vhi'             ], // vertical tab is not a space: unchanged
	[ '\u00a0hi',      '\u00a0hi'         ], // non-breaking space is not ASCII space: kept
	[ '\u3000hi',      '\u3000hi'         ], // ideographic space is not ASCII space: kept
	[ '  \t',          '\t'               ], // strips leading spaces, stops at the tab
	[ '   😀',         '😀'               ], // spaces stripped, surrogate-pair emoji kept

	// More non-string coercions: String() runs before the regex.
	// Input           Result
	[ '  12',          '12'               ], // numeric string: leading spaces stripped
	[ false,           'false'            ], // boolean coerced to string
	[ Infinity,        'Infinity'         ], // Infinity coerced to string
	[ -Infinity,       '-Infinity'        ], // -Infinity coerced to string
	[ [],              ''                 ], // empty array coerces to ''
	[ ['  hi'],        'hi'               ], // single-element array unwraps then trims
	[ ['  a', '  b'],  'a,  b'            ], // join inserts ',', only first elem's spaces lead
	[ [null],          ''                 ], // [null] coerces to '' (null element -> '')
	[ {},              '[object Object]'  ], // plain object stringifies, no leading spaces
	[ '     xxxxx',    'xxxxx'            ], // longer run of leading spaces all stripped
    ])	{
	test(`cxjs_ltrim(${JSON.stringify(input)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_ltrim(input), result);
	    });
	}
    });
