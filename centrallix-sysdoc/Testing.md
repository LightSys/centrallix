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
    - [Running Test-obj Tests](#running-test-obj-tests)
    - [Test Case Format: Standard](#test-case-format-standard)
    - [Test Case Format: Native C](#test-case-format-native-c)
    - [More About Test-obj Tests](#more-about-test-obj-tests)
  - [Centrallix-lib Tests](#centrallix-lib-tests)
    - [Running Centrallix-lib Tests](#running-centrallix-lib-tests)
    - [Understanding Output](#understanding-output)
    - [Centrallix-lib Test Case Format](#centrallix-lib-test-case-format)
    - [Code Coverage](#code-coverage)
    - [More About Centrallix-lib Tests](#more-about-centrallix-lib-tests)
  - [Selenium UI Tests](#selenium-ui-tests)
    - [Running Selenium UI Tests](#running-selenium-ui-tests)
    - [Selenium Test Case Format](#selenium-test-case-format)
    - [More About Selenium UI Tests](#more-about-selenium-ui-tests)


## Test-obj Tests
The test-obj test suite uses `test_obj` to run commands (including SQL queries using the `query` or `csv` commands) and compares the output against the "correct" output specified for the test.  It detects failures, crashes, and lockups.  This test suite does not measure performance.

### Running Test-obj Tests
After configuring and building `centrallix` and `centrallix-lib`, run the test-obj test suite by using `make test` in the `centrallix` directory. When running tests with `make test`, both standard and native C tests will be run together (see below for info on test case formats), and results will be reported together.

To run only a subset of tests, set `TONLY` to the desired test category prefix.  For example, either of the following will run only object system driver tests:

```sh
export TONLY=objdrv
make test
```

```sh
make test TONLY=objdrv
```

The `00baseline` tests are special sanity checks for the test harness itself.  They are intended to demonstrate and validate the suite's ability to report passing, failing, crashing, and lockup behavior.

### Test Case Format: Standard
Each test is composed of two or more files in the `centrallix/tests` directory, and each test has a category name (e.g. `objdrv_cluster` for testing the cluster driver) and a test case number (incrementing from `00`), which are used for naming test files.  The first test file is the `test_{category}_NN.to` script file which specifies a list of commands to be run in test-obj, each on their own line.  Lines starting with a `#` are treated as comments.  By convention, the first line of this file should be `##NAME <Test Name>`.  The second is a `test_{category}_NN.cmp` file, which lists the expected output from running the specified commands.  Additional files used by the test (e.g. file accessed by the `.to` script file) should be placed in `centrallix-os/tests` so that they will be accessible to the object system from the script.  These files should follow the same naming convention as the test that uses them to avoid confusion.
When tests are run, the results are saved in a corresponding `.out` file of the same name as the test.

A common strategy is to create a `.to` file and a blank `.cmp` file, then run the test and copy the `.out` file to the `.cmp`.  When doing so, the developer should *carefully* verify that every part of the resulting `.cmp` file represents a correct output.  In general, writing `.cmp` files "by hand" is safer since it this makes differences between what the developer expects and what the program does (aka. what we want the test to detect) far easier to notice.  However, copying the `.out` file can be much faster, and is usually safe if done carefully.

### Test Case Format: Native C
The TestObj test suite also supports writing tests in native C by creating `test_{category}_NN.c` in the same directory.  This file should implement `long long test(char** name)`, which returns 0 if the test passes and returns a negative value *or* aborts using assert() (or something similar) if it fails.  If the test is skipped (e.g. an integration test using the Sybase object-system driver that automatically skips when Sybase isn't enabled), return 1.  The `name` parameter should be set to the name of the test (e.g. "test_obj native C test for cluster driver").  These tests have no associated `.to`, `.cmp`, or `.out` files.

### More About Test-obj Tests
For more information, see [centrallix/tests/README](../centrallix/tests/README).


## Centrallix-lib Tests
The `centrallix-lib` project has its own native C regression and microbenchmark-style test suite in `centrallix-lib/tests`.  Each test is compiled as a standalone binary and linked with a shared test driver (`tests/t_driver.c`) and binaries generated for every `.c` file in centrallix-lib.  The driver runs the test inside the mtask environment, reports pass/fail/crash/abort/lockup status, and prints a simple operations-per-second figure based on the iteration count returned by the test.

### Running Centrallix-lib Tests
After configuring and building `centrallix-lib`, run this test suite by using `make test` in the `centrallix-lib` directory.

To run only a subset of tests, set `TONLY` to the desired test category prefix.  For example, either of the following will run only lexer tests:

```sh
export TONLY=mtlexer
make test
```

```sh
make test TONLY=mtlexer
```

There is also a `make valtest` target, which runs the same compiled test programs under Valgrind instead of running them directly.

The `00baseline` tests are special sanity checks for the test harness itself.  They are intended to demonstrate and validate the suite's ability to report passing, failing, crashing, and lockup behavior.

### Understanding Output
The `make test` output begins with a two-column header:

- `Test Name`: the descriptive name provided by the test itself.
- `Stat Ops/sec`: an approximate operations-per-second figure computed from the test's returned iteration count and runtime.

When a test finishes, the driver prints one of the following statuses:

- `PASS`: the test completed successfully.
- `FAIL`: the test returned a negative value.
- `CRASH`: the test triggered `SIGSEGV`.
- `ABORT`: the test triggered `SIGABRT`, which commonly happens when an `assert()` fails.
- `LOCKUP`: the test exceeded the driver's 10-second alarm timeout.

Because the driver computes performance from the test's returned iteration count, tests should execute enough iterations that runtime is measurable.  Otherwise, the calculated throughput may be misleading or may even fail due to division by zero.

### Code Coverage
The `centrallix-lib` test suite also supports gcov/lcov coverage collection.

To enable coverage:

1. Re-run `./configure` in `centrallix-lib` with `--enable-coverage`.
2. Run `make clean` to remove previously compiled objects.
3. Run `make test` to regenerate the test binaries and execute them with coverage instrumentation.

This produces `.gcda` and `.gcno` files that can be processed with tools such as `lcov`.  The `make cov-clean` target removes accumulated coverage artifacts (`*.gcov`, `*.gcda`, `*.gcno`, and `lcov.info`).

As with the normal test target, `TONLY` can be used to focus coverage on one category of tests.  Be aware that coverage data accumulates across runs until `make cov-clean` is used.

Note: In my own testing, I could not get coverage to work. I was able to generate a `lcov.info` file using lcov but the VSCode extension would not detect it.

### Centrallix-lib Test Case Format
Each test is a C source file in `centrallix-lib/tests` named `test_{category}_{NN}.c` (for example `test_qprintf_12.c` or `test_mtlexer_05.c`).  There are also a few category-wide baseline tests such as `test_00baseline.c`.

Each test file should implement:

```c
long long test(char **name)
```

The test should:

- Set `*name` to a descriptive test name that will appear in the output.
- Perform whatever setup is needed, including initializing any required Centrallix-Lib subsystems.
- Execute the behavior being tested in a loop so the suite can report performance.
- Return the total number of logical operations performed if the test passes.
- Return a negative value to report failure, or abort (for example via `assert()`).

Tests may include and use any `centrallix-lib` headers and code.  Some tests also use companion data files kept in the same directory, such as `test_mtlexer_05.txt`, which are opened relative to the `centrallix-lib` directory when the suite runs.

### More About Centrallix-lib Tests
For more information, see [centrallix-lib/tests/README](../centrallix-lib/tests/README).


## Selenium UI Tests
The `centrallix-ui-test` project contains browser-based UI tests written as standalone Python scripts using Selenium and ChromeDriver.  These tests exercise UI applications in `centrallix-os/tests/ui` by loading them in a browser, performing interactions, and reporting pass/fail status based on the observed behavior.

### Running Selenium UI Tests
Before running these tests, make sure a Centrallix server is running and serving the UI test applications (check this in `kardia.sh`).  Then, from the `centrallix-ui-test` directory:

1. Install the Python dependencies:

```sh
python3 -m pip install -r requirements.txt
```

2. Create a `config.toml` file in `centrallix-ui-test` containing the base URL for the test server, for example:

```toml
url = "https://user:password@localhost:8080"
```

3. Run an individual test script directly with Python, for example:

```sh
python3 tests/button_test.py
```

There is no single command in the repository for running the entire Selenium suite at once.  Each test is run individually by executing its corresponding script in `centrallix-ui-test/tests`.

<!-- TODO: Israel - Think about adding a command to run all tests. -->

### Selenium Test Case Format
Each Selenium test is a Python source file in `centrallix-ui-test/tests` named `{component}_test.py` (for example `button_test.py` or `form_test.py`).

Each test script should:

- Load the base test server URL from `config.toml`.
- Construct the URL for the corresponding UI test application in `centrallix-os/tests/ui`.
- Launch a Chrome browser using Selenium and ChromeDriver.
- Perform the interactions and assertions needed for the component under test.
- Exit with status code 0 if the test passes and a nonzero status code if it fails.

Shared output formatting helpers may be implemented directly in the script or by importing helpers such as `test_reporter.py`.

### More About Selenium UI Tests
For a high-level summary of current Selenium UI test coverage, see [UITestCoverage.md](./UITestCoverage.md).
