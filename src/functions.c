//functions.c contains the code about: anonymous functions, custom functions, multi-line functions, and builtin functions
#include "general.h"
#include "functions.h"
#include "parser.h"
#include "compute.h"
char** globalLocalVariables = NULL;
Value* globalLocalVariableValues = NULL;
int globalLocalVariableSize = 5;
int functionArrayLength = 10;
int numFunctions = 0;
/**
 * Get stdfunction Id from find
 * @param list list to search (must be sortedBuiltin)
 * @param find Function name to find
 * @param count number of elements in list
 */
int getStdFunctionID(int* list, const char* find, int count) {
    if(count < 3) {
        if(strcmp(stdfunctions[list[0]].name, find) == 0) return 0;
        if(strcmp(stdfunctions[list[1]].name, find) == 0) return 1;
        return -1;
    }
    int half = count / 2;
    int compare = strcmp(stdfunctions[list[half]].name, find);
    if(compare == 0) return half;
    else if(compare < 0) {
        int res = getStdFunctionID(list + half, find, count - half);
        if(res == -1) return -1;
        return res + half;
    }
    else return getStdFunctionID(list, find, half);
}
int cmpFunctionNames(int id1, int id2) {
    return strcmp(stdfunctions[id1].name, stdfunctions[id2].name);
}
const char* includeFuncTypes[] = {
    "quaternion",
    "random",
    "probability",
    "vectors",
    "geometry",
};
struct LibraryFunction includeFuncs[includeFuncsLen] = {
    {"ncr","(n,r)","(fact(n)/fact(n-r))/fact(r)",2},
    {"npr","(n,r)","fact(n)/fact(n-r)",2},
    {"choose","(n,r)","(fact(n)/fact(n-r))/fact(r)",2},
    //Mean of a vector
    {"mean","(vec)","sum(i=>ge(vec,i),0,length(vec),1)/length(vec)",1},
    //Random Range
    {"rand_range","(min,max)","rand*(max-min)+min",1},
    //Random Int
    {"rand_int","(min,max)","floor(rand*(max-min)+min)",1},
    //Random member
    {"rand_member","(vec)","ge(vec,floor(rand*length(vec)))",1},
    //p(func,i) runs func i times and finds the probability it returns 1
    {"p","(func,n)","(n-sum(i=>equal(run(func,i),0),0,n,1))/n",2},

    //getrow
    {"getrow","(vec,row)","fill(x=>ge(vec,x,row),width(vec),1)"},
    //getcolumn
    {"getcolumn","(vec,col)","fill((x,y)=>ge(vec,col,y),1,height(vec))"},
    //excluderow
    //excludecolumn

    //solvesyseq
    {"solvesyseq","(coef,out)","mat_mult(mat_inv(coef),transpose(out))",-1},
    //solvequad
    {"solvequad","(a,b,c)","<-b+sqrt(b^2-4a*c),-b-sqrt(b^2-4a*c)>/2a",-1},

    //area_circle
    {"area_circle","(r)","pi*r^2",4},
    //area_triangle
    {"area_tri","(b,h)","b*h/2",4},
    //Volume circle
    {"vol_sphere","(r)","4*pi*r^3/3",4},
    //pythag(a,b)=c
    {"pythag","(a,b)","sqrt(a^2+b^2)",4},
};
Function* customfunctions;
Value runAnonymousFunction(Value val, Value* args) {
    int argCount = argListLen(val.argNames);
    int size = 1;
    int count = 0;
    if(val.code == NULL) {
        error("Anonymous function code missing");
        return NULLVAL;
    }
    Value* localVars = calloc(1, sizeof(Value));
    FunctionReturn out = runCodeBlock(*val.code, args, argCount, &localVars, &count, &size);
    if(globalError) return NULLVAL;
    free(localVars);
    if(out.type > 1) {
        error("Reached unexpected %s", out.type == 2 ? "break" : "continue");
        return NULLVAL;
    }
    if(out.type == 0) return NULLVAL;
    return out.val;
}
Function newFunction(char* name, CodeBlock code, char argCount, char** argNames) {
    Function out;
    out.name = name;
    if(name == NULL) out.nameLen = 0;
    else out.nameLen = strlen(name);
    out.args = argNames;
    out.argCount = argCount;
    out.code = code;
    return out;
}
void generateFunction(const char* eq) {
    //Parse first half a(x)
    int i, equalPos, openBracket = 0, argCount = 1;
    for(i = 0; i < strlen(eq); i++) {
        if(eq[i] == '(')
            openBracket = i;
        if(eq[i] == ',')
            argCount++;
        if(eq[i] == '=') {
            equalPos = i;
            break;
        }
    }
    if(equalPos == 0) {
        error("function has no name", NULL);
        return;
    }
    if(openBracket == 0) {
        openBracket = equalPos;
        argCount = 0;
    }
    if(openBracket > 25) {
        error("function name too long", NULL);
        return;
    }
    //Get function name
    char* name = calloc(openBracket + 1, 1);
    if(name == NULL) { error(mallocError);return; }
    for(i = 0;i < openBracket;i++) {
        if(eq[i] >= 'A' && eq[i] <= 'Z') name[i] = eq[i] + 32;
        else name[i] = eq[i];
    }
    int nameLen = strlen(name);
    //Verify name is not already used
    for(i = 0; i < immutableFunctions; i++) {
        if(stdfunctions[i].nameLen == 0 || stdfunctions[i].nameLen != nameLen)
            continue;
        else if(strcmp(name, stdfunctions[i].name) != 0)
            continue;
        error("function name '%s' is a builtin function", name);
        free(name);
        return;
    }
    for(i = 0;i < numFunctions;i++) {
        if(customfunctions[i].nameLen != nameLen) continue;
        if(strcmp(name, customfunctions[i].name) != 0) continue;
        error("function name '%s' is already used", name);
        free(name);
        return;
    }
    //Get argument names
    char** argNames = parseArgumentList(eq + openBracket);
    if(argNames == NULL) {
        free(name);
        return;
    }
    //Compute parse code block
    char clean[strlen(eq + equalPos)];
    strcpy(clean, eq + equalPos + 1);
    inputClean(clean);
    //If it is a block
    bool isBlock = clean[0] == '{';
    if(isBlock) {
        isBlock = 1;
        clean[strlen(clean) - 1] = '\0';
    }
    CodeBlock code = parseToCodeBlock(clean + isBlock, argNames, NULL, NULL, NULL);
    if(globalError) {
        free(name);
        int i = 0;
        while(argNames[i] != NULL) free(argNames[i++]);
        free(argNames);
        return;
    }
    if(code.listLen == 1 && code.list[0].id == action_statement) {
        code.list[0].id = action_return;
    }
    //Append to functions
    if(functionArrayLength == numFunctions) customfunctions = recalloc(customfunctions, &functionArrayLength, 10, sizeof(Function));
    customfunctions[numFunctions++] = newFunction(name, code, argCount, argNames);
}
void deleteCustomFunction(int id) {
    //Free members
    customfunctions[id].nameLen = 0;
    freeCodeBlock(customfunctions[id].code);
    free(customfunctions[id].name);
    //Free argNames
    freeArgList(customfunctions[id].args);
    //Set tree to NULL
    customfunctions[id].code.list = NULL;
    customfunctions[id].argCount = 0;
}
Tree findFunction(const char* name, bool useUnits, char** arguments, char** localVariables) {
    Tree out;
    int len = strlen(name), id;
    //Units
    if(useUnits) {
        Number unit = getUnitName(name);
        if(unit.i != -1) {
            out.optype = -1;
            out.op = unit.i;
            out.value.type = value_num;
            out.value.num = newNum(unit.r, 0, unit.u);
            return out;
        }
    }
    //Arguments
    id = getArgListId(arguments, name);
    if(id != -1) {
        out.optype = optype_argument;
        out.op = id;
        return out;
    }
    //Local Variables
    id = getArgListId(localVariables, name);
    if(id != -1) {
        out.optype = optype_localvar;
        out.op = id;
        return out;
    }
    //Custom functions
    for(int i = 0;i < numFunctions;i++) {
        if(customfunctions[i].nameLen != len) continue;
        if(strcmp(customfunctions[i].name, name) == 0) {
            out.optype = optype_custom;
            out.op = i;
            return out;
        }
    }
    //Test for builtin functions
    id = getStdFunctionID(sortedBuiltin, name, sortedBuiltinLen);
    if(id != -1) {
        out.optype = optype_builtin;
        out.op = sortedBuiltin[id];
        return out;
    }
    out.optype = 0;
    out.op = 0;
    return out;
}
const struct stdFunction stdfunctions[immutableFunctions] = {
    {0, 0, " "},{0, 1, "i"},
    {1, 3, "neg"},{2, 3, "pow"},{2, 3, "mod"},{2, 4, "mult"},{2, 3, "div"},{2, 3, "add"},{2, 3, "sub"},
    {0, 0, " "},{0, 0, " "}, {0, 0, " "},
    {1, 3, "sin"},{1,3, "cos"}, {1, 3, "tan"}, {1, 3, "csc"}, {1, 3, "sec"}, {1, 3, "cot"},
    {1, 4, "sinh"}, {1, 4, "cosh"}, {1, 4, "tanh"},
    {1, 4, "asin"}, {1, 4, "acos"}, {1, 4, "atan"}, {1, 4, "acsc"}, {1, 4, "asec"}, {1, 4, "acot"},
    {1, 5, "asinh"}, {1, 5, "acosh"}, {1, 5, "atanh"},
{0, 0, " "}, {0, 0, " "},
    {1, 4, "sqrt"}, {1, 4, "cbrt"}, {1, 3, "exp"}, {1, 2, "ln"}, {1, 6, "logten"}, {2, 3, "log"}, {1, 4, "fact"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {1, 3, "sgn"}, {1, 3, "abs"}, {1, 3, "arg"}, {0, 0, " "},
    {1, 5, "round"}, {1, 5, "floor"}, {1, 4, "ceil"}, {1, 4, "getr"}, {1, 4, "geti"}, {1, 4, "getu"}, {2, 6, "grthan"}, {2, 5, "equal"}, {2, 3, "min"}, {2, 3, "max"}, {3, 4, "lerp"}, {2, 4, "dist"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {1, 3, "not"}, {0, 0, " "}, {2, 3, "and"}, {2, 2, "or"}, {2, 3, "xor"}, {2, 2, "ls"}, {2, 2, "rs"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 2, "pi"}, {0, 3, "phi"}, {0, 1, "e"},
{0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 3, "ans"}, {1, 4, "hist"}, {0, 7, "histnum"}, {0, 4, "rand"},
    {0, 0, " "}, {0, 0, " "},
    {1, 3, "run"}, {4, 3, "sum"}, {4, 7, "product"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 0, " "}, {1, 5, "width"}, {1, 6, "height"}, {1, 6, "length"}, {3, 2, "ge"}, {2, 4, "fill"}, {2, 3, "map"}, {1, 3, "det"}, {1, 9, "transpose"}, {2, 8, "mat_mult"}, {1, 7, "mat_inv"}
};
void appendGlobalLocalVariable(char* name, Value value) {
    int place = 0;
    for(place = 0;true;place++) {
        if(globalLocalVariables[place] == NULL) break;
        if(globalLocalVariables[place] == emptyArg) break;
        if(strcmp(globalLocalVariables[place], name) == 0) {
            free(name);
            globalLocalVariableValues[place] = value;
            return;
        }
    }
    //Resize if necessary
    if(place + 2 == globalLocalVariableSize) {
        globalLocalVariableSize += 5;
        globalLocalVariables = realloc(globalLocalVariables, globalLocalVariableSize * sizeof(char*));
        globalLocalVariableValues = realloc(globalLocalVariableValues, globalLocalVariableSize * sizeof(char*));
    }
    globalLocalVariables[place] = name;
    globalLocalVariableValues[place] = value;
}
Value runFunction(Function func, Value* args) {
    int size = 1;
    int count = 0;
    Value* localVars = calloc(size, sizeof(Value));
    FunctionReturn out = runCodeBlock(func.code, args, func.argCount, &localVars, &count, &size);
    if(out.type == 2 || out.type == 3) {
        error("Reached illegal %s statement", out.type == 2 ? "break" : "continue");
        return NULLVAL;
    }
    if(out.type == 0) return NULLVAL;
    if(out.type == 1) return out.val;
}
int* sortedBuiltin;
int sortedBuiltinLen;
#pragma endregion
#pragma region Code Blocks
CodeBlock codeBlockFromTree(Tree tree) {
    CodeBlock out;
    out.listLen = 1;
    out.list = malloc(sizeof(FunctionAction));
    out.list[0].tree = malloc(sizeof(Tree));
    *(out.list[0].tree) = tree;
    out.list[0].id = action_return;
    out.localVarCount = 0;
    out.localVariables = NULL;
    return out;
}
CodeBlock parseToCodeBlock(const char* eq, char** args, char*** localVars, int* localVarSize, int* localVarCount) {
    int zero = 0, two = 2;
    bool freeVariables = false;
    if(localVars == NULL) {
        freeVariables = true;
        localVars = calloc(1, sizeof(char**));
        *localVars = calloc(2, sizeof(char*));
        localVarSize = &two;
        localVarCount = &zero;
    }
    int localVarStackStart = *localVarCount;
    int eqLen = strlen(eq);
    int semicolons[eqLen + 1];
    memset(semicolons, 0, sizeof(int) * (eqLen + 1));
    semicolons[0] = -1;
    semicolons[1] = 0;
    int semicolonId = 1;
    for(;true;semicolonId++) {
        semicolons[semicolonId] = findNext(eq, semicolons[semicolonId - 1] + 1, ';');
        if(semicolons[semicolonId] == -1) break;
    }
    if(semicolons[semicolonId - 1] != eqLen - 1) semicolons[semicolonId] = eqLen;
    else semicolonId--;
    FunctionAction* list = calloc(semicolonId, sizeof(FunctionAction));
    bool prevIf = false;
    for(int i = 0;i < semicolonId;i++) {
        if(globalError) break;
        int len = semicolons[i + 1] - semicolons[i] - 1;
        char section[len + 1];
        memcpy(section, eq + semicolons[i] + 1, len);
        section[len] = 0;
        int eqPos = isLocalVariableStatement(section);
        if(eqPos != 0) {
            char* name = calloc(eqPos + 1, 1);
            memcpy(name, section, eqPos);
            if(*localVarSize == (*localVarCount) - 1) {
                *localVars = recalloc(*localVars, localVarSize, 5, sizeof(char*));
            }
            list[i].tree = malloc(sizeof(Tree));
            list[i].tree[0] = generateTree(section + eqPos + 1, args, *localVars, 0);
            list[i].localVarID = argListAppend(localVars, name, localVarSize);
            if(globalError) break;
            if(localVars[0][(*localVarCount)] == name) (*localVarCount)++;
            list[i].id = action_localvar;
            continue;
        }
        if(startsWith(section, "return")) {
            list[i].tree = malloc(sizeof(Tree));
            list[i].tree[0] = generateTree(section + 6, args, *localVars, 0);
            if(i != semicolonId - 1) error("illegal statements after return");
            list[i].id = action_return;
            break;
        }
        int parseBlockPos = 0;
        if(startsWith(section, "else")) {
            if(!prevIf) {
                error("unexpected else statement");
                break;
            }
            list[i].id = action_else;
            parseBlockPos = 4;
        }
        prevIf = false;
        bool isWhile = startsWith(section, "while");
        if(startsWith(section, "if") || isWhile) {
            int endParenthesis = findNext(section, 0, ')');
            if(endParenthesis == -1 || section[isWhile ? 5 : 2] != '(') {
                error("missing parenthesis in if statement");
                break;
            }
            list[i].id = isWhile ? action_while : action_if;
            //Parse conditional
            int len = endParenthesis - (isWhile ? 6 : 3);
            char conditional[len];
            memcpy(conditional, section + (isWhile ? 6 : 3), len);
            conditional[len] = 0;
            list[i].tree = malloc(sizeof(Tree));
            list[i].tree[0] = generateTree(conditional, args, *localVars, 0);
            if(globalError) break;
            parseBlockPos = endParenthesis + 1;
            if(!isWhile) prevIf = true;
        }
        if(parseBlockPos) {
            //Inclusive start, exclusive end
            int start, end;
            //Parse as code block
            if(section[parseBlockPos] == '{') {
                start = parseBlockPos + 1;
                end = findNext(section, parseBlockPos, '}');
            }
            //Parse as statement
            else {
                start = parseBlockPos;
                end = semicolons[i + 1];
            }
            char block[end - start + 1];
            memcpy(block, section + start, end - start);
            block[end - start] = 0;
            int oldVarCount = *localVarCount;
            list[i].code = malloc(sizeof(CodeBlock));
            list[i].code[0] = parseToCodeBlock(block, args, localVars, localVarSize, localVarCount);
            if(globalError) break;
            int newVarCount = *localVarCount;
            *localVarCount = oldVarCount;
            memset((*localVars) + oldVarCount, 0, sizeof(char*) * (newVarCount - oldVarCount));
            continue;
        }
        if(startsWith(section, "break")) {
            if(semicolons[i + 1] != 5) error("unexpected '%s' after break", section + 5);
            if(i != semicolonId - 1) error("illegal statements after break");
            if(globalError) break;
            list[i].id = action_break;
        }
        else if(startsWith(section, "continue")) {
            if(semicolons[i + 1] != 8) error("unexpected '%s' after continue", section + 8);
            if(i != semicolonId - 1) error("illegal statements after continue");
            if(globalError) break;
            list[i].id = action_continue;
        }
        else {
            list[i].id = 0;
            list[i].tree = malloc(sizeof(Tree));
            list[i].tree[0] = generateTree(section, args, *localVars, 0);
        }
    }
    CodeBlock out;
    out.list = list;
    out.listLen = semicolonId;
    out.localVarCount = *localVarCount - localVarStackStart;
    if(out.localVarCount != 0) out.localVariables = calloc(out.localVarCount + 1, sizeof(char*));
    else out.localVariables = NULL;
    memcpy(out.localVariables, (*localVars) + localVarStackStart, out.localVarCount * sizeof(char*));
    if(globalError) {
        freeCodeBlock(out);
        return NULLCODE;
    }
    if(freeVariables) {
        //free(*localVars);
        free(localVars);
    }
    return out;
}
const FunctionReturn return_null = { 0,0 };
const FunctionReturn return_break = { 2,0 };
const FunctionReturn return_continue = { 3,0 };
FunctionReturn runCodeBlock(CodeBlock func, Value* arguments, int argCount, Value** localVars, int* localVarCount, int* localVarSize) {
    if(func.localVarCount != 0) *localVars = recalloc(*localVars, localVarSize, func.localVarCount, sizeof(Value));
    bool prevIf = false;
    FunctionReturn toReturn = return_null;
    for(int i = 0;i < func.listLen;i++) {
        FunctionAction action = func.list[i];
        //Statement
        if(action.id == action_statement) {
            freeValue(computeTree(*action.tree, arguments, argCount, *localVars));
        }
        //Return
        else if(action.id == action_return) {
            toReturn.val = computeTree(*action.tree, arguments, argCount, *localVars);
            toReturn.type = 1;
            break;
        }
        //Set local variable
        else if(action.id == action_localvar) {
            (*localVars)[action.localVarID] = computeTree(*action.tree, arguments, argCount, *localVars);
        }
        //If statement
        else if(action.id == action_if) {
            Value val = computeTree(*action.tree, arguments, argCount, *localVars);
            bool branch = getR(val) == 0 ? false : true;
            prevIf = !branch;
            if(branch) {
                if(action.code == NULL) {
                    error("Missing code block");
                    toReturn = return_null;
                    break;
                }
                FunctionReturn out = runCodeBlock(*action.code, arguments, argCount, localVars, localVarCount, localVarSize);
                if(out.type != 0) {
                    toReturn = out;
                    break;
                }
            }
            continue;
        }
        //Else statemnt
        else if(action.id == action_else) {
            if(prevIf) {
                if(action.code == NULL) {
                    error("Missing code block");
                    toReturn = return_null;
                    break;
                }
                FunctionReturn out = runCodeBlock(*action.code, arguments, argCount, localVars, localVarCount, localVarSize);
                if(out.type != 0) {
                    toReturn = out;
                    break;
                }
            }
            continue;
        }
        else if(action.id == action_while) {
            if(action.code == NULL || action.tree == NULL) {
                error("Missing %s", action.code == NULL ? "code block" : "tree");
                toReturn = return_null;
                break;
            }
            Value conditional = computeTree(*action.tree, arguments, argCount, *localVars);
            bool loop = getR(conditional) == 0 ? false : true;
            freeValue(conditional);
            int loopCount = 0;
            while(loop) {
                FunctionReturn ret = runCodeBlock(*action.code, arguments, argCount, localVars, localVarCount, localVarSize);
                //Exit if return or break reached
                if(ret.type == 1) {
                    toReturn = ret;
                    break;
                }
                if(ret.type == 2) break;
                //Generate conditional for next loop
                conditional = computeTree(*action.tree, arguments, argCount, *localVars);
                loop = getR(conditional) == 0 ? false : true;
                freeValue(conditional);
                //Return if loop count maxed
                loopCount++;
                if(loopCount > 100000) {
                    error("Infinite loop detected");
                    toReturn.type = 1;
                    break;
                }
            }
            if(toReturn.type == 1) break;
        }
        else if(action.id == action_for) {

        }
        else if(action.id == action_break) {
            toReturn = return_break;
            break;
        }
        else if(action.id == action_continue) {
            toReturn = return_continue;
            break;
        }
    }
    //Free localvariables
    return toReturn;
}
CodeBlock copyCodeBlock(CodeBlock code, const Tree* replaceArgs, int replaceCount, bool unfold) {
    CodeBlock out = code;
    out.list = calloc(code.listLen, sizeof(FunctionAction));
    for(int i = 0;i < code.listLen;i++) {
        FunctionAction action = code.list[i];
        FunctionAction outAction = action;
        if(action.tree != NULL) {
            outAction.tree = malloc(sizeof(Tree));
            *outAction.tree = copyTree(*action.tree, replaceArgs, replaceCount, unfold);
        }
        if(action.code != NULL) {
            outAction.code = malloc(sizeof(CodeBlock));
            *outAction.code = copyCodeBlock(*action.code, replaceArgs, replaceCount, unfold);
        }
        out.list[i] = outAction;
    }
    out.localVariables = argListCopy(code.localVariables);
    return out;
}
void freeCodeBlock(CodeBlock code) {
    freeArgList(code.localVariables);
    for(int i = 0;i < code.listLen;i++) {
        FunctionAction action = code.list[i];
        if(action.tree != NULL) {
            freeTree(*action.tree);
            free(action.tree);
        }
        if(action.code != NULL) {
            freeCodeBlock(*action.code);
            free(action.code);
        }
    }
    free(code.list);
}
char* codeBlockToString(CodeBlock code, char** localVariables, char** arguments) {
    char** localVars = mergeArgList(localVariables, code.localVariables);
    //Get lines
    char* lines[code.listLen];
    int cumulativeLength = 0;
    for(int i = 0;i < code.listLen;i++) {
        FunctionAction action = code.list[i];
        if(action.id == action_statement) {
            lines[i] = treeToString(*action.tree, false, arguments, localVars);
        }
        else if(action.id == action_return) {
            char* statement = treeToString(*action.tree, false, arguments, localVars);
            lines[i] = calloc(strlen(statement) + 9, 1);
            strcpy(lines[i], "return ");
            strcat(lines[i], statement);
            free(statement);
        }
        else if(action.id == action_localvar) {
            char* var = localVars[action.localVarID];
            char* statement = treeToString(*action.tree, false, arguments, localVars);
            lines[i] = calloc(strlen(var) + strlen(statement) + 3, 1);
            strcpy(lines[i], var);
            strcat(lines[i], "=");
            strcat(lines[i], statement);
            free(statement);
        }
        else if(action.id == action_if || action.id == action_while) {
            char* statement = treeToString(*action.tree, false, arguments, localVars);
            char* code = codeBlockToString(*action.code, localVars, arguments);
            lines[i] = calloc(strlen(statement) + strlen(code) + 9, 1);
            if(action.id == action_if) strcpy(lines[i], "if(");
            else strcpy(lines[i], "while(");
            strcat(lines[i], statement);
            free(statement);
            strcat(lines[i], ") ");
            strcat(lines[i], code);
            free(code);
        }
        else if(action.id == action_else) {
            char* code = codeBlockToString(*action.code, localVars, arguments);
            lines[i] = calloc(strlen(code) + 6, 1);
            strcpy(lines[i], "else ");
            strcat(lines[i], code);
            free(code);
        }
        else if(action.id == action_for) {
            //TODO: Not sure what to do
        }
        else if(action.id == action_break || action.id == action_continue) {
            lines[i] = calloc(10, 1);
            if(action.id == action_break) strcpy(lines[i], "break");
            else strcpy(lines[i], "continue");
        }
        else {
            lines[i] = calloc(1, 1);
        }
        cumulativeLength += strlen(lines[i]);
    }
    //Compile lines
    bool useBrackets = code.listLen == 1;
    char* out = calloc(cumulativeLength + code.listLen + (useBrackets ? 3 : 0) + 2, 1);
    if(useBrackets)out[0] = '{';
    for(int i = 0;i < code.listLen;i++) {
        strcat(out, lines[i]);
        strcat(out, ";");
        free(lines[i]);
    }
    if(useBrackets) strcat(out, "}");
    free(localVars);
    return out;
}
#pragma endregion