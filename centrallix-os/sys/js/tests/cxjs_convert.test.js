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

describe('cxjs_convert', () =>
    {
    // A null datatype or value yields null, regardless of the other
    // argument (== null also catches undefined).
    for (const [ datatype, v, result ] of [
	// Datatype    Value       Result
	[ null,        5,          null ],
	[ undefined,   5,          null ],
	[ 'integer',   null,       null ],
	[ 'integer',   undefined,  null ],
	[ 'double',    null,       null ],
	[ 'string',    null,       null ],
	[ null,        null,       null ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(datatype, v), result);
	    });
	}

    // Conversion to integer.
    // Note:  The '$' is only stripped when it is the *second* character
    // (e.g. 'x$5').  A leading '$5' is not, so it parses to NaN. Stripping
    // the '$' also drops a leading sign.
    for (const [ datatype, v, result ] of [
	// Datatype    Value      Result
	[ 'integer',   0,         0    ],
	[ 'integer',   5,         5    ],
	[ 'integer',  -7,        -7    ],
	[ 'integer',   5.9,       5    ],
	[ 'integer',   '42',      42   ],
	[ 'integer',   '42abc',   42   ],
	[ 'integer',   '  10',    10   ],  // Whitespace is stripped.
	[ 'integer',   '0x1F',    31   ],  // Hex notation is handled.
	[ 'integer',   '1e3',     1    ],  // Scientific notation is ignored.
	[ 'integer',   'x$5',     5    ],
	[ 'integer',   '-$5',     5    ],
	[ 'integer',   '$5',      NaN  ],
	[ 'integer',   '',        NaN  ],
	[ 'integer',   'abc',     NaN  ],
	[ 'integer',   true,      NaN  ],
	[ 'integer',   '-42',    -42   ],  // Leading sign kept when '$' is not the 2nd char.
	[ 'integer',   '  -42',  -42   ],  // Whitespace stripped, sign kept.
	[ 'integer',   'a$bc',    NaN  ],  // 2nd char '$' stripped, but 'bc' isn't numeric.
	[ 'integer',   Infinity,  NaN  ],  // parseInt('Infinity') is NaN.
	[ 'integer',   false,     NaN  ],
	[ 'integer',   '+5',      5    ],  // Leading '+' handled.
	[ 'integer',   '0x10',    16   ],  // Hex notation handled.
	[ 'integer',   ' $12',    12   ],  // 2nd char '$' stripped, parses '12'.
	[ 'integer',   'x$-3',   -3    ],  // Stripped to '-3', sign honored.
	[ 'integer',   'x$',      NaN  ],  // Stripped to '', parseInt('') is NaN.
	[ 'integer',   [ 42 ],    42   ],  // Array stringifies to '42'.
	[ 'integer',   [ 42, 1 ], 42   ],  // '42,1' -> parseInt stops at comma.
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(datatype, v), result);
	    });
	}

    // Conversion to double.
    // A leading currency marker is stripped in several forms ('$', ' $',
    // '+$', '$ ', '-$'). '-$' negates the result. Anything else is passed
    // through to parseFloat.
    for (const [ datatype, v, result ] of [
	// Datatype   Value        Result
	[ 'double',   0,           0        ],
	[ 'double',   5.5,         5.5      ],
	[ 'double',   '5.5',       5.5      ],
	[ 'double',   '3.14abc',   3.14     ],
	[ 'double',   '$5',        5        ],
	[ 'double',   '$5.50',     5.5      ],
	[ 'double',   ' $5',       5        ],
	[ 'double',   '+$5',       5        ],
	[ 'double',   '$ 5',       5        ],
	[ 'double',   '-$5',      -5        ],
	[ 'double',   '-$ 5',     -5        ],
	[ 'double',   '-$2.5',    -2.5      ],
	[ 'double',   '$1,000',    1        ],
	[ 'double',   '-5.5',     -5.5      ],  // Plain negative, no currency marker.
	[ 'double',   Infinity,    Infinity ],  // parseFloat('Infinity') is Infinity.
	[ 'double',   'Infinity',  Infinity ],
	[ 'double',   '$',         NaN      ],
	[ 'double',   'abc',       NaN      ],
	[ 'double',   '1.5e3',     1500     ],  // Scientific notation honored.
	[ 'double',   '  12',      12       ],  // Leading whitespace tolerated by parseFloat.
	[ 'double',   '',          NaN      ],
	[ 'double',   '-$',        NaN      ],  // -parseFloat('') -> -NaN -> NaN.
	[ 'double',   ' $',        NaN      ],  // Strips ' $', parseFloat('') is NaN.
	[ 'double',   '+$',        NaN      ],
	[ 'double',   '$ ',        NaN      ],
	[ 'double',   '  $5',      NaN      ],  // Two leading spaces: no prefix matches, parseFloat fails.
	[ 'double',   '1,234',     1        ],  // parseFloat stops at the comma.
	[ 'double',   true,        NaN      ],  // parseFloat('true') is NaN.
	[ 'double',   false,       NaN      ],
	[ 'double',   [ 1.5 ],     1.5      ],  // Array stringifies to '1.5'.
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(datatype, v), result);
	    });
	}

    // Conversion to string (using standard JS coercion).
    for (const [ datatype, v, result ] of [
	// Datatype    Value       Result
	[ 'string',    0,          '0'               ],
	[ 'string',    5,          '5'               ],
	[ 'string',    5.5,        '5.5'             ],
	[ 'string',   -3.2,        '-3.2'            ],
	[ 'string',    'hello',    'hello'           ],
	[ 'string',    true,       'true'            ],
	[ 'string',    false,      'false'           ],
	[ 'string',    Infinity,   'Infinity'        ],
	[ 'string',   -Infinity,   '-Infinity'       ],
	[ 'string',    NaN,        'NaN'             ],
	[ 'string',    '',         ''                ],
	[ 'string',    [ 1, 2, 3 ], '1,2,3'          ],  // Arrays join with commas.
	[ 'string',    [],         ''                ],
	[ 'string',    { a: 1 },   '[object Object]' ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(datatype, v), result);
	    });
	}

    // Signed zero.
    test('cxjs_convert("double", "-$0") is -0', () =>
	{
	assert.ok(Object.is(env.cxjs_convert('double', '-$0'), -0));
	});
    test('cxjs_convert("double", "$0") is +0', () =>
	{
	assert.ok(Object.is(env.cxjs_convert('double', '$0'), 0));
	});
    test('cxjs_convert("string", -0) is "0"', () =>
	{
	assert.equal(env.cxjs_convert('string', -0), '0');
	});

    // datatype that is falsy-but-not-null (0, '', false).
    for (const [ datatype, v, result ] of [
	// Datatype     Value      Result
	[ 0,            5,         5      ],
	[ '',           5,         5      ],
	[ false,        5,         5      ],
	[ 'INTEGER',    '5abc',    '5abc' ],  // Wrong case -> unknown datatype -> unchanged.
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.deepEqual(env.cxjs_convert(datatype, v), result);
	    });
	}

    // An unrecognized datatype returns the value unchanged.
    for (const [ datatype, v, result ] of [
	// Datatype     Value      Result
	[ 'money',      5,         5    ],
	[ 'datetime',   'x',       'x'  ],
	[ 'MyType',     42,        42   ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(datatype)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(datatype, v), result);
	    });
	}
    });
