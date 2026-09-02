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

// Loads ht_render.js into a node:vm sandbox so its cxjs_* functions
// can be exercised under node:test without modifying the source file
// or running a real browser. The sandbox object is exported; tests
// call functions off it (env.cxjs_*) and may mutate page-level
// globals such as pg_username between assertions.

'use strict';
const fs   = require('node:fs');
const path = require('node:path');
const vm   = require('node:vm');

const HT_RENDER_PATH = path.resolve(__dirname, '..', 'ht_render.js');

// Minimal stubs for the page/browser globals ht_render.js references.
const sandbox =
    {
    pg_username:    'test_user',
    pg_clockoffset: 0,
    pg_expaddpart:  () => {},
    window:         {},
    document:
	{
	getElementsByTagName: () => [],
	addEventListener:     () => {},
	releaseEvents:        () => {},
	captureEvents:        () => {},
	},
    console:        console,
    };
sandbox.globalThis = sandbox;

vm.createContext(sandbox);
vm.runInContext(
    fs.readFileSync(HT_RENDER_PATH, 'utf8'),
    sandbox,
    { filename: HT_RENDER_PATH }
);

module.exports = sandbox;
