/*
    This header file has comments on each function and allows for functions in CalcCLI.c to be in any order.
*/
#ifdef __cplusplus
extern "C" {
#endif
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
    //NULL pointer with val
    #define NULL ((void *)0)
#endif
#ifndef unit_t
    //Number of units#
    #define unitCount 59
    //Number of immutable functions
    #define immutableFunctions 65
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
    #define metricCount 17
#endif
#define value_num 0
//Structs
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
    };
} Value;
/**
 * Unit standard (ie. ft, m, kg)
 * @param name unit name (ex. ft)
 * @param baseUnits base units of the unit
 * @param multiplier multiplier of the unit compared to metric values
 */
typedef struct UnitStandardStuct {
    char* name;
    double multiplier;
    unit_t baseUnits;
    //Multiplier is negative when it supports metric prefixes
} unitStandard;
/**
 * Operation tree. Value and args are in a union, so changing one with change the other.
 * @param op Operation ID (0 means value)
 * @param value Numeric value, if op==0
 * @param args Arguments, if op!=0
 * @param argCount Number of arguments, if op!=0
 */
typedef struct TreeStruct {
    int op;
    union {
        Value value;
        struct {
            struct TreeStruct* branch;
            char argCount;
        };
    };
} Tree;
/**
 * Function
 * @param tree Operation tree with the first argument having opID=-1, NULL if it is a built-in function
 * @param name Name of the function
 * @param argCount Number of arguments
 * @param nameLen length of name (used to make searching faster)
 */
typedef struct FunctionStruct {
    Tree* tree;
    char* name;
    char* argNames;
    char argCount;
    char nameLen;
} Function;
//Degree ratio, 1 if radian, pi/180 if degrees
extern double degrat;
//Size of the history array
extern int historySize;
//Number of items in history
extern int historyCount;
//Use verbosity
extern bool verbose;
//Error has occured
extern bool globalError;
//Number with r=0, i=0, and u=0
extern Number NULLNUM;
//Value with number 0
extern Value NULLVAL;
//History array
extern Value* history;
//All predefined units
extern const unitStandard unitList[];
//Tree with op=0 and value=0
extern Tree NULLOPERATION;
//Number of functions
extern int numFunctions;
//Array length of functions
extern int functionArrayLength;
//List of all functions (operation IDs)
extern Function* functions;
//Metric prefixes
extern const char metricNums[];
//Metric prefix values
extern const double metricNumValues[];
//Metric base units, used for toStringUnit
extern const char* baseUnits[];
extern const char numberChars[];
//Functions
/**
 * Prints an error message as printf("Error: "+format,message), then sets globalError to true
 * @param format error format
 * @param message char* that is passed to format
 */
void error(char* format, char* message);
//Numbers
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
double parseNumber(char* num, double base);
/**
 * Returns the string representation of num (ex. 3+4.5i[m])
 * @param num Number to be printed
 * @param base Base to output
 */
char* toStringNumber(Number num, double base);
/**
 * Returns the string form of num in base base
 * @param num Number to print
 * @param base base to print it in
 * @return string, maximum length of 24
 */
char* doubleToString(double num, double base);
/**
 * Appends a number to history
 * @param num Number to append
 * @param base Base to output in
 */
void appendToHistory(Value num, double base, bool print);
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
//Values
/**
 * Create a numeral value from r, i, and u
 */
Value newValNum(double r, double i, unit_t u);
/**
 * Frees any array buffers stored in a value (vectors or matrices only.
 */
void freeValue(Value val);
/**
 * Copies the value if it is a matrix of vector.
 */
Value copyValue(Value val);
/**
 * Returns the string version of a value. Return char* must be freed.
 */
char* valueToString(Value val, double base);
Value valMult(Value one, Value two);
Value valAdd(Value one, Value two);
Value valDivide(Value one, Value two);
Value valPower(Value one, Value two);
Value valModulo(Value one, Value two);
Value valNegate(Value one);
Value valLn(Value one);
bool valIsEqual(Value one, Value two);
//Units
/**
 * Returns a new unit standard
 * @param name name of the unit (ex. ft)
 * @param mult Multiplier compared to base unit (negative if this unit uses metric prefixes
 * @param units Base units of the unit
 */
unitStandard newUnit(char* name, double mult, unit_t units);
/**
 * Returns a number with the unit from the unit name
 * @param name Name of the unit
 * @return Number with u=baseUnits and r=multiplier
 */
Number getUnitName(char* name);
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
//Tree Constructors
/**
 * Returns new operator from args and opID
 * @param args Arguments of the operation
 * @param argCount Number of arguments
 * @param op OperatorID
 */
Tree newOp(Tree* args, int argCount, int op);
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
 * @param copy whether to treeCopy(one)
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
//Functions
/**
 * Function constructor
 * @param name name of the function
 * @param tree function tree, NULL if predefined
 * @param argCount number of arguments the function recieves
 */
Function newFunction(char* name, Tree* tree, char argCount, char* argNames);
/**
 * Returns the function ID
 * @param name name of the function
 * @return function ID of name, returns 0 if not found
 */
int findFunction(char* name);
/**
 * Parses the equation eq and adds it to the function list
 * @param eq Equation (ex. "f(a)=a^2")
 */
void generateFunction(char* eq);
//Operation Tree
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
Tree treeCopy(Tree op, Tree* args, bool unfold, bool replaceArgs, bool optimize);
/**
 * Returns the string form of operation op, output must be free()d
 * @param op operation to print
 * @param bracket whether to wrap output in brackets (default to false)
 * @param args names of the arguments
 * @return string form of op, must be free()d
 */
char* treeToString(Tree op, bool bracket, char* args);
/**
 * Computes the operation tree
 * @param op Tree to compute
 * @param args Arguments (only used for functions)
 */
Value computeTree(Tree op, Value* args, int argLen);
/**
 * Generates an operation tree from an equation
 * @param eq Equation
 * @param argNames list of argument names (only for functions)
 * @param base base to compute in, defaults to 10, if base is not 0, eq will be treated as inside square brackets
 * @return Tree, must be freeTree()ed
 */
Tree generateTree(char* eq, char* argNames, double base);
/**
 * Returns the derivative of operation
 * @param op Tree to take the derivative of must be copied before passing
 * @return Tree, must be freed with freeTree()
 */
Tree derivative(Tree op);
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
void graphEquation(char* equation, double left, double right, double top, double bottom, int rows, int columns);
/**
 * Cleans input, generates tree, and computes it
 * @param eq Equation
 * @param base Base to calculate in
 */
Value calculate(char* eq, double base);
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
 * Returns input, but removes spaces and lowers to case of the letters
 * @param input
 * @return output, must be free()d
 */
char* inputClean(char* input);
#ifdef __cplusplus
}
#endif
#pragma region Operation IDs
//These definitons are to ease readability of the program and to allow flexibility
#define op_val 0
#define op_i 1
#define op_neg 2
#define op_pow 3
#define op_mod 4
#define op_mult 5
#define op_div 6
#define op_add 7
#define op_sub 8
#define op_sin 9
#define op_cos 10
#define op_tan 11
#define op_csc 12
#define op_sec 13
#define op_cot 14
#define op_sinh 15
#define op_cosh 16
#define op_tanh 17
#define op_asin 18
#define op_acos 19
#define op_atan 20
#define op_acsc 21
#define op_asec 22
#define op_acot 23
#define op_asinh 24
#define op_acosh 25
#define op_atanh 26
#define op_sqrt 27
#define op_cbrt 28
#define op_exp 29
#define op_ln 30
#define op_logten 31
#define op_log 32
#define op_fact 33
#define op_sgn 34
#define op_abs 35
#define op_arg 36
#define op_norm 37
#define op_round 38
#define op_floor 39
#define op_ceil 40
#define op_getr 41
#define op_geti 42
#define op_getu 43
#define op_grthan 44
#define op_equal 45
#define op_min 46
#define op_max 47
#define op_lerp 48
#define op_dist 49
#define op_not 50
#define op_and 51
#define op_or 52
#define op_xor 53
#define op_ls 54
#define op_rs 55
#define op_pi 56
#define op_phi 57
#define op_e 58
#define op_ans 59
#define op_hist 60
#define op_histnum 61
#define op_rand 62
#define op_sum 63
#define op_product 64
#pragma endregion