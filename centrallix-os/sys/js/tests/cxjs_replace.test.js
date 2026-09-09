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

// replace globally substitutes every literal occurrence of search in str with
// replace, returning null when str or search is null/undefined.
describe('cxjs_replace', () =>
    {
    for (const [ str, search, replace, result ] of [
	// Str            Search   Replace  Result
	[ 'hello world',  'o',     '0',     'hell0 w0rld'     ], // replaces every occurrence
	[ 'hello world',  'h',     'J',     'Jello world'     ], // first character
	[ 'hello world',  'd',     '!',     'hello worl!'     ], // last character
	[ 'hello world',  ' ',     '_',     'hello_world'     ], // whitespace
	[ 'hello world',  'l',     ' ',     'he  o wor d'     ],
	[ 'hello world',  'l',     'l',     'hello world'     ], // identical result
	[ 'hello world',  ' ',     ' ',     'hello world'     ],
	[ 'hello world',  'c',     'c',     'hello world'     ],
	[ 'aaa',          'a',     'b',     'bbb'             ],
	[ 'aaa',          'aa',    'b',     'ba'              ], // non-overlapping, left to right
	[ 'hello',        'z',     'x',     'hello'           ], // absent
	[ 'hello',        'l',     'LL',    'heLLLLo'         ], // replacement longer than match
	[ 'Hello',        'h',     'x',     'Hello'           ], // case-sensitive: no match
	[ 'hello',        'l',     '',      'heo'             ], // empty replace deletes matches

	// Matching is left-to-right and non-overlapping.
	[ 'aaaa',         'aa',    'X',     'XX'              ],
	[ 'abab',         'ab',    'X',     'XX'              ],
	[ 'aaaaa',        'aaa',   'X',     'Xaa'             ],
	
	// Search is matched literally (regex metacharacters have no special meaning).
	[ '1.2.3',        '.',     ',',     '1,2,3'           ],
	[ 'a|b|c',        '|',     '-',     'a-b-c'           ],
	[ '*x*',          '*',     '#',     '#x#'             ],
	[ 'a+b',          '+',     '-',     'a-b'             ],
	[ 'a?b',          '?',     '!',     'a!b'             ],
	[ 'a^b',          '^',     '~',     'a~b'             ],
	[ '$5',           '$',     'USD',   'USD5'            ],
	[ '(x)',          '(',     '[',     '[x)'             ],
	[ '{x}',          '{',     '<',     '<x}'             ],
	[ '[x]',          '[',     '(',     '(x]'             ],
	[ 'a\\b',         '\\',    '/',     'a/b'             ],
	[ '[a]',          ']',     'X',     '[aX'             ],
	[ '(a)',          ')',     'X',     '(aX'             ],
	[ '{a}',          '}',     'X',     '{aX'             ],
	[ 'a/b',          '/',     'X',     'aXb'             ],
	[ 'a-b',          '-',     'X',     'aXb'             ],
	[ 'a.*b',         '.*',    'X',     'aXb'             ],
	[ 'ca.bc',        'a.b',   'X',     'cXc'             ],
	[ 'a\\db',        '\\d',   'X',     'aXb'             ],
	
	// Empty search matches the zero-character gap between characters.
	[ 'ab',           '',      '-',     '-a-b-'           ],
	[ '',             '',      '-',     '-'               ],
	[ '',             'x',     'y',     ''                ], // empty str, no match
	
	// Non-string arguments are coerced to strings.
	[ 12321,          2,       9,       '19391'           ],
	[ true,           'r',     'R',     'tRue'            ],
	[ 'a1b1c',        1,       9,       'a9b9c'           ], // numeric search and replace coerced
	[ 'cat',          'a',     0,       'c0t'             ], // numeric replace coerced (not falsy ->"")
	[ [1,2,3],        ',',     '-',     '1-2-3'           ], // array str coerces

	// Multi-byte / Unicode handling operates on UTF-16 code units.
	[ 'café',         'é',     'X',     'cafX'            ], // single composed code point (U+00E9)
	[ 'x😀y',         '😀',    'Z',     'xZy'             ], // full surrogate pair matched
	[ 'a😀b',         '😀',    'X',     'aXb'             ],
	[ '😀',           '',      '-',     '-\uD83D-\uDE00-' ], // empty search splits the pair's code units

	// Embedded newlines are ordinary characters; a literal '.' does not match them.
	[ 'a\nb\nc',      '\n',    '|',     'a|b|c'           ], // newline searched literally
	[ 'a\nb',         '.',     'X',     'a\nb'            ], // '.' is literal, matches nothing here
    ])	{
	test(`cxjs_replace(`
	    + `${JSON.stringify(str)}, `
	    + `${JSON.stringify(search)}, `
	    + `${JSON.stringify(replace)}) `
	    + `= ${JSON.stringify(result)}`,
	() =>
	    {
	    assert.equal(env.cxjs_replace(str, search, replace), result);
	    });
	}

    // null or undefined str/search short-circuits to null; a null/undefined replace
    // (including an omitted third argument) instead defaults to "".
    for (const [ str, search, replace, result ] of [
	// Str        Search      Replace     Result
	[ null,       'a',        'b',        null  ],
	[ undefined,  'a',        'b',        null  ],
	[ 'x',        null,       'b',        null  ],
	[ 'x',        undefined,  'b',        null  ],
	[ null,       null,       'b',        null  ],
	[ 'hello',    'l',        null,       'heo' ], // null replace -> deletes matches
	[ 'hello',    'l',        undefined,  'heo' ], // undefined replace -> deletes matches
    ])	{
	test(`cxjs_replace(${str}, ${search}, ${replace}) = ${JSON.stringify(result)}`, () =>
	    {
	    assert.equal(env.cxjs_replace(str, search, replace), result);
	    });
	}

    // replace is uses String.replace() logic, allowing $ patterns to be used:
    // $& is the match, $$ a literal $, $` and $' the text before/after.
    for (const [ str, search, replace, result ] of [
	// Str       Search   Replace     Result
	[ 'cat',     'a',     '$&',       'cat'     ], // match reinserted unchanged
	[ 'cat',     'a',     '[$&]',     'c[a]t'   ],
	[ 'cat',     'a',     '$$',       'c$t'     ],
	[ 'cat',     'a',     '$`',       'cct'     ], // text preceding the match
	[ 'cat',     'a',     "$'",       'ctt'     ], // text following the match
	[ 'cat',     'a',     '$1',       'c$1t'    ], // no capture group: $1 left verbatim
	[ 'cat',     'a',     '$0',       'c$0t'    ], // $0 is not special: left verbatim
	[ 'cat',     'a',     '$<x>',     'c$<x>t'  ], // no named groups: $<x> left verbatim
	[ 'cat',     'a',     '$$&',      'c$&t'    ], // escaped $ then literal &: yields "$&"
	[ 'cat',     'a',     '$$1',      'c$1t'    ], // escaped $ then "1": yields "$1"
    ])	{
	test(`cxjs_replace(`
	    + `${JSON.stringify(str)}, `
	    + `${JSON.stringify(search)}, `
	    + `${JSON.stringify(replace)}) `
	    + `= ${JSON.stringify(result)}`,
	() =>
	    {
	    assert.equal(env.cxjs_replace(str, search, replace), result);
	    });
	}

    // Omitted replace defaults to "", deleting every match.
    test("cxjs_replace('hello', 'l') = 'heo'", () =>
	{
	assert.equal(env.cxjs_replace('hello', 'l'), 'heo');
	});
    });
