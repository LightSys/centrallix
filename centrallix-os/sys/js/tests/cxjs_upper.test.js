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

// JSON.stringify renders undefined as the bare word "undefined" only inside
// arrays; as a standalone value it yields undefined (not a string), which
// breaks template names. fmt renders such values verbatim so names stay unique.
function fmt(v)
    {
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

describe('cxjs_upper', () =>
    {
    for (const [ input, result ] of [
	// Input               Result
	[ 'abc',               'ABC'              ],
	[ 'ABC',               'ABC'              ],
	[ 'AbC',               'ABC'              ],
	[ '',                  ''                 ],
	[ 'hello world',       'HELLO WORLD'      ],
	[ 'a1b2c3',            'A1B2C3'           ],  // digits pass through
	[ '!@# $%^',           '!@# $%^'          ],  // punctuation/space unchanged
	[ '\tn\r',             '\tN\r'            ],  // whitespace preserved
	[ 'café',              'CAFÉ'             ],  // accented letter uppercases
	[ 'ß',                 'SS'               ],  // sharp-s expands to two chars

	// Non-strings are String()-coerced first, then uppercased. Only strict
	// null short-circuits to null; undefined does not.
	[ null,                null               ],  // sole short-circuit case
	[ undefined,           'UNDEFINED'        ],  // not null, so coerced
	[ 123,                 '123'              ],
	[ 1.5,                 '1.5'              ],
	[ true,                'TRUE'             ],
	[ false,               'FALSE'            ],
	[ NaN,                 'NAN'              ],
	[ Infinity,            'INFINITY'         ],
	[ ['a', 'b'],          'A,B'              ],  // Joins with ',' and no spaces
	[ {},                  '[OBJECT OBJECT]'  ],  // plain object stringifies
    ])	{
	test(`cxjs_upper(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_upper(input), result);
	    });
	}
    });
