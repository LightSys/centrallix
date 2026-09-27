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

// JSON.stringify renders undefined/NaN as "null" and -0 as "0", which would
// collapse distinct edge-case rows into duplicate test names; fmt renders them
// verbatim (and recurses into arrays/objects) so each name stays unique.
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

describe('htr_boolean', () =>
    {
    // False values.
    for (const input of [
	null,
	undefined,
	0,
	-0,
	false,
	'',
	' ',        // whitespace coerces to 0
	'\t',
	'0',
	'00',
	'0.0',
	'no', 'No', 'NO',
	'false', 'False', 'FALSE',
	'off', 'Off', 'OFF',
	// More numeric strings that == 0 (compared numerically, not as text).
	'0.00',
	'+0',
	'-0',     // the string '-0', distinct from the number -0 above
	'0e0',
	'0x0',    // hex zero also == 0
	'\n',
	'\r\n',
	'\t\n ',  // mixed whitespace coerces to 0
	// Arrays whose string coercion is empty or a single 0-valued element.
	[],       // [] -> '' -> 0
	[0],      // [0] -> '0' -> == 0
	[''],     // [''] -> '' -> 0
	['0'],    // ['0'] -> '0' -> == 0
    ]) {
	test(`htr_boolean(${fmt(input)}) === false`, () =>
	    {
	    assert.equal(env.htr_boolean(input), false);
	    });
	}

    // True values.
    for (const input of [
	1,
	-1,
	0.5,
	true,
	NaN,
	'1',
	'yes',
	'true',
	'on',
	'hello',
	'n',
	'nope',
	'no ',           // trailing space defeats the 'no' match
	' no',
	Infinity,
	-Infinity,
	{},              // object -> '[object Object]', != 0
	{ a: 1 },
	[1],             // single non-zero element -> '1', != 0
	[1, 2],          // multi-element array -> '1,2', != 0
	[0, 0],          // -> '0,0' which is != 0 numerically
	' false',        // leading space defeats the 'false' match
	'false ',        // trailing space defeats the 'false' match
	'offf',          // not exactly 'off'
	'noo',           // not exactly 'no'
	'null',          // the text, not the value
	'undefined',
	'NaN',           // String(NaN) lower-cases to 'nan', no match
    ]) {
	test(`htr_boolean(${fmt(input)}) === true`, () =>
	    {
	    assert.equal(env.htr_boolean(input), true);
	    });
	}
    });
