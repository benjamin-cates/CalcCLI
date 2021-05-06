//functions.h contains header information for functions.c
#ifndef FUNCTIONS_H
#define FUNCTIONS_H 1
#include "general.h"
//Number of optional functions
#define includeFuncsLen 17
//Number of immutable functions
#define immutableFunctions 120
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
//Internal function to compare std function names
int cmpFunctionNames(int id1, int id2);
/**
 * Get stdfunction Id from find
 * @param list list to search (must be sortedBuiltin)
 * @param find Function name to find
 * @param count number of elements in list
 */
int getStdFunctionID(int* list, const char* find, int count);
/**
 * Runs an anonymous function with the args
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
 * Returns it's position
 */
int appendGlobalLocalVariable(char* name, Value value, bool overwrite);
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
extern const FunctionReturn return_null;
extern const FunctionReturn return_break;
extern const FunctionReturn return_continue;
typedef enum FunctionActionTypes {
    action_statement = 0,
    action_return = 1,
    action_localvar = 2,
    action_if = 3,
    action_else = 4,
    action_while = 5,
    action_for = 6,
    action_break = 7,
    action_continue = 8,
    action_localvaraccessor = 9,
} FunctionActionTypes;
#endif