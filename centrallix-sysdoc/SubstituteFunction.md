# Substitute Function Formats

Date: 20-Jun-2012

Author: Greg Beeley (GRB)

## Overview
The substitute() function, available in Centrallix SQL both on the server and on the client, allows for the dynamic formatting of data elements into a string.

This functionality is used for custom formatting of names, addresses, and potentially other related information.

This document specifies the format used by substitute().

## Table of Contents
- [Substitute Function Formats](#substitute-function-formats)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [Syntax](#syntax)
  - [Format](#format)
    - [A.	Simple Attribute Reference](#asimple-attribute-reference)
    - [B.	Object and Attribute Reference](#bobject-and-attribute-reference)
    - [C.	Conditional Formatting if Attribute is Set](#cconditional-formatting-if-attribute-is-set)
    - [D.	Conditional Formatting if Any Attribute is Set](#dconditional-formatting-if-any-attribute-is-set)
    - [E.	Conditional Formatting if Attribute is Unset](#econditional-formatting-if-attribute-is-unset)
    - [F.	Conditional Formatting if Attribute has a Certain Value](#fconditional-formatting-if-attribute-has-a-certain-value)
    - [G.	Shortened ending tag.](#gshortened-ending-tag)

## Syntax
`strval = substitute(format [, objectmap])`

- 'strval' is the resulting formatted string.
- 'format' is the formatting string, as specified by this document.
- 'objectmap' is a string specifying available object names to be used.

## Format
The format string can contain arbitrary text as well as a series of placeholders that specify data to be inserted or conditional formatting.

### A.	Simple Attribute Reference
`[:attrname]`

### B.	Object and Attribute Reference
`[:objname:attrname]`

### C.	Conditional Formatting if Attribute is Set
`[+:attrname]Conditional Text goes Here[/+:attrname]`

`[+:objname:attrname]Conditional Text goes Here[/+:objname:attrname]`

These conditionals trigger IF ALL of the following are TRUE:

1.	Attribute is NOT NULL
2.	Attribute is not a string, or if a string, is non-empty.
3.	Attribute is not an integer, or if an integer, is non-zero.

A string is considered "empty" if it has a zero length when trimmed of leading and trailing spaces.

### D.	Conditional Formatting if Any Attribute is Set
`[+:firstattr,:secondattr]Text[/+:firstattr,:secondattr]`

These conditionals are like simple conditional formatting when an attribute is set, but this allows formatting the text if ANY of the comma-separate attributes (or :objname:attrname) are set.

To do conditional formatting when ALL of a set of attributes are set, just nest the single-attribute version of this format.

### E.	Conditional Formatting if Attribute is Unset
`[-:attrname]Conditional Text goes Here[/-:attrname]`

`[-:objname:attrname]Conditional Text goes Here[/-:objname:attrname]`

These conditionals trigger IF ANY of the following are TRUE:

1.	Attribute is NULL
2.	Attribute is a string and is empty
3.	Attribute is an integer and is zero

A string is considered "empty" if it has a zero length when trimmed of leading and trailing spaces.

### F.	Conditional Formatting if Attribute has a Certain Value
`[?:attrname=Value]Conditional Text[/?:attrname=Value]`

`[?:objname:attrname=Value]Conditional Text[/?:objname:attrname=Value]`

These conditionals trigger if the attribute is not null and has the given value.

### G.	Shortened ending tag.
The full ending tag may be used for clarity, but the ending tag may also be shortened to just `[/]`

Or, shortened to any of the below, based on the type of tag being used:

- `[/+]`
- `[/-]`
- `[/?]`
