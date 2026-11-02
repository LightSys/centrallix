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

describe('cxjs_rtrim', () =>
    {
    for (const [ s, result ] of [
	// String          Result
	[ 'hello   ',      'hello'            ], // trailing spaces trimmed
	[ '   hello',      '   hello'         ], // leading spaces preserved
	[ '   hello   ',   '   hello'         ], // only trailing trimmed
	[ 'hello',         'hello'            ], // no change
	[ 'a b ',          'a b'              ], // internal spaces preserved
	[ '',              ''                 ], // empty stays empty
	[ ' ',             ''                 ], // single space
	[ '   ',           ''                 ], // all spaces trimmed
	[ 'hello\t',       'hello\t'          ], // trailing tab not trimmed
	[ 'hello\n',       'hello\n'          ], // trailing newline not trimmed
	[ 'hello\t ',      'hello\t'          ], // space after tab trimmed
	[ 'hello \t',      'hello \t'         ], // space before tab preserved
	[ 42,              '42'               ], // number coerced to string
	[ 0,               '0'                ], // falsy but not null: coerced, not dropped
	[ true,            'true'             ], // boolean coerced to string
	[ NaN,             'NaN'              ], // NaN coerced to string
	[ null,            null               ], // null returns null
	[ undefined,       null               ], // undefined returns null

	// Only the ASCII space (U+0020) is stripped; every other trailing
	// whitespace character is left in place.
	[ 'hi\r',          'hi\r'             ], // carriage return is not a space: unchanged
	[ 'hi\f',          'hi\f'             ], // form feed is not a space: unchanged
	[ 'hi\v',          'hi\v'             ], // vertical tab is not a space: unchanged
	[ 'hi\u00a0',      'hi\u00a0'         ], // non-breaking space is not ASCII space: kept
	[ '\t  ',          '\t'               ], // strips trailing spaces, stops at the tab
	[ '😀   ',         '😀'               ], // surrogate-pair emoji kept, spaces stripped

	// Non-string coercion edge cases.
	[ '12  ',          '12'               ], // numeric string: trailing spaces stripped
	[ false,           'false'            ], // boolean coerced to string
	[ Infinity,        'Infinity'         ], // Infinity coerced to string
	[ -Infinity,       '-Infinity'        ], // -Infinity coerced to string
	[ [],              ''                 ], // empty array coerces to ''
	[ ['hi  '],        'hi'               ], // single-element array unwraps then trims
	[ [null],          ''                 ], // [null] coerces to '' (null element -> '')
	[ {},              '[object Object]'  ], // plain object stringifies, no trailing spaces
	[ 'xxxxx     ',    'xxxxx'            ], // longer run of trailing spaces all stripped
    ])	{
	test(`cxjs_rtrim(${JSON.stringify(s)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_rtrim(s), result);
	    });
	}
    });
