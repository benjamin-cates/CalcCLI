/*
    This header file has comments on each function and allows for functions in CalcCLI.c to be in any order.
*/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#define CLOCKS_PER_SEC 1
#include <time.h>
//Definitions
#ifndef NULL
    //NULL pointer
    #define NULL ((void *)0)
#endif
#ifndef unit_t
    //Number of units#
    #define unitCount 58
    //Number of immutable functions
    #define immutableFunctions 107
    //Number of optional functions
    #define includeFuncsLen 16
    /*
        Unit type
        {0: meter , 1: kilogram, 2: second, 3: Amp, 4: Kelvin, 5: mole, 6: currency, 7:bits}
        (unit_t>>(0*8))&255 is the length component
        (unit_t>>(1*8))&255 is the mass component
        (unit_t>>(2*8))&255 is the time component
        ...
        unit_t can be converted into an array of 8 signed bytes using this method
        m^2 = 0x02 {2} m^2
        m/s^2 = 0xFE0001 {1,0,-2} m * s^-2
        Joule = 0xFE0102 {2,1,-2} m^2 * kg * s^-2
    */
    #define unit_t unsigned long long
    //Number of metric prefixes
    #define metricCount 18
#endif
#define value_num 0
#define value_vec 1
#define value_func 2
#define value_arb 3
#ifdef __cplusplus
extern "C" {
#endif
#pragma region Structs
/**
 * Complex number with unit
 * @param r Real component
 * @param i Imaginary component
 * @param u Unit component
 */
typedef struct NumberStruct {
    double r;
    double i;
    unit_t u;
} Number;
/**
 * A vector (2D array of Number)
 * total is the total number of elements
 * width and height describe the shape of the vector
 * val[0] is the top left corner
 * val[mat.width-1] is the top right corner
 */
typedef struct VectorStruct {
    Number *val;
    //Width of the vector
    short width;
    //Height of the vector
    short height;
    //Total number of members = width * height
    short total;
} Vector;
/**
 * Arbitrary Precision Number
 * @param mantissa Stores the base-256 digits of the number
 * @param accu Maximum of length
 * @param len Length of the mantissa
 * @param exp Exponent, 0 for integers, 1 for the value 256, -1 for 1/10 ...
 * @param sign 0 for positive numbers, 1 for negative
 */
typedef struct ArbStruct {
    unsigned char* mantissa;
    short accu;
    short len;
    short exp;
    char sign;
} Arb;
/**
 * Number, but with arbitrary precision instead of doubles
 */
typedef struct ArbNumber {
    Arb r;
    Arb i;
    unit_t u;
} ArbNum;
struct TreeStruct;
/**
 * Value that can contain a number or any other type
 * @param type type of value (0 for numbers)
 * @param num Number when type==0
 * @param r Real component when type==0
 * @param i Imaginary component when type==0
 * @param u Unit component type==0
*/
typedef struct ValueStruct {
    int type;
    union {
        Number num;
        struct {
            double r;
            double i;
            unit_t u;
        };
        Vector vec;
        struct {
            struct CodeBlock* code;
            char** argNames;
        };
        ArbNum* numArb;
    };
} Value;
/**
 * Unit standard (ie. ft, m, kg)
 * @param name unit name (ex. ft)
 * @param baseUnits base units of the unit
 * @param multiplier multiplier of the unit compared to metric values
 */
typedef struct UnitStandardStuct {
    const char* name;
    double multiplier;
    unit_t baseUnits;
    //Multiplier is negative when it supports metric prefixes
} unitStandard;
typedef struct CodeBlock CodeBlock;
/**
 * Operation tree. Value and args are in a union, so changing one with change the other.
 * @param op Operation ID
 * @param optype Type of operation ID, 0 for builtin, 1 for custom functions, 2 for function arguments
 * @param value Numeric value, if op==op_val && optype==optype_builtin
 * @param branch Branches of the operation tree (only for functions)
 * @param code CodeBlock for optype==optype_anon
 * @param argCount Number of arguments, if op!=0
 * @param argWidth Width of argument vector (internal, only for vector constructor)
 */
typedef struct TreeStruct {
    int op;
    int optype;
    union {
        Value value;
        struct {
            union {
                struct CodeBlock* code;
                struct TreeStruct* branch;
            };
            short argCount;
            //Only for op_vector
            short argWidth;
            //Only for optype_anon
            char** argNames;
        };
    };
} Tree;
/**
 * Function
 * @param tree Operation tree
 * @param name Name of the function
 * @param argNames List of argument names (ex. a(b,c) will have argNames="bc")
 * @param argCount Number of arguments
 * @param nameLen length of name (used to make searching faster)
 */
/**
 * This describes a line of code in a code block
 * @param id Type of action, see #define action_xxx for types
 * @param localVarID Only for id==2 describes the local variable id
 * @param tree Returns a conditional for: if, while, and for blocks. Returns a value for: return, setVariable, and statement.
 * @param code Code block for: if, else, while, and for.
 */
typedef struct FunctionAction {
    int id;
    int localVarID;
    Tree* tree;
    CodeBlock* code;
} FunctionAction;
/**
 * Describes a return value from a function, only used internally.
 * @param type Type of return: 0: no return, 1: return val, 2: break, 3: continue
 * @param val return value for type==1
 */
typedef struct FunctionReturn {
    int type;
    Value val;
} FunctionReturn;
extern const FunctionReturn return_null;
extern const FunctionReturn return_break;
extern const FunctionReturn return_continue;
/**
 * Describes a grouping of code lines
 * Note: While sub-blocks are defined in the lines of code, they cannot be simply extracted because the sub-lines may reference a local variable in a higher scope. This is why codeBlocks are packaged into Functions.
 * @param list List of lines
 * @param listLen Length of list
 * @param localVariables Names of the local variables that are defined in this scope
 * @param localVarCount Number of local variables defined in this scope
 */
typedef struct CodeBlock {
    FunctionAction* list;
    int listLen;
    int localVarCount;
    char** localVariables;
} CodeBlock;
/**
 * This describes a multiline function
 * @param name Name of the function
 * @param nameLen length of the name, this is to prevent repeated strcmp calls when searching for a function
 * @param args List of the argument names of the function
 * @param argCount Number of arguments required
 * @param code The actual code that composes the function
 */
typedef struct Function {
    char* name;
    char** args;
    int nameLen;
    int argCount;
    CodeBlock code;
} Function;
/**
 * Holds information about a builtin function
 * @param argCount Number of arguments
 * @param nameLen Length of the name, useful for finding function IDs faster
 * @param name Name of the function
 */
struct stdFunction {
    const int argCount;
    const int nameLen;
    const char* name;
};
/**
 * Stores information about an optionally included function
 * Library functions are stored in includeFuncs
 */
struct LibraryFunction {
    char* name;
    char* arguments;
    char* equation;
    int libraryID;
};
#pragma endregion
#pragma region External Variables
//Degree ratio, 1 if radian, pi/180 if degrees
extern double degrat;
//Size of the history array
extern int historySize;
//Number of items in history
extern int historyCount;
//Error has occured
extern bool globalError;
//Global arbitrary precision accuracy
extern int globalAccuracy;
//Global use arbitrary precision
extern bool useArb;
//Global arbitrary precision accuracy in base 10
extern int digitAccuracy;
//Whether to ignore errors or not
extern bool ignoreError;
//Number with r=0, i=0, and u=0
extern Number NULLNUM;
//Value with number 0
extern Value NULLVAL;
//Tree with op=0 and value=0
extern Tree NULLOPERATION;
//Code block that returns zero
extern CodeBlock NULLCODE;
//History array
extern Value* history;
//All predefined units
extern const unitStandard unitList[];
//Number of custom functions
extern int numFunctions;
//Array length of functions
extern int functionArrayLength;
//List of all custom functions (optype_custom)
extern Function* customfunctions;
//List of all standard functions (optype_builtin)
extern const struct stdFunction stdfunctions[immutableFunctions];
//List of local variables in the global scope
extern char** globalLocalVariables;
//Size of globalLocalVariables that is allocate
extern int globalLocalVariableSize;
//List of the values of global local variables
extern Value* globalLocalVariableValues;
//Sorted list of stdfunctions (sorted in startup)
extern int* sortedBuiltin;
//Length of sortedBuiltin
extern int sortedBuiltinLen;
//List of all optional functions (include with -include)
extern struct LibraryFunction includeFuncs[includeFuncsLen];
//Types of includeFuncs (not currently used)
extern const char* includeFuncTypes[];
//Metric prefixes
extern const char metricNums[];
//Metric prefix values
extern const double metricNumValues[];
//Metric base units, used for toStringUnit
extern const char* baseUnits[];
extern const char numberChars[];
//Error message for when malloc returns zero
extern const char* mallocError;
//List of syntax type names
extern const char* syntaxTypes[];
#pragma endregion
//Functions
/**
 * Prints an error message as printf("Error: "+format,message), then sets globalError to true
 * @param format error format
 * @param message char* that is passed to format
 */
void error(const char* format, ...);
#pragma endregion
#pragma region Arbitrary Precision
///General Functions
Arb arbCTR(unsigned char* mant, short len, short exp, char sign, short accu);
Arb copyArb(Arb arb);
void freeArb(Arb arb);
/*
    Returns 1 if one>two, -1 if two>one, or 0 if they are equal
*/
int arbCmp(Arb one, Arb two);
//Trim the zeroes and round up if len is greater than accu
void trimZeroes(Arb* arb);
//Arb conversions
Arb parseArb(char* string, int base, int accu);
double arbToDouble(Arb arb);
char* arbToString(Arb arb, int base, int digitAccuracy);
Arb doubleToArb(double val, int accu);
//Math functions
Arb arb_divModInt(Arb one, unsigned char two, int* carryOut);
Arb multByInt(Arb one, int two);
Arb arb_floor(Arb one);
Arb arb_add(Arb one, Arb two);
Arb arb_subtract(Arb one, Arb two);
Arb arb_mult(Arb one, Arb two);
//Calculate the reciprocal of one
Arb arb_recip(Arb one);
//Calculate pi to accu digits
Arb arb_pi(int accu);
//Calculate e to accu digits
Arb arb_e(int accu);
//Calculate the integer factorial of one
Arb arb_intFact(Arb one);
Arb arb_exp(Arb one);
Arb arb_ln(Arb one);
Arb arb_pow(Arb one, Arb two);
Arb arb_sinh(Arb one);
#pragma endregion
#pragma region Numbers
/**
 * Returns a number made of the three components
 * @param r Real component
 * @param i Imaginary component
 * @param u Unit component
 */
Number newNum(double r, double i, unit_t u);
/**
 * Returns a double parsed from the string
 * @param num Text of the number
 * @param base Base to be parsed in
 */
double parseNumber(const char* num, double base);
/**
 * Returns the string representation of num (ex. 3+4.5i[m])
 * @param num Number to be printed
 * @param base Base to output
 * @return string that must be freed
 */
char* toStringNumber(Number num, double base);
/**
 * Prints the number num as a ratio if it is one
 * Return value must be freed
 */
char* toStringAsRatio(Number num);
/**
 * Returns the string form of num in base base
 * @param num Number to print
 * @param base base to print it in
 * @return string, maximum length of 24, must be freed
 */
char* doubleToString(double num, double base);
/**
 * Appends a number to history
 * @param num Number to append
 * @param base Base to output in
 * @param print Whether to print or return the message
 * @return Returns the message if print is false (return must be freed)
 */
char* appendToHistory(Value num, double base, bool print);
//Returns one+two
Number compAdd(Number one, Number two);
//Returns one-two
Number compSubtract(Number one, Number two);
//Returns one*two
Number compMultiply(Number one, Number two);
//Returns pow(one,two) or one^two
Number compPower(Number one, Number two);
//Returns one/two
Number compDivide(Number one, Number two);
//Returns sin(one)
Number compSine(Number one);
//Returns one-two
Number compSubtract(Number one, Number two);
//Returns sqrt(one)
Number compSqrt(Number one);
/**
 * Returns gamma function of one
 * The gamma function is equal to factorial(one-1)
 * @param one number to pass to the gamma function
 */
Number compGamma(Number one);
/**
 * Returns the trigonometric result of num, based on type
 * @param type Type of operation to run (ex. op_sin)
 * @param num The input into the function
 */
Number compTrig(int type, Number num);
/**
 * Returns the binary operation of one and two, based on type
 * @param type Type of operation to run (ex. op_and)
 * @param one First input
 * @param two Second input
 */
Number compBinOp(int type, Number one, Number two);
#pragma endregion
#pragma region Vectors
/**
 * Return the determinant of a square matrix
 */
Number determinant(Vector vec);
/**
 * Returns the subsection of a vector with the row and column omitted.
 */
Vector subsection(Vector vec, int row, int column);
/**
 * Transpose the matrix (swap x and y coordinates)
 */
Vector transpose(Vector one);
/**
 * Multiply two matrices (order matters)
 */
Vector matMult(Vector one, Vector two);
/**
 * newVecScalar, but returns a Value
 */
Value newValMatScalar(int type, Number scalar);
/**
 * Creates a one-by-one vector and fills it with num
 */
Vector newVecScalar(Number num);
/**
 * Initializes an empty vector with width and height
 */
Vector newVec(short width,short height);
#pragma endregion
#pragma region Values
/**
 * Create a numeral value from r, i, and u
 */
Value newValNum(double r, double i, unit_t u);
/**
 * Frees any array buffers stored in a value (vectors)
 */
void freeValue(Value val);
/**
 * Copies the value if it is a vector
 */
Value copyValue(Value val);
/**
 * Returns the string version of a value. Return char* must be freed.
 */
char* valueToString(Value val, double base);
/**
 * Returns the real component of a value. (top left corner if val is a vector)
 *
 */
double getR(Value val);
/**
 * Returns the Division, Modulus, or Exponent of the two values
 *  This function was created to solve the repetition of num to num, vec to num, num to vec, vec to vec
 * @param function can be &compDivide, &compModulo, or &compPower
 * @param one First value
 * @param two Second value
 * @param type How to handle different sized vectors (0 to take the maximum width for power, 1 to take the minimum width for modulo and divide)
 */
Value DivPowMod(Number(*function)(Number, Number), Value one, Value two, int type);
Value valMult(Value one, Value two);
Value valAdd(Value one, Value two);
Value valDivide(Value one, Value two);
Value valPower(Value one, Value two);
Value valModulo(Value one, Value two);
Value valNegate(Value one);
Value valLn(Value one);
bool valIsEqual(Value one, Value two);
#pragma endregion
#pragma region Units
/**
 * Returns a new unit standard
 * @param name name of the unit (ex. ft)
 * @param mult Multiplier compared to base unit (negative if this unit uses metric prefixes
 * @param units Base units of the unit
 */
unitStandard newUnit(const char* name, double mult, unit_t units);
/**
 * Returns a number with the unit from the unit name
 * @param name Name of the unit
 * @return Number with u=baseUnits and r=multiplier
 */
Number getUnitName(const char* name);
/**
 * Returns the base units or metric unit
 * @param unit base units
 * @return String of the unit
 */
char* toStringUnit(unit_t unit);
/**
 * Returns the interaction of two units based on op
 * @param one first unit
 * @param two second unit
 * @param op can be '+' '*' '/' or '^'
 * @param twor value of two.r, used when op=='^'
 */
unit_t unitInteract(unit_t one, unit_t two, char op, double twor);
#pragma endregion
#pragma region Trees
/**
 * Returns new operator from args and opID
 * @param args Arguments of the operation
 * @param argCount Number of arguments
 * @param op OperatorID
 */
Tree newOp(Tree* args, int argCount, int op, int optype);
/**
 * Returns a new operation with id 0 and value val
 */
Tree newOpValue(Value val);
/**
 * Shortcut for newOpValue(newValue(r,i,u))
 */
Tree newOpVal(double r, double i, unit_t u);
/**
 * Mallocs two arguments and copies them if flags are true
 * @param one first operation
 * @param two second operation
 * @param copyone bool whether to copy one before malloc
 * @param copytwo bool whether to copy two before malloc
 */
Tree* allocArgs(Tree one, Tree two, bool copyone, bool copytwo);
/**
 * Mallocs one argument and returns the pointer, copies if true
 * @param one operation to malloc
 * @param copy whether to copyTree(one)
 */
Tree* allocArg(Tree one, bool copy);
//Operation Tests
/**
 * Returns true if in.op==0 and in.val==0
 * @param in operation to test
 */
bool treeIsZero(Tree in);
/**
 * Returns true if in.op==0 and in.val==1
 * @param in operation to test
 */
bool treeIsOne(Tree in);
/**
 * Returns true if both trees are equal
 */
bool treeEqual(Tree one, Tree two);
/**
 * Frees the op.args and runs freeTree on all arguments. Does not need to be called if the operation has no arguments.
 * @param op Operation to free
 */
void freeTree(Tree op);
/**
 * Copies the tree op with specific rules
 * @param op Operation to copy
 * @param args used only if replaceArgs is true
 * @param unfold whether to replace custom functions with their trees
 * @param replaceArgs whether to replace function arguments with args
 * @param optimize whether to precalculate non-variable branches
 * @return return value must be freeTree()ed
 */
Tree copyTree(Tree tree, const Tree* replaceArgs, int replaceCount,bool unfold);
/**
 * Returns the string form of operation op, output must be free()d
 * @param op operation to print
 * @param bracket whether to wrap output in brackets (default to false)
 * @param args names of the arguments
 * @return string form of op, must be free()d
 */
char* treeToString(Tree op, bool bracket, char** argNames, char** localVars);
/**
 * Computes the operation tree
 * @param op Tree to compute
 * @param args Arguments (only used for functions)
 */
Value computeTree(Tree op, const Value* args, int argLen, Value* localVars);
/**
 * Returns the type of a section and when it ends
 * In the parsing engine, text is first split into secttions, this function will find the end of the section.
 * @param eq Equation to parse
 * @param start Position of the first character in the section
 * @param out Return value, sets the int at the pointer to the final character in the section
 * @param base Base (for things like 10A)
 * @return Section type: {
 *      -1: Section not recognized
 *      0: Number
 *      1: Variable
 *      2: Function call
 *      3: Parenthesis
 *      4: Square bracket
 *      5: Square bracket with underscore notation
 *      6: Operator
 *      7: Vector
 *      8: Anonymous function
 *   }
 */
int nextSection(const char* eq, int start, int* end, int base);
int getCharType(char in, int curType, int base, bool useUnits);
//Takes an equation as an argument and states the position of the equal sign if it is a local variable assignment
int isLocalVariableStatement(const char* eq);
/**
 * Generates an operation tree from an equation
 * @param eq Equation
 * @param argNames list of argument names (only for functions)
 * @param base base to compute in, defaults to 10, if base is not 0, eq will be treated as inside square brackets
 * @return Tree, must be freeTree()ed
 */
Tree generateTree(const char* eq, char** argNames, char** localVars, double base);
/**
 * Returns the derivative of operation
 * @param op Tree to take the derivative of must be copied before passing
 * @return Tree, must be freed with freeTree()
 */
Tree derivative(Tree op);
#pragma endregion
#pragma region Functions
/**
 * Frees an argument list
 */
void freeArgList(char** argList);
/**
 * Get the length of an argument list (number of arguments)
 */
int argListLen(char** argList);
/**
 * Returns of a copy of the argList. Return must be freeArgList()ed.
 */
char** argListCopy(char** argList);
/**
 * Merges two argument lists
 * Warning: only run free on the output, do not run freeArgList on it
*/
char** mergeArgList(char** one, char** two);
/**
 * Returns the string form of an argument list
 *
 *
 */
char* argListToString(char** argList);
/**
 * Returns an argument list from a string
 */
char** parseArgumentList(const char* list);
/*
 *  Appends toAppend to list, expands size of necessary returns the position of toAppend
 * If it encounters an arg that is identical to toAppend, it will return the position of that arg
 *  Usage: int size=5;char** list=calloc(5,sizeof(char**));
 *  int appendPos = argListAppend(&list,{somestring},&size);
 */
int argListAppend(char*** pointerToList, char* toAppend, int* pointerToSize);
/**
 * Runs an anonymous function with the args
 * returns a value
 */
Value runAnonymousFunction(Value val, Value* args);
/**
 * Runs a function with the args and returns a value
 */
Value runFunction(Function func, Value* args);
/**
 * Function constructor
 * @param name name of the function
 * @param tree function tree, NULL if predefined
 * @param argCount number of arguments the function recieves
 * @param argNames list of argument names (null terminated char* array)
 */
Function newFunction(char* name, CodeBlock code, char argCount, char** argNames);
/**
 * Returns the function ID
 * @param name name of the function
 * @return function ID of name, returns 0 if not found
 */
Tree findFunction(const char* name, bool useUnits, char** arguments, char** localVariables);
/**
 * Parses the equation eq and adds it to the function list
 * @param eq Equation (ex. "f(a)=a^2")
 */
void generateFunction(const char* eq);
/**
 * Deletes the custom function with id
 */
void deleteCustomFunction(int id);
/*
 * Add a global local variable
 */
void appendGlobalLocalVariable(char* name, Value value);
#pragma region CodeBlocks
/**
 * Creates a new code block that just returns tree
 * @param tree The return expression for the code block
 * @return must be freed
 */
CodeBlock codeBlockFromTree(Tree tree);
/**
 * Parses a code block from an equation and arguments, the final three inputs should be null
 * @param eq expression to parse. The expession should not include the wrapping brackets ("{" and "}")
 * @param args function argument to use
 * The final three inputs keep track of the stack, leave them at NULL
 */
CodeBlock parseToCodeBlock(const char* eq, char** args, char*** localVars, int* localVarSize, int* localVarCount);
/**
 * Runs a code block, this function needs to be used carefully
 * Calling:
 *     int localVarCount=0;
 *     int localVarSize=1;
 *     Value* localVars=calloc(1,sizeof(Value));
 *     runCodeBlock(block,args,argsCount,&localVars,&localVarCount,&localVarSize);
 */
FunctionReturn runCodeBlock(CodeBlock func, Value* arguments, int argCount, Value** localVars, int* localVarCount, int* localVarSize);
/**
 * Copies a code block, replacing the first replaceCount arguments with replace args
 * Arg replace example: copyCodeBlock(arg1=>{return arg1+arg0;},[2],1,false) = arg0=>{return arg0+2;}
 * @param code code to copy
 * @param replaceArgs what to replace the arguments with
 * @param replaceCount number of arguments to replace
 * @param unfold whether to remove references to custom functions
 */
CodeBlock copyCodeBlock(CodeBlock code, const Tree* replaceArgs, int replaceCount, bool unfold);
/**
 * Frees a codeblock, including all of the trees and code blocks within the function actions
 */
void freeCodeBlock(CodeBlock code);
/**
 * Returns a code in the form of a string, using localVariables and arguments because they are not builtin to the codeBlock struct
 */
char* codeBlockToString(CodeBlock code, char** localVariables, char** arguments);
#pragma endregion
#define action_statement 0
#define action_return 1
#define action_localvar 2
#define action_if 3
#define action_else 4
#define action_while 5
#define action_for 6
#define action_break 7
#define action_continue 8
#pragma endregion
/**
 * Prints graph of equation to stdout
 * @param equation Equation to print
 * @param left left side of the graph
 * @param right right side of the graph
 * @param top top side of the graph
 * @param bottom bottom side of the graph
 * @param rows number of rows (changes scale)
 * @param columns number of columns (changes scale)
*/
void graphEquation(const char* equation, double left, double right, double top, double bottom, int rows, int columns);
/**
 * Cleans input, generates tree, and computes it
 * @param eq Equation
 * @param base Base to calculate in
 */
Value calculate(const char* eq, double base);
/**
 * Frees units, functions, and history. Must be run after every call of startup() to prevent leaks
 */
void cleanup();
/**
 * Initializes units, functions, and history
 */
void startup();
//User functions
/**
 * Runs input
 * @param input The line to run
 */
void runLine(char* input);
/**
 * Removes spaces from the input. Note: this will alter the contents of input
 * @param input after the function is called, all spaces will have been removed
 */
void inputClean(char* input);
/**
 * Returns whether string starts with sw
 */
bool startsWith(const char* string, const char* sw);
/**
 * Basic command running, returns a string with the command output
 * Returns NULL if the command is not recognized
 * Return value must be freed
 */
char* runCommand(char* input);
#pragma region Misc
int* primeFactors(int num);
bool isPrime(int num);
void getRatio(double num, int* numerOut, int* denomOut);
/**
 * Print a double as a ratio
 * Return value must be freed
 */
char* printRatio(double in, bool forceSign);
/*
    Fills out with the color codes for eq
    Note: out must be a string with an allocated length greater than or equal to eq
    "eq" is not const to do some null termination tricks, but the contents of eq will not be different after the function is complete.
*/
void highlightSyntax(char* eq, char* out, char** args, char** localVars, int base, bool useUnits);
/**
 * Returns an allocated string containing the color information for a line
 * This will highlight commands and comments properly
 * Return value must be freed
 */
char* highlightLine(char* eq);
/**
 * Highlights a code block
 * Note: if eq is wrapped by { and }, it will not work
 * Note: out must be a string with an allocated length greater than or equal to eq
 */
void highlightCodeBlock(char* eq, char* out, char** args, char** localVars);
/**
 * highlights an argument list and returns the position of the terminating equal sign
 *
 */
int highlightArgumentList(char* eq, char* out);
#pragma endregion
#ifdef __cplusplus
}
#endif
#pragma region Operation IDs
//These definitons are to ease readability of the program and to allow flexibility
#define optype_builtin 0
#define optype_custom 1
#define optype_argument 2
#define optype_anon 3
#define optype_localvar 4
#define op_val 0
#define op_i 1
#define op_neg 2
#define op_pow 3
#define op_mod 4
#define op_mult 5
#define op_div 6
#define op_add 7
#define op_sub 8
#define op_sin 12
#define op_cos 13
#define op_tan 14
#define op_csc 15
#define op_sec 16
#define op_cot 17
#define op_sinh 18
#define op_cosh 19
#define op_tanh 20
#define op_asin 21
#define op_acos 22
#define op_atan 23
#define op_acsc 24
#define op_asec 25
#define op_acot 26
#define op_asinh 27
#define op_acosh 28
#define op_atanh 29
#define op_sqrt 32
#define op_cbrt 33
#define op_exp 34
#define op_ln 35
#define op_logten 36
#define op_log 37
#define op_fact 38
#define op_sgn 43
#define op_abs 44
#define op_arg 45
#define op_round 47
#define op_floor 48
#define op_ceil 49
#define op_getr 50
#define op_geti 51
#define op_getu 52
#define op_grthan 53
#define op_equal 54
#define op_min 55
#define op_max 56
#define op_lerp 57
#define op_dist 58
#define op_not 65
#define op_and 67
#define op_or 68
#define op_xor 69
#define op_ls 70
#define op_rs 71
#define op_pi 75
#define op_phi 76
#define op_e 77
#define op_ans 84
#define op_hist 85
#define op_histnum 86
#define op_rand 87
#define op_run 90
#define op_sum 91
#define op_product 92
#define op_vector 96
#define op_width 97
#define op_height 98
#define op_length 99
#define op_ge 100
#define op_fill 101
#define op_map 102
#define op_det 103
#define op_transpose 104
#define op_mat_mult 105
#define op_mat_inv 106
#pragma endregion