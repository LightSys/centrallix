# GCC Dependencies

Author: Israel Fuller

Date: Descember 4, 2025

## Table of Contents
- [GCC Dependencies](#gcc-dependencies)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#intoduction)
  - [List of Dependencies](#list-of-dependencies)

## Intoduction
This document tracks dependencies on the GCC toolchain in the centrallix codebase.  As code is added which relies on GCC specific behavior, such additions should be noted here to make possible use of a different toolchain (e.g. LLVM) in the future less painful.

## List of Dependencies
- `util.h` & `magic.h`: Use `__typeof__` and `({ ... })` in macros to avoid double-computation.

## Notes
`__FILE__` and `__LINE__` are not dependencies as they were added in C90. See [this page](https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html) for information about predefined macros.
