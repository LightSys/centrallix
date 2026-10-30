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
	[ Infinity,             '"Infinity"'          ],  // Infinity coerced
	[ -Infinity,            '"-Infinity"'         ],  // -Infinity coerced
	[ NaN,                  '"NaN"'               ],  // NaN coerced
	[ [1, 2],               '"1,2"'               ],  // array coerced (with no spaces)
	[ {},                   '"[object Object]"'   ],  // plain object coerced
	[ { a: 1 },             '"[object Object]"'   ],  // object contents are irrelevant to String()
	[ '\\\\',               '"\\\\"'              ],  // two backslashes both pass as is (unescaped)
	[ 'a"b"c',              '"a\\"b\\"c"'         ],  // every embedded quote is escaped
	[ '\r',                 '"\r"'                ],  // carriage return preserved
	[ '😀',                 '"😀"'                ],  // multi-byte char passes as is
	[ 0,                    '"0"'                 ],  // numeric zero coerced
	[ -0,                   '"0"'                 ],  // negative zero renders as "0"
	[ false,                '"false"'             ],  // boolean false coerced
	[ [],                   '""'                  ],  // empty array coerces to ""
	[ [null],               '""'                  ],  // null element renders as "" inside array
	[ ['a', null],          '"a,"'                ],  // null element is empty between commas
	[ ['a"b'],              '"a\\"b"'             ],  // quote inside array element still escaped
    ])	{
	test(`cxjs_quote(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_quote(input), result);
	    });
	}
    });
