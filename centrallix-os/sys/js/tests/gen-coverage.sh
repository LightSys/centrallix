#!/bin/sh
# Regenerate lcov.info for the cxjs_* test suite, viewable in VSCode via the
# Coverage Gutters extension by ryanluker.

set -e

# Must run from the repo root so the SF: paths in lcov.info will be
# workspace-relative (a leading ../ breaks file resolution).
root=$(git rev-parse --show-toplevel)
cd "$root"

node --test --experimental-test-coverage \
	--test-reporter=lcov --test-reporter-destination=lcov.info \
	--test-reporter=spec --test-reporter-destination=stdout \
	'centrallix-os/sys/js/tests/*.test.js'

echo "Wrote $root/lcov.info"
