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

describe('cxjs_reverse', () =>
    {
    for (const [ input, result ] of [
	// Input            Result
	[ 'abc',            'cba'           ],
	[ 'a',              'a'             ],
	[ '',               ''              ],
	[ 'aba',            'aba'           ],  // palindrome unchanged
	[ 'Hello World',    'dlroW olleH'   ],
	[ 'a  b c',         'c b  a'        ],  // spaces reversed
	[ '\ta\n',          '\na\t'         ],  // whitespace reversed
	[ ' \tx ',          ' x\t '         ],  // surrounding whitespace preserved
	[ '123!?',          '?!321'         ],  // digits/symbols reverse like any char
	[ 'àéî',            'îéà'           ],  // non-ASCII letters: a/e/i w/ accents
    ])	{
	test(`cxjs_reverse(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_reverse(input), result);
	    });
	}

    // null and undefined both short-circuit to null.  Other non-strings are
    // coerced before reversal.
    for (const [ input, result ] of [
	// Input        Result
	[ null,         null         ],
	[ undefined,    null         ],
	[ 123,          '321'        ],
	[ 1.5,          '5.1'        ],
	[ true,         'eurt'       ],
	[ false,        'eslaf'      ],
	[ NaN,          'NaN'        ],  // 'NaN' reverses to itself
	[ Infinity,     'ytinifnI'   ],
	[ -Infinity,    'ytinifnI-'  ],
	[ [],           ''           ],  // empty array coerces to ''
	[ ['a', 'b'],   'b,a'        ],
    ])	{
	test(`cxjs_reverse(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_reverse(input), result);
	    });
	}

    // Reversal walks UTF-16 code units, not whole characters, so surrogate
    // pairs and combining marks get split apart and reordered.  Escapes are
    // used directly so these cases do not depend on the file's encoding.
    for (const [ name, input, result ] of [
	// emoji (one code point, two code units) splits into swapped halves
	[ 'surrogate pair split',      '\uD83D\uDE00',     '\uDE00\uD83D'  ],
	[ 'pair split before char',    '\uD83D\uDE00a',    'a\uDE00\uD83D' ],
	// 'e' + combining acute reverses to combining acute + 'e'
	[ 'combining mark reorder',    'e\u0301',          '\u0301e'  ],
    ])	{
	test(`cxjs_reverse: ${name}`, () =>
	    {
	    assert.equal(env.cxjs_reverse(input), result);
	    });
	}
    });
