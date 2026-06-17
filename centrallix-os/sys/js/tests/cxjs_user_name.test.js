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
const { describe, it } = require('node:test');
const assert           = require('node:assert/strict');
const env              = require('./_setup');

describe('cxjs_user_name', () =>
    {
    for (const name of [
	'alice',
	'bob',
	'',
	' !@#$%^&*()":;\' ',
    ])	{
	it(`returns pg_username (\"${name}\")`, () =>
	    {
	    env.pg_username = name;
	    assert.equal(env.cxjs_user_name(), name);
	    });
	}
    });
