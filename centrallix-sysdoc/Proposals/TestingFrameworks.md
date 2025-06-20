# Thoughts on Third-Party Testing Frameworks in Centrallix
Author: Alison Blomenberg

Date: May 2022

## Current testing framework
Unique features:
- Allows for directly testing Centrallix SQL as well as Centrallix C functions

Pain points:
- Fragile to maintain; much of the testing logic is bash buried in a Makefile
- No tools for mocking/faking dependencies to allow for more isolated unit tests (that don't also inadvertently test their dependencies)
- Not designed to integrate with automated continuous integration
- Tests are all in one very large folder

## Third-party testing frameworks

### C testing frameworks I looked at
- [Googletest](https://google.github.io/googletest/) - C++, lots of macros
- [CppUTest](https://cpputest.github.io/manual.html) - C++, probably the most relevant to Centrallix
- [Ceedling/Cmock/Unity](http://www.throwtheswitch.org/) - Ruby dependency, opinionated about project setup

### Available features
- Mocking/faking dependencies
- Tools for discovering and running tests, including grouping tests, having shared setup + teardown methods, different test folders, etc
- Assertion library
- Integrations with automated continuous integration
- C++ support
- Memory leak detection tooling

### Downsides
- Have to significantly rework Centrallix's build system to integrate any sort of mocking
- Either have to rewrite existing C tests to use new testing framework, or have multiple testing frameworks being used at once
- How would these tools interface with the Centrallix SQL tests? Could you still run them all with one command like you can currently without extra crufty scripting on top?
- Several tools use C++, which introduces some extra complexity and a learning curve
- Several tools extensively use macros, which are also potentially confusing and hide implementation details
- Some tools are opinionated about project setup (folder structure, etc)

## Analysis
In my opinion, the most significant reason to introduce a third-party testing framework would be to support mocks/fakes, but this is not doable without significantly reworking the Centrallix build system. This is a fundamental C build problem, not something a testing tool can easily fix. Essentially, once you've linked in all the Centrallix libraries necessary to get even a simple unit test working, you cannot redefine (i.e. mock or fake) any of the functions in those libraries. There are plenty of tools out there for generating mocks, but they all require the original definitions to not be linked in, and the current Centrallix build system would make it very difficult to only selectively link some dependencies. Moreover, since Centrallix doesn't currently have extensive testing, more integration-type tests get you more "bang for your buck" than highly isolated unit tests anyhow.

Besides mocking/faking, most of the other features a third-party testing framework would provide are just basic quality of life things (like being able to group tests or use pretty assertions). I don't think those are worth the work of integrating a third-party framework into Centrallix, having to rewrite a lot of tests, and losing the Centrallix SQL direct test system (which is easy to write tests in, if not to maintain).

I recommend just focusing on writing more tests for Centrallix and incrementally improving the current homegrown testing framework as needed.
