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

// cxjs_quote() wraps a value in double quotes, escaping any embedded
// double quote as \". Backslashes are NOT escaped, and non-string
// inputs are coerced via String() first.
describe('cxjs_quote', () =>
    {
    for (const [ input, result ] of [
	// Input                Result
	[ '',                   '""'                  ],
	[ 'hello',              '"hello"'             ],
	[ '"',                  '"\\""'               ],  // lone quote
	[ '""',                 '"\\"\\""'            ],  // adjacent quotes
	[ 'a"b',                '"a\\"b"'             ],  // quote in middle
	[ '\\',                 '"\\"'                ],  // backslash quoted as-is
	[ '\\"',                '"\\\\""'             ],  // backslash + escaped quote
	[ "'x'",                '"\'x\'"'             ],  // single quotes untouched
	[ 'a\nb',               '"a\nb"'              ],  // newline preserved
	[ '\t ',                '"\t "'               ],  // tab/space preserved
	[ 123,                  '"123"'               ],  // number coerced
	[ true,                 '"true"'              ],  // boolean coerced
	[ null,                 '"null"'              ],  // null coerced
	[ undefined,            '"undefined"'         ],  // undefined coerced
	[ NaN,                  '"NaN"'               ],  // NaN coerced
	[ [1, 2],               '"1,2"'               ],  // array coerced (with no spaces)
	[ {},                   '"[object Object]"'   ],  // plain object coerced
	[ { a: 1 },             '"[object Object]"'   ],  // object contents are irrelevant to String()
    ])	{
	test(`cxjs_quote(${JSON.stringify(input)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_quote(input), result);
	    });
	}
    });
