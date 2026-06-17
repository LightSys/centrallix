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

// JSON.stringify renders Infinity/NaN as "null"; show numbers verbatim so
// those rows stay distinct, and quote strings so they're not confused with
// numbers (concatenation vs. addition hinges on the operand type).
const fmt = (v) => (typeof v === 'number') ? String(v) : JSON.stringify(v);

describe('cxjs_plus', () =>
    {
    for (const [ a, b, result ] of [
	// a              b              Result
	// Null/undefined operands yield null, even alongside a string.
	[ null,           1,             null      ],
	[ 1,              null,          null      ],
	[ undefined,      1,             null      ],
	[ 1,              undefined,     null      ],
	[ null,           null,          null      ],
	[ null,           'x',           null      ],
	[ undefined,      undefined,     null      ],
	
	// Numeric addition.
	[ 0,              0,             0         ],
	[ 1,              2,             3         ],
	[ -1,             1,             0         ],
	[ -2,            -3,            -5         ],
	[ 1.5,            2.25,          3.75      ],
	[ -2.5,           1.25,         -1.25      ],
	[ Infinity,       1,             Infinity  ],
	[ -Infinity,      1,            -Infinity  ],
	[ Infinity,       Infinity,      Infinity  ],
	[ Infinity,      -Infinity,      NaN       ], // opposite infinities
	
	// String concatenation when either operand is a string.
	[ 'foo',          'bar',         'foobar'    ],
	[ '',             '',            ''          ],
	[ 'a',            '',            'a'         ],
	[ 'x',            1,             'x1'        ],
	[ 1,              'x',           '1x'        ],
	[ 'x',           -1,             'x-1'       ],
	[ '1',            '2',           '12'        ],
	[ '1',            2,             '12'        ],
	[ 0,              '',            '0'         ], // string branch beats numeric 0
	[ 'n',            Infinity,      'nInfinity' ],
	[ 'a',            NaN,           'aNaN'      ],
    ])	{
	test(`cxjs_plus(${fmt(a)}, ${fmt(b)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_plus(a, b), result);
	    });
	}
    });
