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
const { describe, it, after } = require('node:test');
const assert                  = require('node:assert/strict');
const env                     = require('./_setup');

let sandbox_username = env.pg_username;

describe('cxjs_user_name', () =>
    {
    // pg_username is a shared sandbox global; save and restore
    // the username to prevent affects on other suites.
    after(()  => { env.pg_username = sandbox_username; });

    for (const [ label, value ] of [
	// Label                Value
	['alice',               'alice'               ],
	['bob',                 'bob'                 ],
	['',                    ''                    ],
	[' !@#$%^&*()":;\' ',   ' !@#$%^&*()":;\' '   ],
	['null',                null                  ],
	['undefined',           undefined             ],
	['number 42',           42                    ],
	['number 0',            0                     ],
	['false',               false                 ],
	['array',               ['a','b']             ],
	['object',              { x: 1 }              ],
    ])	{
	it(`returns pg_username (\"${label}\")`, () =>
	    {
	    env.pg_username = value;
	    assert.equal(env.cxjs_user_name(), value);
	    });
	}
    });
