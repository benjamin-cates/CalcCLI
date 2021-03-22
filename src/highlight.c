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
const char* syntaxTypes[] = { "Null","Numeral","Variable","Comment","Error","Bracket","Operator","String","Command","Space","Escape","Delimiter","Invalid Operator","Invalid Variable","Builtin","Custom","Argument","Unit","Local Variable","Control Flow" };
int getVariableType(const char* name, bool useUnits, char** argNames, char** localVars) {
    Tree tree = findFunction(name, useUnits, argNames, localVars);
    if(tree.optype == -1) return 17;
    if(tree.optype == optype_builtin) {
        if(tree.op == 0) return 13;
        return 14;
    }
    if(tree.optype == optype_custom) return 15;
    if(tree.optype == optype_argument) return 16;
    if(tree.optype == optype_localvar) return 18;
    return 13;
}
void highlightSyntax(char* eq, char* out, char** args, char** localVars, int base, bool useUnits) {
    int eqLen = strlen(eq);
    int start = 0;
    int end = start;
    while(eq[start] != 0) {
        while(eq[start] == ' ') {
            out[start] = 9;
            start++;
            if(eq[start] == 0) return;
        }
        if(eq[start] == ',' || eq[start] == ';') {
            out[start] = 11;
            start++;
            continue;
        }
        int type = nextSection(eq, start, &end, base);
        int len = end - start + 1;
        if(type == -1) {
            out[start] = 4;
            start++;
            continue;
        }
        //Numbers
        else if(type == 0) memset(out + start, 1, len);
        //Variables
        else if(type == 1) {
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
        //Function
        else if(type == 2) {
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
                out[firstParenth] = 4;
                continue;
            }
            out[firstParenth] = 5;
            out[end] = 5;
            char endOriginal = eq[end];
            eq[end] = 0;
            highlightSyntax(eq + firstParenth + 1, out + firstParenth + 1, args, localVars, base, useUnits);
            eq[end] = endOriginal;
        }
        //Parenthesis, square brackets or vectors
        else if(type == 4 || type == 3 || type == 7) {
            if(eq[end] == 0) {
                out[start] = 4;
                start++;
                if(type == 4) useUnits = true;
                continue;
            }
            out[start] = 5;
            out[end] = 5;
            char endOriginal = eq[end];
            eq[end] = 0;
            highlightSyntax(eq + start + 1, out + start + 1, args, localVars, base, type == 4 ? true : useUnits);
            eq[end] = endOriginal;
        }
        //Square bracket with base
        else if(type == 5) {
            int endBrac = findNext(eq, start, ']');
            //Find underscore
            int underscore = endBrac + 1;
            while(eq[underscore] != '_') {
                out[underscore] = 9;
                underscore++;
            }
            //Color brackets
            out[endBrac] = 5;
            out[start] = 5;
            //Calculate base
            eq[endBrac] = 0;
            double base = 0;
            char charAfterEndOfBase = eq[end + 1];
            eq[end + 1] = 0;
            ignoreError = true;
            Value baseVal = calculate(eq + underscore + 1, base);
            ignoreError = false;
            eq[end + 1] = charAfterEndOfBase;
            if(globalError) {
                globalError = false;
                base = 10;
                highlightSyntax(eq + underscore + 1, out + underscore + 1, NULL, NULL, base, false);
            }
            else {
                base = getR(baseVal);
                freeValue(baseVal);
                //Highlight as error if out of bounds
                if(base < 1 || base > 36) {
                    memset(out + underscore + 1, 4, end - underscore);
                    base = 10;
                }
                else highlightSyntax(eq + underscore + 1, out + underscore + 1, NULL, NULL, base, false);
            }
            //Highlight underscore as operator
            out[underscore] = 6;
            highlightSyntax(eq + start + 1, out + start + 1, args, localVars, base, true);
            eq[endBrac] = ']';
        }
        //Operators
        else if(type == 6) {
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
                else isValid = false;
            }
            else if(opLen == 2) {
                if(opStr[0] == '*' && opStr[1] == '*');
                else isValid = false;
            }
            else isValid = false;
            if(isValid) memset(out + start, 6, len);
            else memset(out + start, 12, len);
        }
        //Anonymous functions
        else if(type == 9 || type == 8) {
            int eqPos = highlightArgumentList(eq + start, out + start) + start;
            out[eqPos] = 6;
            out[eqPos + 1] = 6;
            ignoreError = true;
            char** args = parseArgumentList(eq + start);
            ignoreError = false;
            if(globalError) {
                args = NULL;
                globalError = false;
            }
            //For non-code blocks
            if(type == 8) {
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
                            newEnd = strlen(eq) - 1;
                            break;
                        }
                    }
                    if(ch == ',' || ch == ';' || ch == 0) {
                        break;
                    }
                }
                char endChar = eq[newEnd];
                eq[newEnd] = 0;
                highlightSyntax(eq + eqPos + 2, out + eqPos + 2, args, NULL, 10, false);
                eq[newEnd] = endChar;
                end = newEnd - 1;
            }
            ///For code blocks
            else {
                int startBracket = eqPos + 2;
                while(eq[startBracket] != '{') startBracket++;
                memset(out + eqPos + 2, 9, startBracket - eqPos - 2);
                bool isEndBracket = eq[end] != 0;
                if(isEndBracket) {
                    out[startBracket] = 5;
                    out[end] = 5;
                    eq[end] = 0;
                }
                else out[startBracket] = 4;
                highlightCodeBlock(eq + startBracket + 1, out + startBracket + 1, args, NULL);
                if(isEndBracket) eq[end] = '}';
            }
            freeArgList(args);
        }
        start = end + 1;
    }
}
int highlightArgumentList(char* eq, char* out) {
    int endPos = 0;
    int i = 0;
    if(eq[0] == '(') {
        endPos = findNext(eq, 0, ')');
        if(endPos == -1) out[0] = 4;
        else {
            out[0] = 5;
            out[endPos] = 5;
        }
        i++;
    }
    bool prevWasComma = true;
    for(;true;i++) {
        if(eq[i] == '=' && i > endPos) return i;
        if(eq[i] == 0) return i;
        if(eq[i] == ' ') {
            out[i] = 9;
            continue;
        }
        if(eq[i] == ',') {
            out[i] = 11;
            prevWasComma = true;
            continue;
        }
        if(eq[i] >= 'A' && eq[i] <= 'Z') out[i] = 16;
        else if(eq[i] >= 'a' && eq[i] <= 'z') out[i] = 16;
        else if(eq[i] == '.' || eq[i] == '_') out[i] = 16;
        else if(eq[i] >= '0' && eq[i] <= '9') out[i] = 16;
        if(prevWasComma) {
            prevWasComma = false;
            if((eq[i] >= '0' && eq[i] <= '9') || eq[i] == '.') out[i] = 4;
        }
        if(out[i] == 0) out[i] = 4;
    }
}
char* highlightLine(char* eq) {
    char* out = calloc(strlen(eq) + 1, 1);
    int start = 0;
    //Comments
    if((eq[0] == '/' && eq[1] == '/') || eq[0] == '#') {
        for(int i = 0;eq[i] != 0;i++) {
            //Set out to comment character
            out[i] = 3;
            if(eq[i] == '$' && eq[i + 1] == '(') {
                //Set $ to operator and find end bracket
                out[i] = 6;
                int endBrac = findNext(eq, i + 1, ')');
                //If end bracket doesn't exist, set ( to error and highlight the rest
                if(endBrac == -1) {
                    out[i + 1] = 4;
                    highlightSyntax(eq + i + 2, out + i + 2, NULL, globalLocalVariables, 10, false);
                    i = strlen(eq) - 1;
                }
                //Else set both to matching and set i to the end bracket
                else {
                    out[i + 1] = 5;
                    out[endBrac] = 5;
                    eq[endBrac] = 0;
                    highlightSyntax(eq + i + 2, out + i + 2, NULL, globalLocalVariables, 10, false);
                    eq[endBrac] = ')';
                    i = endBrac + 1;
                }
            }
        }
        return out;
    }
    if(eq[0] == '-') {
        //Find the position of the space
        while(eq[start] != ' ' && eq[start] != 0) start++;
        memset(out, 8, start);
        out[start] = 9;
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
            while(eq[firstParenth] != '(' && eq[firstParenth] != 0) firstParenth++;
            //Highlight the name
            memset(out + start + 1, 15, firstParenth - start - 1);
            //highlight the argument list and find the equal position
            int eqPos = highlightArgumentList(eq + firstParenth, out + firstParenth) + firstParenth;
            if(eq[eqPos] != 0) {
                out[eqPos] = 8;
                int next = eqPos + 1;
                while(eq[next] == ' ') next++;
                char** args = parseArgumentList(eq + firstParenth);
                //If function is a code block
                if(eq[next] == '{') {
                    //Find end bracket
                    int endBrac = findNext(eq, next, '}');
                    bool hasEndBracket = endBrac == -1;
                    if(hasEndBracket) {
                        out[next] = 4;
                        endBrac = strlen(eq);
                    }
                    else {
                        out[next] = 5;
                        out[endBrac] = 5;
                        eq[endBrac] = 0;
                    }
                    //Copy content and highlight
                    int len = next - endBrac - 1;
                    char content[len + 1];
                    memcpy(content, eq + next + 1, len);
                    content[len] = 0;
                    highlightCodeBlock(content, eq + next + 1, args, NULL);
                    if(hasEndBracket) out[endBrac] = '}';
                }
                else highlightSyntax(eq + eqPos + 1, out + eqPos + 1, args, NULL, 10, false);
                freeArgList(args);
            }
        }
        if(startsWith(eq, "-f ")) {
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
                    memset(out + 9, 15, strlen(eq + 9));
                    return out;
                }
            }
            memset(out + 9, 13, strlen(eq + 9));
            return out;
        }
    }
    if(eq[0] == '.') {
        out[0] = 8;
        start++;
    }
    int isLocalVariable = isLocalVariableStatement(eq);
    if(isLocalVariable) {
        memset(out, 2, isLocalVariable);
        out[isLocalVariable] = 8;
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
        while(eq[i] == ' ') out[i++] = 9;
        //This loop parses each statement separated by ;
        char* type = eq + i;
        bool isCodeBlock = false;
        int end = findNext(eq, i + 1, ';');
        bool endIsNull = end == -1;
        if(endIsNull) end = strlen(eq);
        else eq[end] = 0;
        int isEqual = isLocalVariableStatement(eq + i);
        if(isEqual) {
            char* name = calloc(isEqual + 1, 1);
            memset(out + i, 18, isEqual);
            out[i + isEqual] = 8;
            memcpy(name, eq + i, isEqual);
            i = isEqual + 1;
            highlightSyntax(eq + i, out + i, args, localVarCopy, 10, false);
            argListAppend(&localVarCopy, name, &localVarSize);
        }
        else if(startsWith(type, "return")) {
            memset(out + i, 19, 6);
            i += 6;
            if(i != end) highlightSyntax(eq + i, out + i, args, localVarCopy, 10, false);
        }
        else if(startsWith(type, "else")) {
            if(prevLineIsIf) memset(out + i, 19, 4);
            else memset(out + i, 4, 4);
            i += 4;
            isCodeBlock = true;
        }
        else if(startsWith(type, "if") || startsWith(type, "while")) {
            int typeLen = type[0] == 'i' ? 2 : 5;
            memset(out + i, 19, typeLen);
            isCodeBlock = true;
            i += typeLen;
            int startBrac = i;
            while(eq[startBrac] != '(' && eq[startBrac] != 0) startBrac++;
            if(eq[startBrac] == 0) {
                memset(out + i, 4, end - i);
                goto next;
            }
            int endBrac = findNext(eq, startBrac, ')');
            if(endBrac == -1) {
                out[startBrac] = 4;
                highlightSyntax(eq + startBrac + 1, out + startBrac + 1, args, localVarCopy, 10, false);
                goto next;
            }
            out[startBrac] = 5;
            out[endBrac] = 5;
            eq[endBrac] = 0;
            highlightSyntax(eq + startBrac + 1, out + startBrac + 1, args, localVarCopy, 10, false);
            eq[endBrac] = ')';
            if(type[0] == 'i') prevLineIsIf = true;
            isCodeBlock = true;
            i = endBrac + 1;
        }
        else if(startsWith(type, "break") || startsWith(type, "continue")) {
            if(type[0] == 'b') {
                memset(out + i, 19, 5);
                i += 5;
            }
            else {
                memset(out + i, 19, 8);
                i += 8;
            }
            memset(out + i, 4, end - i);
        }
        else {
            //Set invalid characters to error
            memset(out + i, 4, end - i);
        }
        //Highlight rest of the statement
        if(isCodeBlock) {
            while(eq[i] == ' ') out[i++] = 9;
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
                out[openBrac] = 4;
                highlightCodeBlock(eq + openBrac + 1, out + openBrac + 1, args, localVarCopy);
                goto next;
            }
            //If they both exist, set both to valid brackets and highlight the innards
            out[openBrac] = 5;
            out[closeBrac] = 5;
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
            out[end] = 11;
        }
    }
    freeArgList(localVarCopy);
    return;
}