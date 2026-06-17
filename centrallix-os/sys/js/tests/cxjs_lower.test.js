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

// JSON.stringify renders NaN/Infinity as "null" and a standalone undefined as
// undefined (not a string), which would give distinct edge-case rows the same
// (or a broken) test name; fmt renders those verbatim so names stay unique.
function fmt(v)
    {
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

describe('cxjs_lower', () =>
    {
    for (const [ input, result ] of [
	// Input                Result
	[ 'HELLO',              'hello'        ],
	[ 'hello',              'hello'        ],  // already lowercase
	[ 'Hello World',        'hello world'  ],
	[ 'MiXeD',              'mixed'        ],
	[ '',                   ''             ],
	[ '123ABC!?',           '123abc!?'     ],  // digits/symbols pass through
	[ '   SPACES   ',       '   spaces   ' ],  // leading/trailing spaces kept
	[ '\tT\r',              '\tt\r'        ],  // tab/CR whitespace preserved
	[ 'ÀÉÎ',                'àéî'          ],  // non-ASCII letters
	[ 'İ',                  'i̇'            ],  // one char expands to i + combining dot
	[ 'ΟΔΟΣ',               'οδος'         ],  // trailing Σ lowercases to final sigma ς
    ])	{
	test(`cxjs_lower(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_lower(input), result);
	    });
	}

    // null is returned verbatim; everything else is coerced via String()
    // before lowercasing, so non-strings get their string form lowercased.
    for (const [ input, result ] of [
	// Input        Result
	[ null,         null              ],
	[ undefined,    'undefined'       ],
	[ 5,            '5'               ],
	[ 1.5,          '1.5'             ],
	[ true,         'true'            ],
	[ false,        'false'           ],
	[ NaN,          'nan'             ],
	[ Infinity,     'infinity'        ],
	[ -Infinity,    '-infinity'       ],
	[ [],           ''                ],  // empty array coerces to ''
	[ ['A', 'B'],   'a,b'             ],  // Joins with ',' and no spaces
	[ {},           '[object object]' ],
    ])	{
	test(`cxjs_lower(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_lower(input), result);
	    });
	}
    });
