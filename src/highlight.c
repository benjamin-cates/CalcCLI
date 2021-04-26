//highlight.c contains functions that highlight text
//Header information is in general.h
#include "general.h"
#include "functions.h"
#include "parser.h"
#include <string.h>
/*
    0 - Null (no color)
    1 - Number (bold)
    2 - Variable (not used)
    3 - Comment (green)
    4 - Error character (red)
    5 - Bracket (white)
    6 - Operator (bright black)
    7 - String (yellow)
    8 - Command (blue)
    9 - Space (no color)
    10 - Escape sequence(for strings)
    11 - Delimeter (commas, white)
    12 - Invalid operator (red)
    13 - Invalid varible name (red)
    14 - Builtin variable (yellow)
    15 - Custom functions (purple)
    16 - Arguments (blue)
    17 - Unit (green)
    18 - Local Variable (blue)
    19 - Control Flow (bold purple)
*/
const char* syntaxTypes[] = { "Null","Numeral","Variable","Comment","Error","Bracket","Operator","String","Command","Space","Escape","Delimiter","Invalid Operator","Invalid Variable","Builtin","Custom","Argument","Unit","Local Variable","Control Flow","History Accessor" };
typedef enum Color {
    hl_null = 0,
    hl_number = 1,
    hl_variable = 2,
    hl_comment = 3,
    hl_error = 4,
    hl_bracket = 5,
    hl_operator = 6,
    hl_string = 7,
    hl_command = 8,
    hl_space = 9,
    hl_escape = 10,
    hl_delimiter = 11,
    hl_invalidOperator = 12,
    hl_invalidVariable = 13,
    hl_builtin = 14,
    hl_custom = 15,
    hl_argument = 16,
    hl_unit = 17,
    hl_localvar = 18,
    hl_controlFlow = 19,
    hl_hist = 20,
} Color;
int getVariableType(const char* name, bool useUnits, char** argNames, char** localVars) {
    Tree tree = findFunction(name, useUnits, argNames, localVars);
    if(tree.optype == -1) return hl_unit;
    if(tree.optype == optype_builtin) {
        if(tree.op == 0) return hl_invalidVariable;
        return hl_builtin;
    }
    if(tree.optype == optype_custom) return hl_custom;
    if(tree.optype == optype_argument) return hl_argument;
    if(tree.optype == optype_localvar) return hl_localvar;
    return hl_invalidVariable;
}
void highlightSyntax(char* eq, char* out, char** args, char** localVars, int base, bool useUnits) {
    int eqLen = strlen(eq);
    int start = 0;
    int end = start;
    while(eq[start] != 0) {
        while(eq[start] == ' ') {
            out[start] = hl_space;
            start++;
            if(eq[start] == 0) return;
        }
        if(eq[start] == ',' || eq[start] == ';') {
            out[start] = hl_delimiter;
            start++;
            continue;
        }
        int type = nextSection(eq, start, &end, base);
        int len = end - start + 1;
        if(type == sec_undef) {
            out[start] = hl_error;
            start++;
            continue;
        }
        if(type == sec_number) memset(out + start, hl_number, len);
        if(type == sec_variable) {
            char name[len + 1];
            memcpy(name, eq + start, len);
            name[len] = 0;
            inputClean(name);
            if(!useUnits) lowerCase(name);
            Tree op = findFunction(name, useUnits, args, localVars);
            //Units
            if(op.optype == -1) op.optype = 3;
            //Variable not found
            if(op.optype == 0 && op.op == 0) op.optype = -1;
            memset(out + start, op.optype + 14, len);
        }
        if(type == sec_function) {
            //Copy name and remove spaces
            int firstParenth = start;
            while(eq[firstParenth] != '(') firstParenth++;
            int len = firstParenth - start;
            char name[len + 1];
            //Remove spaces and lowercase
            int offset = 0;
            for(int i = 0;i < len - offset;i++) {
                if(eq[i + start + offset] == ' ') { i--;offset++; }
                char copy = eq[i + start + offset];
                if(copy <= 'Z') copy += 32;
                name[i] = copy;
            }
            name[len - offset] = 0;
            Tree op = findFunction(name, false, NULL, NULL);
            memset(out + start, op.optype + 14, len);
            if(eq[end] == 0) {
                start = firstParenth + 1;
                out[firstParenth] = hl_error;
                continue;
            }
            out[firstParenth] = hl_bracket;
            out[end] = hl_bracket;
            char endOriginal = eq[end];
            eq[end] = 0;
            highlightSyntax(eq + firstParenth + 1, out + firstParenth + 1, args, localVars, base, useUnits);
            eq[end] = endOriginal;
        }
        if(type == sec_square || type == sec_parenthesis || type == sec_vector) {
            if(eq[end] == 0) {
                out[start] = hl_error;
                start++;
                if(type == sec_square) useUnits = true;
                continue;
            }
            out[start] = hl_bracket;
            out[end] = hl_bracket;
            char endOriginal = eq[end];
            eq[end] = 0;
            highlightSyntax(eq + start + 1, out + start + 1, args, localVars, base, type == 4 ? true : useUnits);
            eq[end] = endOriginal;
        }
        if(type == sec_squareWithBase) {
            int endBrac = findNext(eq, start, ']');
            //Find underscore
            int underscore = endBrac + 1;
            while(eq[underscore] != '_') {
                out[underscore] = hl_space;
                underscore++;
            }
            //Color brackets
            out[endBrac] = hl_bracket;
            out[start] = hl_bracket;
            //Calculate base
            eq[endBrac] = 0;
            double base = 0;
            char charAfterEndOfBase = eq[end + 1];
            eq[end + 1] = 0;
            ignoreError++;
            Value baseVal = calculate(eq + underscore + 1, 10);
            ignoreError--;
            eq[end + 1] = charAfterEndOfBase;
            if(globalError) {
                globalError = false;
                base = 10;
                highlightSyntax(eq + underscore + 1, out + underscore + 1, NULL, NULL, 10, false);
            }
            else {
                base = getR(baseVal);
                freeValue(baseVal);
                //Highlight as error if out of bounds
                if(base < 1 || base > 36) {
                    memset(out + underscore + 1, hl_error, end - underscore);
                    base = 10;
                }
                else highlightSyntax(eq + underscore + 1, out + underscore + 1, NULL, NULL, base, false);
            }
            //Highlight underscore as operator
            out[underscore] = hl_operator;
            highlightSyntax(eq + start + 1, out + start + 1, args, localVars, base, true);
            eq[endBrac] = ']';
        }
        if(type == sec_operator) {
            //Copy operator to opStr and remove spaces
            char opStr[len + 1];
            memcpy(opStr, eq + start, len);
            opStr[len] = 0;
            inputClean(opStr);
            int opLen = strlen(opStr);
            //Check if it ends in a negative
            if(opStr[opLen - 1] == '-' && opLen != 1) {
                opStr[opLen - 1] = 0;
                opLen--;
            }
            //Check if section is valid
            bool isValid = true;
            if(opLen == 1) {
                if(opStr[0] == '+');
                else if(opStr[0] == '-');
                else if(opStr[0] == '*');
                else if(opStr[0] == '/');
                else if(opStr[0] == '^');
                else if(opStr[0] == '%');
                else if(opStr[0] == '=');
                else if(opStr[0] == '>');
                else if(opStr[0] == '<');
                else isValid = false;
            }
            else if(opLen == 2) {
                if(opStr[0] == '*' && opStr[1] == '*');
                else if(opStr[0] == '=' && opStr[1] == '=');
                else if(opStr[0] == '!' && opStr[1] == '=');
                else if(opStr[0] == '>' && opStr[1] == '=');
                else if(opStr[0] == '<' && opStr[1] == '=');
                else isValid = false;
            }
            else isValid = false;
            if(isValid) memset(out + start, hl_operator, len);
            else memset(out + start, hl_invalidOperator, len);
        }
        if(type == sec_anonymousMultilineFunction || type == sec_anonymousFunction) {
            int eqPos = highlightArgumentList(eq + start, out + start) + start;
            out[eqPos] = hl_operator;
            out[eqPos + 1] = hl_operator;
            ignoreError++;
            char** newArgs = parseArgumentList(eq + start);
            ignoreError--;
            if(globalError) {
                freeArgList(newArgs);
                newArgs = NULL;
                globalError = false;
            }
            char** mergedArgs = mergeArgList(args, newArgs);
            if(type == sec_anonymousFunction) {
                //Find the acutal end of the statement
                int newEnd = eqPos + 2;
                for(;true;newEnd++) {
                    char ch = eq[newEnd];
                    if(ch == '(' || ch == '[' || ch == '<' || ch == '{') {
                        char endType = ')';
                        if(ch == '[') endType = ']';
                        if(ch == '{') endType = '}';
                        if(ch == '<') endType = '>';
                        newEnd = findNext(eq, newEnd, endType);
                        if(newEnd == -1) {
                            newEnd = strlen(eq);
                            break;
                        }
                    }
                    if(ch == '"') {
                        int end = newEnd;
                        nextSection(eq, newEnd, &end, 10);
                        newEnd = end;
                        continue;
                    }
                    if(ch == ',' || ch == ';' || ch == 0) {
                        break;
                    }
                }
                char endChar = eq[newEnd];
                eq[newEnd] = 0;
                highlightSyntax(eq + eqPos + 2, out + eqPos + 2, mergedArgs, NULL, 10, false);
                eq[newEnd] = endChar;
                end = newEnd - 1;
            }
            ///For code blocks
            else {
                int startBracket = eqPos + 2;
                while(eq[startBracket] != '{') startBracket++;
                memset(out + eqPos + 2, hl_space, startBracket - eqPos - 2);
                bool isEndBracket = eq[end] != 0;
                if(isEndBracket) {
                    out[startBracket] = hl_bracket;
                    out[end] = hl_bracket;
                    eq[end] = 0;
                }
                else out[startBracket] = hl_error;
                highlightCodeBlock(eq + startBracket + 1, out + startBracket + 1, mergedArgs, NULL);
                if(isEndBracket) eq[end] = '}';
            }
            freeArgList(newArgs);
            free(mergedArgs);
        }
        if(type == sec_string) {
            bool isEscape = false;
            out[start] = hl_string;
            for(int i = start + 1;i < start + len;i++) {
                if(isEscape) { isEscape = false;out[i] = hl_escape; }
                else if(eq[i] == '\\') { isEscape = true;out[i] = hl_escape; }
                else {
                    out[i] = hl_string;
                    if(eq[i] == '"') break;
                }
            }
        }
        if(type == sec_hist) memset(out + start, hl_hist, len);
        start = end + 1;
        if(eq[end] == 0) break;
    }
}
int highlightArgumentList(char* eq, char* out) {
    int endPos = 0;
    int i = 0;
    if(eq[0] == '(') {
        endPos = findNext(eq, 0, ')');
        if(endPos == -1) out[0] = hl_error;
        else {
            out[0] = hl_bracket;
            out[endPos] = hl_bracket;
        }
        i++;
    }
    bool prevWasComma = true;
    for(;true;i++) {
        if(eq[i] == '=' && i >= endPos) return i;
        if(eq[i] == 0) return i;
        if(eq[i] == ' ') {
            out[i] = hl_space;
            continue;
        }
        if(eq[i] == ',') {
            out[i] = hl_delimiter;
            prevWasComma = true;
            continue;
        }
        if(eq[i] >= 'A' && eq[i] <= 'Z') out[i] = hl_argument;
        else if(eq[i] >= 'a' && eq[i] <= 'z') out[i] = hl_argument;
        else if(eq[i] == '.' || eq[i] == '_') out[i] = hl_argument;
        else if(eq[i] >= '0' && eq[i] <= '9') out[i] = hl_argument;
        if(prevWasComma) {
            prevWasComma = false;
            if((eq[i] >= '0' && eq[i] <= '9') || eq[i] == '.') out[i] = hl_error;
        }
        if(out[i] == 0) out[i] = hl_error;
    }
}
char* highlightLine(char* eq) {
    char* out = calloc(strlen(eq) + 1, 1);
    int start = 0;
    //Comments
    if((eq[0] == '/' && eq[1] == '/') || eq[0] == '#') {
        for(int i = 0;eq[i] != 0;i++) {
            //Set out to comment character
            out[i] = hl_comment;
            if(eq[i] == '$' && eq[i + 1] == '(') {
                //Set $ to operator and find end bracket
                out[i] = hl_operator;
                int endBrac = findNext(eq, i + 1, ')');
                //If end bracket doesn't exist, set ( to error and highlight the rest
                if(endBrac == -1) {
                    out[i + 1] = hl_error;
                    highlightSyntax(eq + i + 2, out + i + 2, NULL, globalLocalVariables, 10, false);
                    i = strlen(eq) - 1;
                }
                //Else set both to matching and set i to the end bracket
                else {
                    out[i + 1] = hl_bracket;
                    out[endBrac] = hl_bracket;
                    eq[endBrac] = 0;
                    highlightSyntax(eq + i + 2, out + i + 2, NULL, globalLocalVariables, 10, false);
                    eq[endBrac] = ')';
                    i = endBrac + 1;
                }
            }
        }
        return out;
    }
    if(eq[0] == '-' && eq[1] >= 'a' && eq[1] <= 'z') {
        //Find the position of the space
        while(eq[start] != ' ' && eq[start] != 0) start++;
        memset(out, hl_command, start);
        out[start] = hl_space;
        if(startsWith(eq, "-base") || startsWith(eq, "-unit")) {
            //Highlight the command suffix
            eq[start] = 0;
            highlightSyntax(eq + 5, out + 5, NULL, globalLocalVariables, 10, false);
            //Highlight the rest
            eq[start] = ' ';
            highlightSyntax(eq + start + 1, out + start + 1, NULL, globalLocalVariables, 10, false);
            return out;
        }
        if(startsWith(eq, "-def ")) {
            int firstParenth = start;
            while(eq[firstParenth] != '(' && eq[firstParenth] != 0 && eq[firstParenth] != '=') firstParenth++;
            //Highlight the name
            memset(out + start + 1, hl_custom, firstParenth - start - 1);
            //highlight the argument list and find the equal position
            int eqPos = highlightArgumentList(eq + firstParenth, out + firstParenth) + firstParenth;
            if(eq[eqPos] != 0) {
                out[eqPos] = hl_command;
                int next = eqPos + 1;
                while(eq[next] == ' ') next++;
                char** args = parseArgumentList(eq + firstParenth);
                //If function is a code block
                if(eq[next] == '{') {
                    //Find end bracket
                    int endBrac = findNext(eq, next, '}');
                    bool hasEndBracket = endBrac != -1;
                    if(hasEndBracket) {
                        out[next] = hl_bracket;
                        out[endBrac] = hl_bracket;
                        eq[endBrac] = 0;
                    }
                    else {
                        out[next] = hl_error;
                        endBrac = strlen(eq);
                    }
                    //Copy content and highlight
                    int len = endBrac - next - 1;
                    char content[len + 1];
                    memcpy(content, eq + next + 1, len);
                    content[len] = 0;
                    highlightCodeBlock(content, out + next + 1, args, NULL);
                    if(hasEndBracket) eq[endBrac] = '}';
                }
                else highlightSyntax(eq + eqPos + 1, out + eqPos + 1, args, NULL, 10, false);
                freeArgList(args);
            }
            out[eqPos] = hl_command;
            return out;
        }
        if(startsWith(eq, "-f ") || startsWith(eq, "-help")) {
            return out;
        }
        if(startsWith(eq, "-dx ") || startsWith(eq, "-g ")) {
            char* args[2];
            args[0] = "x";
            args[1] = NULL;
            highlightSyntax(eq + start + 1, out + start + 1, args, globalLocalVariables, 10, false);
            return out;
        }
        if(startsWith(eq, "-degset ")) {
            char* args[4];
            args[0] = "rad";
            args[1] = "deg";
            args[2] = "grad";
            args[3] = NULL;
            highlightSyntax(eq + start + 1, out + start + 1, args, globalLocalVariables, 10, false);
            return out;
        }
        if(startsWith(eq, "-include ")) {
            for(int i = 0;i < includeFuncsLen;i++) {
                if(strcmp(eq + 9, includeFuncs[i].name) == 0) {
                    memset(out + 9, hl_custom, strlen(eq + 9));
                    return out;
                }
            }
            memset(out + 9, hl_invalidVariable, strlen(eq + 9));
            return out;
        }
    }
    if(eq[0] == '.') {
        out[0] = hl_command;
        start++;
    }
    int isLocalVariable = isLocalVariableStatement(eq);
    if(isLocalVariable) {
        memset(out, hl_localvar, isLocalVariable);
        out[isLocalVariable] = hl_command;
        start = isLocalVariable + 1;
    }
    highlightSyntax(eq + start, out + start, NULL, globalLocalVariables, 10, false);
    return out;
}
void highlightCodeBlock(char* eq, char* out, char** args, char** localVars) {
    if(eq[0] == 0) return;
    char** localVarCopy = argListCopy(localVars);
    int localVarSize = argListLen(localVarCopy);
    int prevLineIsIf = false;
    int eqLen = strlen(eq);
    for(int i = 0;i < eqLen;i++) {
        while(eq[i] == ' ') out[i++] = hl_space;
        //This loop parses each statement separated by ;
        char* type = eq + i;
        bool isCodeBlock = false;
        int end = findNext(eq, i, ';');
        bool endIsNull = end == -1;
        if(endIsNull) end = strlen(eq);
        else eq[end] = 0;
        int isEqual = isLocalVariableStatement(eq + i);
        if(isEqual) {
            char* name = calloc(isEqual + 1, 1);
            memset(out + i, hl_localvar, isEqual);
            out[i + isEqual] = hl_command;
            memcpy(name, eq + i, isEqual);
            lowerCase(name);
            i += isEqual + 1;
            highlightSyntax(eq + i, out + i, args, localVarCopy, 10, false);
            argListAppend(&localVarCopy, name, &localVarSize);
        }
        else if(startsWith(type, "return")) {
            memset(out + i, hl_controlFlow, 6);
            i += 6;
            if(i != end) highlightSyntax(eq + i, out + i, args, localVarCopy, 10, false);
        }
        else if(startsWith(type, "else")) {
            if(prevLineIsIf) memset(out + i, hl_controlFlow, 4);
            else memset(out + i, hl_error, 4);
            i += 4;
            isCodeBlock = true;
        }
        else if(startsWith(type, "if") || startsWith(type, "while")) {
            int typeLen = type[0] == 'i' ? 2 : 5;
            memset(out + i, hl_controlFlow, typeLen);
            isCodeBlock = true;
            i += typeLen;
            int startBrac = i;
            while(eq[startBrac] != '(' && eq[startBrac] != 0) startBrac++;
            if(eq[startBrac] == 0) {
                memset(out + i, hl_error, end - i);
                goto next;
            }
            int endBrac = findNext(eq, startBrac, ')');
            if(endBrac == -1) {
                out[startBrac] = hl_error;
                highlightSyntax(eq + startBrac + 1, out + startBrac + 1, args, localVarCopy, 10, false);
                goto next;
            }
            out[startBrac] = hl_bracket;
            out[endBrac] = hl_bracket;
            eq[endBrac] = 0;
            highlightSyntax(eq + startBrac + 1, out + startBrac + 1, args, localVarCopy, 10, false);
            eq[endBrac] = ')';
            if(type[0] == 'i') prevLineIsIf = true;
            isCodeBlock = true;
            i = endBrac + 1;
        }
        else if(startsWith(type, "break") || startsWith(type, "continue")) {
            if(type[0] == 'b') {
                memset(out + i, hl_controlFlow, 5);
                i += 5;
            }
            else {
                memset(out + i, hl_controlFlow, 8);
                i += 8;
            }
            memset(out + i, hl_error, end - i);
        }
        else {
            //Highlight as an expression
            highlightSyntax(eq + i, out + i, args, localVarCopy, 10, false);
        }
        //Highlight rest of the statement
        if(isCodeBlock) {
            while(eq[i] == ' ') out[i++] = hl_space;
            //Find open bracket
            int openBrac = eq[i] == '{' ? i : -1;
            if(openBrac == -1) {
                //If doesn't exist, highlight anyway
                highlightCodeBlock(eq + i, out + i, args, localVarCopy);
                goto next;
            }
            //Find closing bracket
            int closeBrac = findNext(eq, openBrac, '}');
            if(closeBrac == -1) {
                //If doesn't exist, set open to error and highlight rest
                out[openBrac] = hl_error;
                highlightCodeBlock(eq + openBrac + 1, out + openBrac + 1, args, localVarCopy);
                goto next;
            }
            //If they both exist, set both to valid brackets and highlight the innards
            out[openBrac] = hl_bracket;
            out[closeBrac] = hl_bracket;
            eq[closeBrac] = 0;
            highlightCodeBlock(eq + openBrac + 1, out + openBrac + 1, args, localVarCopy);
            eq[closeBrac] = '}';
        }
    next:
        if(!startsWith(type, "if")) prevLineIsIf = false;
        i = end;
        if(endIsNull) break;
        else {
            eq[end] = ';';
            out[end] = hl_delimiter;
        }
    }
    freeArgList(localVarCopy);
    return;
}