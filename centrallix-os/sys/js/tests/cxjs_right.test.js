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

describe('cxjs_right', () =>
    {
    for (const [ s, l, result ] of [
	// String          Length  Result
	[ "hello",         2,      "lo"      ],
	[ "hello",         1,      "o"       ],
	[ "hello",         5,      "hello"   ], // l equals length: whole string
	[ "hello",         10,     "hello"   ], // l exceeds length: whole string
	[ "hello",         0,      ""        ], // zero length: empty
	[ "hello",        -2,      ""        ], // negative length: empty
	[ "",              3,      ""        ], // empty string stays empty
	[ "",              0,      ""        ],
	[ "x",             1,      "x"       ], // single char
	[ "  ab ",         2,      "b "      ], // trailing whitespace preserved
	[ "  ab ",         4,      " ab "    ], // preceding whitespace preserved
	[ "abc",           null,   null      ], // null length
	[ "abc",           undefined, null   ], // undefined length
	[ null,            2,      null      ], // null string
	[ undefined,       2,      null      ], // undefined string
	[ null,            null,   null      ],
    ])	{
	test(`cxjs_right(${JSON.stringify(s)}, ${l}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_right(s, l), result);
	    });
	}
    });
