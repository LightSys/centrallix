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
    for (const [ dt, v, result ] of [
	// Datatype    Value       Result
	[ null,        5,          null ],
	[ undefined,   5,          null ],
	[ 'integer',   null,       null ],
	[ 'integer',   undefined,  null ],
	[ 'double',    null,       null ],
	[ 'string',    null,       null ],
	[ null,        null,       null ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(dt)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(dt, v), result);
	    });
	}

    // Conversion to integer. Note the '$' is only stripped when it is
    // the *second* character (e.g. 'x$5'); a leading '$5' is not, so it
    // parses to NaN. Stripping the '$' also drops a leading sign.
    for (const [ dt, v, result ] of [
	// Datatype    Value       Result
	[ 'integer',   0,          0    ],
	[ 'integer',   5,          5    ],
	[ 'integer',  -7,         -7    ],
	[ 'integer',   5.9,        5    ],
	[ 'integer',   '42',       42   ],
	[ 'integer',   '42abc',    42   ],
	[ 'integer',   '  10',     10   ],
	[ 'integer',   '0x1F',     31   ],
	[ 'integer',   '1e3',      1    ],
	[ 'integer',   'x$5',      5    ],
	[ 'integer',   '-$5',      5    ],
	[ 'integer',   '$5',       NaN  ],
	[ 'integer',   '',         NaN  ],
	[ 'integer',   'abc',      NaN  ],
	[ 'integer',   true,       NaN  ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(dt)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(dt, v), result);
	    });
	}

    // Conversion to double. A leading currency marker is stripped in
    // several forms ('$', ' $', '+$', '$ ', '-$'); '-$' negates the
    // result. Anything else falls through to parseFloat.
    for (const [ dt, v, result ] of [
	// Datatype    Value        Result
	[ 'double',    0,           0    ],
	[ 'double',    5.5,         5.5  ],
	[ 'double',    '5.5',       5.5  ],
	[ 'double',    '3.14abc',   3.14 ],
	[ 'double',    '$5',        5    ],
	[ 'double',    '$5.50',     5.5  ],
	[ 'double',    ' $5',       5    ],
	[ 'double',    '+$5',       5    ],
	[ 'double',    '$ 5',       5    ],
	[ 'double',    '-$5',      -5    ],
	[ 'double',    '-$2.5',    -2.5  ],
	[ 'double',    '$1,000',    1    ],
	[ 'double',    '$',         NaN  ],
	[ 'double',    'abc',       NaN  ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(dt)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(dt, v), result);
	    });
	}

    // Conversion to string is just '' + v, so every value is coerced.
    for (const [ dt, v, result ] of [
	// Datatype    Value       Result
	[ 'string',    0,          '0'      ],
	[ 'string',    5,          '5'      ],
	[ 'string',    5.5,        '5.5'    ],
	[ 'string',   -3.2,        '-3.2'   ],
	[ 'string',    'hello',    'hello'  ],
	[ 'string',    true,       'true'   ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(dt)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(dt, v), result);
	    });
	}

    // An unrecognized datatype returns the value unchanged.
    for (const [ dt, v, result ] of [
	// Datatype     Value      Result
	[ 'money',      5,         5    ],
	[ 'datetime',   'x',       'x'  ],
	[ 'MyType',     42,        42   ],
    ])	{
	test(`cxjs_convert(${JSON.stringify(dt)}, ${JSON.stringify(v)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_convert(dt, v), result);
	    });
	}
    });
