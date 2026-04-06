# Centrallix Testing

Author:    Israel Fuller

Date:      April 4th, 2026

License:   Copyright (C) 2026 LightSys Technology Services.  See LICENSE.txt.


## Overview
Centrallix has various sets of tests and test suites for running them.  This file contains a complete list of all such testing and instructions for how to run tests.  When making changes in Centrallix, this file may be helpful to ensure that all relevant tests have been run before committing code or putting it up for review.
<!-- TODO: Greg - Above, I've claimed that this file represents a COMPLETE list of the tests in Centrallix. Could you verify that and let me know if there's any test suites lying around that I forgot to include? -->


## Table of Contents
- [Centrallix Testing](#centrallix-testing)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [Test-obj Tests](#test-obj-tests)
    - [Running Tests](#running-tests)
    - [Test Case Format: Standard](#test-case-format-standard)
    - [Test Case Format: Native C](#test-case-format-native-c)
    - [More Information](#more-information)


## Test-obj Tests
The test-obj test suite uses `test_obj` to run commands (including SQL queries using the `query` or `csv` commands) and compares the output against the "correct" output specified for the test.  It detects failures, crashes, and lockups.  This test suite does not measure performance.

### Running Tests
After configuring and building centrallix and centrallix-lib, run the test-obj test suite by using `make test` in the `centrallix` directory.  You may set `TONLY` to restrict tests (e.g. `export TONLY=objdrv` to specify that only tests for object drivers should be run).  When running tests with `make test`, both standard and native C tests will be run together (see below for info on test case formats), and results will be reported together. 

### Test Case Format: Standard
Each test is composed of two or more files in the `centrallix/tests` directory, and each test has a category name (e.g. `objdrv_cluster` for testing the cluster driver) and a test case number (incrementing from `00`), which are used for naming test files.  The first test file is the `test_{category}_NN.to` script file which specifies a list of commands to be run in test-obj, each on their own line.  Lines starting with a `#` are treated as comments.  By convention, the first line of this file should be `##NAME <Test Name>`.  The second is a `test_{category}_NN.cmp` file, which lists the expected output from running the specified commands.  Additional files used by the test (e.g. file accessed by the `.to` script file) should be placed in `centrallix-os/tests` so that they will be accessible to the object system from the script.  These files should follow the same naming convention as the test that uses them to avoid confusion.
When tests are run, the results are saved in a corresponding `.out` file of the same name as the test.

A common strategy is to create a `.to` file and a blank `.cmp` file, then run the test and copy the `.out` file to the `.cmp`.  When doing so, the developer should *carefully* verify that every part of the resulting `.cmp` file represents a correct output.  In general, writing `.cmp` files "by hand" is safer since it this makes differences between what the developer expects and what the program does (aka. what we want the test to detect) far easier to notice.  However, copying the `.out` file can be much faster, and is usually safe if done carefully.

### Test Case Format: Native C
The TestObj test suite also supports writing tests in native C by creating `test_{category}_NN.c` in the same directory.  This file should implement `long long test(**char name)`, which returns 0 if the test passes and returns a negative value *or* aborts using assert() (or something similar) if it fails.  If the test is skipped (e.g. an integration test using the Sybase object-system driver that automatically skips when Sybase isn't enabled), return 1.  The `name` parameter should be set to the name of the test (e.g. "test_obj native C test for cluster driver").  These tests have no associated `.to`, `.cmp`, or `.out` files.

### More Information
For more information, see [centrallix/tests/README](../centrallix/tests/README).
