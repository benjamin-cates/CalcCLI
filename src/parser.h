//parser.h contains header information for parser.c
#ifndef PARSER_H
#define PARSER_H 1
#include <stdbool.h>
typedef struct TreeStruct Tree;
typedef enum Section {
    sec_undef = -1,
    sec_number = 0,
    sec_variable = 1,
    sec_function = 2,
    sec_parenthesis = 3,
    sec_square = 4,
    sec_squareWithBase = 5,
    sec_operator = 6,
    sec_vector = 7,
    sec_anonymousFunction = 8,
    sec_anonymousMultilineFunction = 9,
    sec_string = 10,
} Section;
/**
 * Returns the type of a section and when it ends
 * In the parsing engine, text is first split into secttions, this function will find the end of the section.
 * @param eq Equation to parse
 * @param start Position of the first character in the section
 * @param out Return value, sets the int at the pointer to the final character in the section
 * @param base Base (for things like 10A)
 * @return section type (see enum Section)
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
 * Returns a double parsed from the string
 * @param num Text of the number
 * @param base Base to be parsed in
 */
double parseNumber(const char* num, double base);
#endif