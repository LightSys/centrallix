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
	// Input        Result
	[ '  hello',    'hello'     ], // leading spaces stripped
	[ 'hello',      'hello'     ], // no leading spaces
	[ 'hello  ',    'hello  '   ], // trailing spaces preserved
	[ '  a b  ',    'a b  '     ], // interior/trailing spaces preserved
	[ ' ',          ''          ], // single space
	[ '   ',        ''          ], // all spaces collapse to empty
	[ '',           ''          ], // empty stays empty
	[ ' \thello',   '\thello'   ], // leading space stripped, tab kept
	[ '\t hello',   '\t hello'  ], // leading tab is not a space: unchanged
	[ '\nhi',       '\nhi'      ], // newline is not a space: unchanged
	[ 42,           '42'        ], // non-string is stringified
	[ 0,            '0'         ], // falsy but not null: coerced, not dropped
	[ true,         'true'      ], // boolean coerced to string
	[ NaN,          'NaN'       ], // NaN coerced to string
	[ null,         null        ], // null returns null
	[ undefined,    null        ], // undefined returns null
    ])	{
	test(`cxjs_ltrim(${JSON.stringify(input)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_ltrim(input), result);
	    });
	}
    });
