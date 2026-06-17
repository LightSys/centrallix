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

describe('cxjs_condition', () =>
    {
    for (const [ c, vtrue, vfalse, result ] of [
	// Condition           vtrue  vfalse  Result
	// null/undefined short-circuit to null before vtrue/vfalse.
	[ null,                'T',       'F',    null      ],
	[ undefined,           'T',       'F',    null      ],
	// Truthy conditions yield vtrue.
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
	// Falsy conditions yield vfalse.
	[ false,               'T',       'F',    'F'       ],
	[ 0,                   'T',       'F',    'F'       ],
	[ -0,                  'T',       'F',    'F'       ],
	[ NaN,                 'T',       'F',    'F'       ],
	[ '',                  'T',       'F',    'F'       ],
	// vtrue/vfalse pass through verbatim, regardless of type.
	[ true,                42,        99,     42        ],
	[ false,               42,        99,     99        ],
	[ true,                null,      'F',    null      ],  // truthy c can still return null
	[ true,                undefined, 'F',    undefined ],
	[ false,               'T', undefined,    undefined ],
    ])	{
	test(`cxjs_condition(${JSON.stringify(c)}, ${JSON.stringify(vtrue)}, ${JSON.stringify(vfalse)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_condition(c, vtrue, vfalse), result);
	    });
	}
    });
