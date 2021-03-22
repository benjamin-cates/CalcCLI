The CalcCLI source code is split into several different files under the `src/` directory. The aim of this guide is to show what code belongs where.

## arb.c
`arb.c` deals solely with arbitrary precision numbers. It controls parsing, printing, and computation.

## command.c
`command.c` deals with the "meta" level of the program, meaning commands and history. It contains the several hundred line function `runCommand` that controls all builtin commands. Although in different implementations of this program, different commands can be added in a wrapper.

## compute.c
`compute.c` deals solely with computing trees and numbers. It contains all of the code that deals with vectors, all of the code for complex number operations, and all of `computeTree`. The only computation that is not in `compute.c` belongs in `arb.c`.

## functions.c
`functions.c` deals with anonymous functions, multiline functions, includeable functions, and local variables.

## general.c
`general.c` contains much of the commonly used code, including: `startup` and `cleanup`, constructors, freers, copiers, arguments lists, global variables, and commonly used functions like `calculate` and `recalloc`.

## highlight.c
`highlight.c` is solely composed of functions that return a color map in exchange for an expression. Some functions return an allocated string, and others accept a pointer to an empty string that is equal in length to the expression.

## misc.c
`misc.c` contains miscellaneous code like `derivative`, factoring functions, and `getRatio`.

## parser.c
`parser.c` contains `generateTree` and code frequently used by both `generateTree` and `highlight.c`.

## print.c
`print.c` contains code that converts structs into text form. All of these functions return an allocated string, since the length of the output cannot easily be predetermined.

## units.c
`units.c` contains unit constants (like the list of metric prefixes) and `unitInteract`