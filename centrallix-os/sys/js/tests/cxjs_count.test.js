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

describe('cxjs_count', () =>
    {
    for (const [ input, result ] of [
	// Input                                   Result
	[ [],                                      0 ],
	[ [0, 1],                                  2 ],
	[ [0, 1, 2, 3, 4],                         5 ],
	[ [9, 10.1, 10.2],                         3 ],
	[ [-1.1, 1],                               2 ],
	[ [-Infinity, Infinity],                   2 ],
	// null and undefined are not counted.
	[ [null],                                  0 ],
	[ [undefined],                             0 ],
	[ [null, undefined],                       0 ],
	[ [null, 0],                               1 ],
	[ [undefined, 0],                          1 ],
	// NaN is not counted.
	[ [NaN],                                   0 ],
	[ [NaN, 1],                                1 ],
	[ [NaN, null, undefined, 5],               1 ],
	// Numeric strings count; non-numeric strings do not. "" is numeric (0).
	[ ["5", 6],                                2 ],
	[ ["", 6],                                 2 ],
	[ ["abc", 6],                              1 ],
	[ ["abc", "def"],                          0 ],
    ])	{
	test(`cxjs_count(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_count(input), result);
	    });
	}

    for (const [ input, result ] of [
	// Input                                   Result
	[ {},                                      0 ],
	[ { a: 0, b: 1 },                          2 ],
	[ { c: 0, d: 1, e: 2, f: 3, g: 4 },        5 ],
	[ { l: 9, m: 10.1, n: 10.2 },              3 ],
	[ { t: -1.1, u: 1 },                       2 ],
	[ { v: -Infinity, w: Infinity },           2 ],
	// null and undefined are not counted.
	[ { D: null },                             0 ],
	[ { E: undefined },                        0 ],
	[ { F: null, G: undefined },               0 ],
	[ { H: null, I: 0 },                       1 ],
	[ { J: undefined, K: 0 },                  1 ],
	// NaN is not counted.
	[ { L: NaN },                              0 ],
	[ { M: NaN, N: 1 },                        1 ],
	[ { O: NaN, P: null, Q: undefined, R: 5 }, 1 ],
	// Numeric strings count; non-numeric strings do not. "" is numeric (0).
	[ { S: "5", T: 6 },                        2 ],
	[ { U: "", V: 6 },                         2 ],
	[ { W: "abc", X: 6 },                      1 ],
	[ { Y: "abc", Z: "def" },                  0 ],
    ])	{
	test(`cxjs_count(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_count(input), result);
	    });
	}

    // Scalar (non-array, non-object) inputs always count as one, even when
    // the value is null, undefined, or NaN.
    for (const [ input, result ] of [
	// Input        Result
	[ 0,            1 ],
	[ 42,           1 ],
	[ -1.1,         1 ],
	[ Infinity,     1 ],
	[ "foo",        1 ],
	[ "",           1 ],
	[ true,         1 ],
	[ NaN,          1 ],
	[ null,         1 ],
	[ undefined,    1 ],
    ])	{
	test(`cxjs_count(${JSON.stringify(input)}) = ${result}`, () =>
	    {
	    assert.equal(env.cxjs_count(input), result);
	    });
	}
    });
