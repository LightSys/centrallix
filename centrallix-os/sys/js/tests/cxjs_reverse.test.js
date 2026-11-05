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

// JSON.stringify renders NaN/Infinity as "null", a standalone undefined as
// undefined (not a string), and -0 as "0", which would give distinct edge-case
// rows the same (or a broken) test name; fmt renders those verbatim (and
// recurses into arrays/objects) so names stay unique.
function fmt(v)
    {
    if (Array.isArray(v))
	return '[' + v.map(fmt).join(',') + ']';
    if (v !== null && typeof v === 'object')
	return '{' + Object.keys(v).map((k) => JSON.stringify(k) + ':' + fmt(v[k])).join(',') + '}';
    if (Object.is(v, -0))
	return '-0';
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
	[ 'a"b',            'b"a'           ],  // embedded double quote reverses like any char
	[ 'a\\b',           'b\\a'          ],  // embedded backslash reverses like any char
	[ 'aabb',           'bbaa'          ],  // repeated chars
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
	[ -12,          '21-'        ],
	[ true,         'eurt'       ],
	[ false,        'eslaf'      ],
	[ NaN,          'NaN'        ],  // 'NaN' reverses to itself
	[ Infinity,     'ytinifnI'   ],
	[ -Infinity,    'ytinifnI-'  ],
	[ [],           ''           ],  // empty array coerces to ''
	[ ['a'],        'a'          ],
	[ ['a', 'b'],   'b,a'        ],
	[ 0,            '0'          ],  // numeric zero coerces to '0'
	[ -0,           '0'          ],  // negative zero renders as '0'
	[ [1, 2, 3],    '3,2,1'      ],  // commas reverse with digits
	[ {},           ']tcejbO tcejbo['], // object -> '[object Object]' reversed
    ])	{
	test(`cxjs_reverse(${fmt(input)}) = ${fmt(result)}`, () =>
	    {
	    assert.equal(env.cxjs_reverse(input), result);
	    });
	}

    // Reversal walks UTF-16 code units, not whole characters, so surrogate
    // pairs and combining marks get split apart and reordered.  Escapes are
    // used directly so these cases do not depend on file encodings.
    for (const [ name, input, result ] of [
	// emoji (one code point, two code units) splits into swapped halves.
	[ 'surrogate pair split',      '\uD83D\uDE00',     '\uDE00\uD83D'  ],
	[ 'pair split before char',    '\uD83D\uDE00a',    'a\uDE00\uD83D' ],
	// each emoji's two halves swap in place, so the run is fully reordered.
	[ 'two emoji halves reorder',  '\uD83D\uDE00\uD83D\uDE00', '\uDE00\uD83D\uDE00\uD83D' ],
	// char before an emoji ends up after the swapped halves.
	[ 'char before pair split',    'a\uD83D\uDE00b',   'b\uDE00\uD83Da' ],
	// 'e' + combining acute reverses to combining acute + 'e'.
	[ 'combining mark reorder',    'e\u0301',          '\u0301e'  ],
    ])	{
	test(`cxjs_reverse: ${name}`, () =>
	    {
	    assert.equal(env.cxjs_reverse(input), result);
	    });
	}
    });
