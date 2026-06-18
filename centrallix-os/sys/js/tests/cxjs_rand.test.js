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

// Renders NaN/Infinity/undefined verbatim so seed test names stay unique
// (JSON.stringify would collapse them).
function fmt(v)
    {
    if (Array.isArray(v))
	return '[' + v.map(fmt).join(',') + ']';
    if (v !== null && typeof v === 'object')
	return '{' + Object.keys(v).map((k) => JSON.stringify(k) + ':' + fmt(v[k])).join(',') + '}';
    if (typeof v === 'number' || v === undefined)
	return String(v);
    return JSON.stringify(v);
    }

// Runs fn with the sandbox's console.warn replaced by a counter, returning
// the number of warnings emitted. Restores the real console afterward.
function captureWarningCount(fn)
    {
    const real = env.console;
    let count = 0;
    env.console = { warn: () => { count++; } };
    try { fn(); }
    finally { env.console = real; }
    return count;
    }

describe('cxjs_rand', () =>
    {
    // Result is always a float in [0,1).
    test('returns a float in [0,1)', () =>
	{
	for (let i = 0; i < 1000; i++)
	    {
	    const n = env.cxjs_rand();
	    assert.equal(typeof n, 'number');
	    assert.ok(n >= 0 && n < 1, `${n} not in [0,1)`);
	    }
	});

    // Output is not constant across calls.
    test('produces varied output', () =>
	{
	const seen = new Set();
	for (let i = 0; i < 1000; i++) seen.add(env.cxjs_rand());
	assert.ok(seen.size > 1, 'expected more than one distinct value');
	});

    // A fixed seed is ignored, so it does not pin the output to one value.
    test('ignores the seed (output stays non-deterministic)', () =>
	{
	const seen = new Set();
	captureWarningCount(() => // Warnings suppressed (tested later).
	    {
	    for (let i = 0; i < 1000; i++) seen.add(env.cxjs_rand(42));
	    });
	assert.ok(seen.size > 1, 'seed should not make output deterministic');
	});

    // Any non-null/undefined seed -- even a falsy one -- emits one warning
    // and still returns a valid result.
    for (const seed of [0, 1, 42, -5, 3.14, '', 'abc', false, true, NaN, Infinity, [], {}])
	{
	test(`warns when given seed ${fmt(seed)}`, () =>
	    {
	    let result;
	    const warnings = captureWarningCount(() => { result = env.cxjs_rand(seed); });
	    assert.equal(warnings, 1);
	    assert.equal(typeof result, 'number');
	    assert.ok(result >= 0 && result < 1, `${result} not in [0,1)`);
	    });
	}

    // No seed, or an explicit null/undefined, produces no warning.
    for (const [ label, rand_fn ] of [
	[ 'no argument', () => env.cxjs_rand()          ],
	[ 'null',        () => env.cxjs_rand(null)      ],
	[ 'undefined',   () => env.cxjs_rand(undefined) ],
    ])	{
	test(`does not warn with ${label}`, () =>
	    {
	    let result;
	    const warnings = captureWarningCount(() => { result = rand_fn(); });
	    assert.equal(warnings, 0);
	    assert.ok(result >= 0 && result < 1, `${result} not in [0,1)`);
	    });
	}
    });
