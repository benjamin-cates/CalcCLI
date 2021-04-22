//general.h contains header information for general.c and common standard library header files
#ifndef GENERAL_H
#define GENERAL_H 1
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#pragma region Structures
#define unit_t unsigned long long
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
    Number* val;
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
        char* string;
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
    const int nameLen;
    const char* name;
    const unsigned char inputs[5];
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
#pragma region Global Variables
//Degree ratio, 1 if radian, pi/180 if degrees
extern double degrat;
//Error has occured
extern bool globalError;
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
extern const char numberChars[];
//Error message for when malloc returns zero
extern const char* mallocError;
//Empty arg is used as a place holder in argument lists
extern char* emptyArg;
#pragma endregion
#pragma region Common functions
/**
 * Prints an error message as printf("Error: "+format,message), then sets globalError to true
 * Arguments are similar to printf
 */
void error(const char* format, ...);
/**
 * prints a string, this is called when the builtin function print()
 */
void printString(Value string);
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
/**
 * Reallocates ptr to be *sizePtr+sizeIncrease*elSize
 * Then increments sizePtr by sizeIncrease*elSize
 * Fills the new allocated space with zeroes and returns a pointer to it
 */
void* recalloc(void* ptr, int* sizePtr, int sizeIncrease, int elSize);
//Removes all spaces from input
void inputClean(char* input);
//Will lowercase the contents of str (move range A-Z to a-z)
void lowerCase(char* str);
/**
 * Returns whether string starts with sw
 */
bool startsWith(const char* string, const char* sw);
/**
 * Sorts list using cmp as a compare function by applying a mergesort
 * @param list list of integers to sort
 * @param count length of list
 * @param cmp function to compare list pieces
 */
void mergeSort(int* list, int count, int (*cmp)(int, int));
int findNext(const char* str, int start, char find);
#pragma endregion
#pragma region Files other than general.c
#pragma region command.c
/**
 * Basic command running, returns a string with the command output
 * Returns NULL if the command is not recognized
 * Return value must be freed
 * Note: this function is in command.c
 */
char* runCommand(char* input);
/**
 * Appends a number to history
 * @param num Number to append
 * @param base Base to output in
 * @param print Whether to print or return the message
 * @return Returns the message if print is false (return must be freed)
 */
char* appendToHistory(Value num, double base, bool print);
//Size of the history array (in command.c)
extern int historySize;
//Number of items in history (in command.c)
extern int historyCount;
//History array (in command.c)
extern Value* history;
#pragma endregion
#pragma region units.c
//Number of metric prefixes
#define metricCount 18
//Number of units#
#define unitCount 58
//Metric prefixes
extern const char metricNums[];
//Metric prefix values
extern const double metricNumValues[];
//Metric base units, used for toStringUnit
extern const char* baseUnits[];
//All predefined units
extern const unitStandard unitList[];
/**
 * Returns a number with the unit from the unit name
 * @param name Name of the unit
 * @return Number with u=baseUnits and r=multiplier
 */
Number getUnitName(const char* name);
/**
 * Returns the base units or metric unit
 * @param unit base units
 * @return String of the unit, NULL if unit==0
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
#pragma region misc.c
int* primeFactors(int num);
bool isPrime(int num);
void getRatio(double num, int* numerOut, int* denomOut);
/**
 * Print a double as a ratio
 * Return value must be freed
 */
char* printRatio(double in, bool forceSign);
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
 * Returns the derivative of operation
 * @param op Tree to take the derivative of must be copied before passing
 * @return Tree, must be freed with freeTree()
 */
Tree derivative(Tree op);
#pragma endregion
#pragma region highlight.c
extern const char* syntaxTypes[];
///Return the syntax id of name
int getVariableType(const char* name, bool useUnits, char** argNames, char** localVars);
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
#pragma region print.c
/**
 * Returns the string form of num in base base
 * @param num Number to print
 * @param base base to print it in
 * @return string, maximum length of 24, must be freed
 */
char* doubleToString(double num, double base);
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
char* arbToString(Arb arb, int base, int digitAccuracy);
/**
 * Returns the string version of a value. Return char* must be freed.
 */
char* valueToString(Value val, double base);
/**
 * Returns the string form of operation op, output must be free()d
 * @param op operation to print
 * @param bracket whether to wrap output in brackets (default to false)
 * @param args names of the arguments
 * @return string form of op, must be free()d
 */
char* treeToString(Tree op, bool bracket, char** argNames, char** localVars);
#pragma endregion
#pragma endregion
#pragma region Constructors, Copiers, and Freers
/**
 * Returns a number made of the three components
 * @param r Real component
 * @param i Imaginary component
 * @param u Unit component
 */
Number newNum(double r, double i, unit_t u);
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
Vector newVec(short width, short height);
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
//Returns the value converted to a number
Number getNum(Value val);
//one and two will be altered to be similar types depending on what (int type) is. (int type) can be op_mult, op_add, or op_div
// The (return & 1) is whether one has been altered. The (return &2) is whether two has been altered
int valueConvert(int type, Value* one, Value* two);
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
Tree copyTree(Tree tree, const Tree* replaceArgs, int replaceCount, bool unfold);
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
Tree copyTree(Tree tree, const Tree* replaceArgs, int replaceCount, bool unfold);
#pragma endregion
#pragma region Argument Lists
//Frees an argument list
void freeArgList(char** argList);
//Returns the number of elements in the argument list
int argListLen(char** argList);
//Returns a copy of an argument list, return value must be freed with freeArgList()
char** argListCopy(char** argList);
//Merges two argument lists; return value must be freed with free(). Warning: all this does is move the pointers from one and the pointers from two into a single list. It does not copy the contents of either. Using freeArgList() on the return value will wreak havoc
char** mergeArgList(char** one, char** two);
//Prints an argument list as a string
char* argListToString(char** argList);
//Parses an argument list, will end at an equal sign and ignore most characters. Return value must be freed with freeArgList()
char** parseArgumentList(const char* list);
/*
 *  Appends toAppend to list, expands size of necessary returns the position of toAppend
 * If it encounters an arg that is identical to toAppend, it will return the position of that arg
 *  Usage: int size=5;char** list=calloc(5,sizeof(char**));
 *  int appendPos = argListAppend(&list,{somestring},&size);
 */
int argListAppend(char*** pointerToList, char* toAppend, int* pointerToSize);
//Returns the position of name in argList, -1 if not found
int getArgListId(char** argList, const char* name);
#pragma endregion
#pragma region Enumerators
typedef enum ValueType {
    value_num = 0,
    value_vec = 1,
    value_func = 2,
    value_arb = 3,
    value_string = 4
} ValueType;
typedef enum OpType {
    optype_builtin = 0,
    optype_custom = 1,
    optype_argument = 2,
    optype_anon = 3,
    optype_localvar = 4
} OpType;
//Builtin function ids
typedef enum Op {
    op_val = 0,
    op_i = 1,
    op_neg = 2,
    op_pow = 3,
    op_mod = 4,
    op_mult = 5,
    op_div = 6,
    op_add = 7,
    op_sub = 8,
    op_sin = 12,
    op_cos = 13,
    op_tan = 14,
    op_csc = 15,
    op_sec = 16,
    op_cot = 17,
    op_sinh = 18,
    op_cosh = 19,
    op_tanh = 20,
    op_asin = 21,
    op_acos = 22,
    op_atan = 23,
    op_acsc = 24,
    op_asec = 25,
    op_acot = 26,
    op_asinh = 27,
    op_acosh = 28,
    op_atanh = 29,
    op_sqrt = 32,
    op_cbrt = 33,
    op_exp = 34,
    op_ln = 35,
    op_logten = 36,
    op_log = 37,
    op_fact = 38,
    op_sgn = 43,
    op_abs = 44,
    op_arg = 45,
    op_round = 47,
    op_floor = 48,
    op_ceil = 49,
    op_getr = 50,
    op_geti = 51,
    op_getu = 52,
    op_equal = 53,
    op_not_equal = 54,
    op_lt = 55,
    op_gt = 56,
    op_lt_equal = 57,
    op_gt_equal = 58,
    op_min = 59,
    op_max = 60,
    op_lerp = 61,
    op_dist = 62,
    op_not = 65,
    op_and = 67,
    op_or = 68,
    op_xor = 69,
    op_ls = 70,
    op_rs = 71,
    op_pi = 75,
    op_phi = 76,
    op_e = 77,
    op_typeof = 83,
    op_ans = 84,
    op_hist = 85,
    op_histnum = 86,
    op_rand = 87,
    op_run = 90,
    op_sum = 91,
    op_product = 92,
    op_vector = 96,
    op_width = 97,
    op_height = 98,
    op_length = 99,
    op_ge = 100,
    op_fill = 101,
    op_map = 102,
    op_det = 103,
    op_transpose = 104,
    op_mat_mult = 105,
    op_mat_inv = 106,
    op_string = 111,
    op_eval = 112,
    op_print = 113,
    op_error = 114,
    op_replace = 115,
    op_indexof = 116,
    op_substr = 117,
    op_lowercase = 118,
    op_uppercase = 119,
} Op;
#pragma endregion
#endif