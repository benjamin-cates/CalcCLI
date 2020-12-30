# Description of the data structures of CalcCLI
Most structures can be found near the top of [Calc.h](../Calc.h).

## Arb

Arb is an arbitrary precision data type, it stores one number.

Members:
- `mantissa` - The dynamically-allocated, base-256 mantissa of the number (unsigned char*)
- `len` - The actual length of the current mantissa (short)
- `accu` - The maximum length of the mantissa (short)
- `exp` - The exponent of the value (base-256), increasing by one multiplies the value by 256. (short)
- `sign` - 0 if it's negative, 1 if it's positive. (char)


## Number
Number contains three components: a real component, an imaginary component, and a unit. The real and imaginary components are both 64-bit doubles, and the unit is a 64-bit long composed of 8 different numbers. It has a size of 24 bytes.

The members are:
- `r` - Real component (double)
- `i` - Imaginary component (double)
- `u` - Unit (unsigned long long)

## ArbNum
See [Number], this is identical to Number, except it uses arbitrary precision types instead of floating-point doubles.

## Vector
A vector is a two dimensional array of numbers. In reality, this array is one dimensional, and has a length of `vec.total`. It has a size of

The members are:
- `val` - Array of number components (Number*)
- `width` - Width of the array (short)
- `height` - Height of the array (short)
- `total` - Total number of elements (short)

## Value
The **Value** struct can store either: a number, a vector, or an anonymous function.

Always present members:
- `type` - Type of value (number, vector, or function) (int).

Members for type == value_num:
- `num` - A number (Number)
- `r` - shortcut for num.r (double)
- `i` - shortcut for num.i (double)
- `u` - shortcut for num.u (unsigned long long)

Members for type == value_vec:
- `vec` - A vector (Vector)
- `vec.val`, `vec.width`, `vec.height`, and `vec.total` are therefore present.

Members for type == value_func:
- `tree` - Pointer to the tree to compute (Tree*) (run with `computeTree(val.tree[0],...`)
- `argNames` - list of argument names (char**) (used when printing the value)

Members for type == value_arb:
- `numArb` - Pointer to a ArbNum type. The pointer saves space so that sizeof(Value) does not increase. (ArbNum*)

## unitStandard
This struct is not used in most of the code, it only describes the structure for the constant array `unitList`.

Members:
- `name` - Name of the unit (char*)
- `multiplier` - Offset from the base unit (negative when it supports metric prefixes) (double)
- `baseUnits` - Base units of this unit (unsigned long long)

## Tree
This structure is arguably the most important of the whole program. **Tree** contains an operation ID (function ID), and a list of subtrees.

### Always present members:
- `optype` - Type of operation: builtin, custom, function, or argument (int)
- `op` - Actual operation ID (int)

### When optype == builtin and op == op_val
- `value` - The value to return (Value)
This is the default tree, and the default value is 0. computeTree in this situation will directly return Value.

### When optype == builtin or optype == custom
- `branch` - List of subtrees (Tree*)
- `argCount` - Number of branches (short)
- `argWidth` - Width of the arguments, only used when op == op_vector (short)

If you run generateTree("3+4"), the resulting tree.branch[0] will be 3, and tree.branch[1] will be 4.

### When optype == anon
- `branch` - Pointer to the tree.
- `argCount` - Number of arguments that the return value will have
- `argWidth` - Number of arguments that surround the function
- `argNames` - Names of the arguments

If you run `-def a(x)=y=>(x*y)`, then both argCount and argWidth will be 1, argNames will be {"y"}, and branch will be `arg0*arg1`. When this function is run, `arg0` will be replaced with the value of `x`, and `arg1` will become `arg0`.

### When optype == argument
None of the values in the union are used, only `op`. In the case of `-def j(x,y)=x*y`, x will become a tree with optype = argument, and op = 0. `y` will be the same, but with op = 1.

##