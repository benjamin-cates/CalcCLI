//general.c contains basic global variables and commonly used functions
#include "general.h"
#include "parser.h"
#include "compute.h"
#include "arb.h"
#include "functions.h"
#pragma region Global Variables
double degrat = 1;
bool globalError = false;
bool ignoreError = false;
Number NULLNUM;
Tree NULLOPERATION;
Value NULLVAL;
CodeBlock NULLCODE;
char* emptyArg = "";
const char* mallocError = "malloc returned null";
const char numberChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#pragma endregion
#pragma region Standard Functions
bool startsWith(const char* string, const char* sw) {
    int compareLength = strlen(sw);
    return memcmp(string, sw, compareLength) == 0 ? true : false;
}
void* recalloc(void* ptr, int* sizePtr, int sizeIncrease, int elSize) {
    int oldSize = *sizePtr;
    (*sizePtr) += sizeIncrease;
    void* out = realloc(ptr, (*sizePtr) * elSize);
    if(out == NULL) {
        error(mallocError);
        return NULL;
    }
    memset(((char*)out) + oldSize * elSize, 0, sizeIncrease * elSize);
    return out;
}
void inputClean(char* input) {
    if(input[0] == '\0') {
        error("no equation");
        return;
    }
    int offset = 0, i;
    for(i = 0; input[i] != 0; i++) {
        if(input[i] == '\n') offset++;
        else if(input[i] == ' ') offset++;
        else {
            input[i - offset] = input[i];
        }
    }
    input[i - offset] = 0;
}
void lowerCase(char* str) {
    int i = -1;
    while(str[++i] != 0) {
        if(str[i] >= 'A' && str[i] <= 'Z') str[i] += 32;
    }
}
void mergeSort(int* list, int count, int (*cmp)(int, int)) {
    if(count == 1) return;
    int half1 = count / 2;
    int half2 = count - half1;
    mergeSort(list, half1, cmp);
    mergeSort(list + half1, half2, cmp);
    int listCopy[count];
    memcpy(listCopy, list, count * sizeof(int));
    int p1pos = 0;
    int p2pos = half1;
    int listPos = 0;
    while(p1pos != half1 || p2pos != count) {
        if(p1pos == half1) list[listPos++] = listCopy[p2pos++];
        else if(p2pos == count) list[listPos++] = listCopy[p1pos++];
        else {
            if((*cmp)(listCopy[p1pos], listCopy[p2pos]) > 0) list[listPos++] = listCopy[p2pos++];
            else list[listPos++] = listCopy[p1pos++];
        }
    }
    return;
}
#pragma endregion
#pragma region Commonly Used
Value calculate(const char* eq, double base) {
    //Clean input
    char clean[strlen(eq) + 1];
    strcpy(clean, eq);
    inputClean(clean);
    if(globalError) return NULLVAL;
    //Generate tree
    Tree tree = generateTree(clean, NULL, globalLocalVariables, base);
    if(globalError) return NULLVAL;
    //Compute tree
    Value ans = computeTree(tree, NULL, 0, globalLocalVariableValues);
    freeTree(tree);
    if(globalError) return NULLVAL;
    return ans;

}
int findNext(const char* str, int start, char find) {
    int bracket = 0;
    char brackets[strlen(str + start) + 1];
    char ch;
    for(int i = start;(ch = str[i]) != 0;i++) {
        if(ch == '(' || ch == '[' || ch == '{' || ch == '<') {
            brackets[bracket] = ch;
            bracket++;
        }
        if(ch == ')') {
            int j = bracket - 1;
            for(;j > 0;j--) if(brackets[j] == '(') break;
            if(j == 0 && brackets[0] != '(') bracket = j + 1;
            else bracket = j;
        }
        if(ch == ']') {
            int j = bracket - 1;
            for(;j > 0;j--) if(brackets[j] == '[') break;
            if(j == 0 && brackets[0] != '[') bracket = j + 1;
            else bracket = j;
        }
        if(ch == '}') {
            int j = bracket - 1;
            for(;j > 0;j--) if(brackets[j] == '{') break;
            if(j == 0 && brackets[0] != '{') bracket = j + 1;
            else bracket = j;
        }
        if(ch == '>' && i != 0 && str[i - 1] != '=') {
            if(bracket != 0 && brackets[bracket - 1] == '<') bracket--;
        }
        if(bracket == 0 && ch == find) return i;
    }
    return -1;
}
#pragma endregion
#pragma region Main Program
void startup() {
    //Constants
    NULLNUM = newNum(0, 0, 0);
    NULLOPERATION = newOpVal(0, 0, 0);
    NULLVAL = newValNum(0, 0, 0);
    memset(&NULLCODE, 0, sizeof(CodeBlock));
    //Allocate history
    history = calloc(10, sizeof(Value));
    historySize = 10;
    //Allocate local variables
    globalLocalVariables = calloc(5, sizeof(char*));
    globalLocalVariableValues = calloc(5, sizeof(Value));
    //Allocate functions
    customfunctions = calloc(functionArrayLength, sizeof(Function));
    if(history == NULL || customfunctions == NULL) error(mallocError);
    //Sort standard functions
    sortedBuiltin = calloc(immutableFunctions, sizeof(int));
    sortedBuiltinLen = 0;
    //Remove functions with no name
    for(int i = 0;i < immutableFunctions;i++) {
        char firstchar = stdfunctions[i].name[0];
        if(firstchar == ' ' || firstchar == '\0') continue;
        sortedBuiltin[sortedBuiltinLen++] = i;
    }
    mergeSort(sortedBuiltin, sortedBuiltinLen, &cmpFunctionNames);
}
void cleanup() {
    int i;
    //Free functions
    for(i = 0; i < numFunctions; i++) {
        if(customfunctions[i].code.list == NULL) continue;
        free(customfunctions[i].name);
        freeCodeBlock(customfunctions[i].code);
        freeArgList(customfunctions[i].args);
    }
    //Free history
    for(i = 0;i < historyCount;i++) {
        freeValue(history[i]);
    }
    //Free global arrays
    free(customfunctions);
    free(history);
    free(sortedBuiltin);
    int localVarSize = argListLen(globalLocalVariables);
    for(i = 0;i < localVarSize;i++) {
        freeValue(globalLocalVariableValues[i]);
    }
    free(globalLocalVariableValues);
    free(globalLocalVariables);
}
#pragma endregion
#pragma region Constructors, Copiers, and Freers
//Numbers
Number newNum(double r, double i, unit_t u) {
    Number out;
    out.r = r;
    out.i = i;
    out.u = u;
    return out;
}
//Vectors
Vector newVecScalar(Number num) {
    Vector out;
    out.width = out.height = out.total = 1;
    out.val = malloc(sizeof(Number));
    if(out.val == NULL) error(mallocError);
    out.val[0] = num;
    return out;
}
Vector newVec(short width, short height) {
    Vector out;
    out.width = width;
    out.height = height;
    out.total = width * height;
    out.val = calloc(out.total, sizeof(Number));
    if(out.val == NULL) error(mallocError);
    return out;
}
//Values
void valueConvert(int type, Value* one, Value* two) {
    //Convert to Arb if one is arb
    if((one->type == value_arb || two->type == value_arb) && one->type != two->type) {
        if(one->type == value_vec || two->type == value_vec) {
            error("Vectors do not have arbitrary precision support.");
            return;
        }
        if(two->type == value_num) {
            //Swap pointers to make one the non-arb type.
            Value* temp = one;
            one = two;
            two = temp;
        }
        if(one->type == value_num) {
            one->type = value_arb;
            ArbNum* num = malloc(sizeof(ArbNum));
            num->u = one->num.u;
            num->r = doubleToArb(one->num.r, globalAccuracy);
            num->i = doubleToArb(one->num.i, globalAccuracy);
            one->numArb = num;
        }
    }
    //Throw if(one.type!=two.type) valueConvert(&one,&two); to make them have the same type
    //Put the vector first if multiplied by value
    //and convert vectors to vectors if multiplied by a vector
    if(type == op_mult) if(one->type == value_num && two->type == value_vec) {
        //Swap values
        Value temp = *one;
        *one = *two;
        *two = temp;
    }
    //Convert both values to the same type
    if(type == op_add) {
        if(one->type == value_num && two->type == value_vec) {
            *one = newValMatScalar(value_vec, one->num);
        }
        if(one->type == value_vec && two->type == value_num) {
            *two = newValMatScalar(value_vec, two->num);
        }
    }
}
Value newValMatScalar(int type, Number scalar) {
    Value out;
    out.type = type;
    out.vec = newVecScalar(scalar);
    return out;
}
Value newValNum(double r, double i, unit_t u) {
    Value out;
    out.type = 0;
    out.r = r;
    out.i = i;
    out.u = u;
    return out;
}
Value copyValue(Value val) {
    Value out;
    out.type = val.type;
    if(val.type == value_num) out.num = val.num;
    if(val.type == value_vec) {
        out.vec = newVec(val.vec.width, val.vec.height);
        int i;
        for(i = 0;i < out.vec.total;i++) out.vec.val[i] = val.vec.val[i];
    }
    if(val.type == value_func) {
        out.code = malloc(sizeof(CodeBlock));
        if(out.code == NULL) error(mallocError);
        *out.code = copyCodeBlock(*val.code, NULL, 0, false);
        out.argNames = argListCopy(val.argNames);
    }
    if(val.type == value_arb) {
        if(val.numArb != NULL) {
            out.numArb = malloc(sizeof(ArbNum));
            *out.numArb = *val.numArb;
            out.numArb->r.mantissa = malloc(val.numArb->r.len);
            memcpy(out.numArb->r.mantissa, val.numArb->r.mantissa, val.numArb->r.len);
            out.numArb->i.mantissa = malloc(val.numArb->i.len);
            memcpy(out.numArb->i.mantissa, val.numArb->i.mantissa, val.numArb->i.len);
        }
    }
    return out;
}
double getR(Value val) {
    if(val.type == value_num) {
        return val.r;
    }
    if(val.type == value_vec) {
        if(val.vec.val == NULL) return 0;
        return val.vec.val[0].r;
    }
    if(val.type == value_func) return 0;
    if(val.type == value_arb) {
        if(val.numArb == NULL) return 0;
        return arbToDouble(val.numArb->r);
    }
    return 0;
}
Number getNum(Value val) {
    if(val.type == value_num) return val.num;
    if(val.type == value_vec) return val.vec.val[0];
    if(val.type == value_arb) {
        if(val.numArb == NULL) return NULLNUM;
        return newNum(arbToDouble(val.numArb->r), arbToDouble(val.numArb->i), val.numArb->u);
    }
    return NULLNUM;
}
void freeValue(Value val) {
    if(val.type == value_vec) free(val.vec.val);
    if(val.type == value_func) {
        freeArgList(val.argNames);
        freeCodeBlock(*val.code);
        free(val.code);
    }
    if(val.type == value_arb) {
        if(val.numArb != NULL) {
            free(val.numArb->r.mantissa);
            free(val.numArb->i.mantissa);
        }
        free(val.numArb);
    }
}
//Trees
Tree newOp(Tree* branches, int argCount, int opID, int optype) {
    Tree out;
    out.branch = branches;
    out.argCount = argCount;
    out.op = opID;
    out.optype = optype;
    return out;
}
Tree newOpValue(Value value) {
    Tree out;
    out.op = op_val;
    out.optype = optype_builtin;
    out.value = value;
    return out;
}
Tree newOpVal(double r, double i, unit_t u) {
    Tree out;
    out.op = op_val;
    out.optype = optype_builtin;
    out.value.type = value_num;
    out.value.r = r;
    out.value.i = i;
    out.value.u = u;
    return out;
}
Tree* allocArgs(Tree one, Tree two, bool copyOne, bool copyTwo) {
    Tree* out = malloc(2 * sizeof(Tree));
    if(out == NULL) { error(mallocError);return NULL; }
    if(copyOne)
        out[0] = copyTree(one, NULL, 0, false);
    else
        out[0] = one;
    if(copyTwo)
        out[1] = copyTree(two, NULL, 0, false);
    else
        out[1] = two;
    return out;
}
Tree* allocArg(Tree one, bool copy) {
    Tree* out = malloc(sizeof(Tree));
    if(out == NULL) { error(mallocError);return NULL; }
    if(copy)
        out[0] = copyTree(one, NULL, 0, false);
    else
        out[0] = one;
    return out;
}
void freeTree(Tree tree) {
    if(tree.optype == optype_anon) {
        freeCodeBlock(*tree.code);
        free(tree.code);
        freeArgList(tree.argNames);
        return;
    }
    //Frees a tree's args
    if(tree.op != op_val && tree.branch != NULL) {
        int i = 0;
        for(i = 0; i < tree.argCount; i++)
            freeTree(tree.branch[i]);
        free(tree.branch);
    }
    if(tree.op == op_val && tree.optype == optype_builtin) freeValue(tree.value);
}
Tree copyTree(Tree tree, const Tree* replaceArgs, int replaceCount, bool unfold) {
    Tree out = tree;
    if(unfold && tree.optype == optype_custom) {
        //Todo reference to unfold code block
        out.branch = NULL;

        //(*customfunctions[tree.op]., out.branch, out.argCount)
        //return ret;
        return NULLOPERATION;
    }
    if(tree.optype == optype_anon) {
        out.argNames = argListCopy(tree.argNames);
        out.code = malloc(sizeof(CodeBlock));
        if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
        *out.code = copyCodeBlock(*tree.code, replaceArgs, replaceCount, unfold);
        out.argWidth -= replaceCount;
        return out;
    }
    if(tree.optype == optype_builtin && tree.op == op_val) {
        out.value = copyValue(tree.value);
        return out;
    }
    if(tree.optype == optype_argument) {
        if(replaceCount > tree.op) {
            return copyTree(replaceArgs[tree.op], NULL, 0, false);
        }
        else out.op -= replaceCount;
    }
    //Copy tree branches
    if(tree.argCount != 0) {
        out.branch = malloc(tree.argCount * sizeof(Tree));
        if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
    }
    int i;
    for(i = 0; i < tree.argCount; i++) {
        Tree* branch = out.branch + i;
        *branch = copyTree(tree.branch[i], replaceArgs, replaceCount, unfold);
    }
    //Return
    return out;
}
#pragma endregion
#pragma region Argument Lists
void freeArgList(char** argList) {
    if(argList == NULL) return;
    int i = -1;
    while(argList[++i]) {
        if(argList[i] != emptyArg) free(argList[i]);
    }
    free(argList);
}
int argListLen(char** argList) {
    if(argList == NULL) return 0;
    int out = -1;
    while(argList[++out]);
    return out;
}
char** argListCopy(char** argList) {
    if(argList == NULL) return NULL;
    int len = argListLen(argList);
    char** out = calloc(len + 1, sizeof(char*));
    if(out == NULL) { error(mallocError);return NULL; }
    int i;
    for(i = 0;i < len;i++) {
        if(argList[i] == emptyArg) {
            out[i] = emptyArg;
            continue;
        }
        int strLen = strlen(argList[i]);
        out[i] = calloc(strLen + 1, 1);
        if(out[i] == NULL) { error(mallocError);return NULL; }
        strcpy(out[i], argList[i]);
    }
    return out;
}
char** mergeArgList(char** one, char** two) {
    int oneLen = 0, twoLen = 0;
    if(one != NULL) oneLen = argListLen(one);
    if(two != NULL) twoLen = argListLen(two);
    char** out = calloc(oneLen + twoLen + 1, sizeof(char*));
    if(out == NULL) { error(mallocError);return NULL; }
    memcpy(out, one, oneLen * sizeof(char*));
    memcpy(out + oneLen, two, twoLen * sizeof(char*));
    return out;
}
char* argListToString(char** argList) {
    if(argList[0] == NULL) {
        return calloc(1, 1);
    }
    int totalLen = 0;
    int i = -1;
    while(argList[++i]) {
        totalLen += strlen(argList[i]) + 1;
    }
    if(i == 1) {
        char* out = calloc(strlen(argList[0]), 1);
        if(out == NULL) { error(mallocError);return NULL; }
        strcpy(out, argList[0]);
        return out;
    }
    char* out = calloc(totalLen + 3, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    out[0] = '(';
    i = -1;
    while(argList[++i]) {
        int j;
        strcat(out, argList[i]);
        strcat(out, ",");
    }
    out[totalLen] = ')';
    return out;
}
char** parseArgumentList(const char* list) {
    if(list[0] == '=' || list[0] == '\0') {
        return calloc(1, sizeof(char*));
    }
    int listLen = strlen(list);
    int i;
    int argCount = 1;
    int commaPos[(listLen + 3) / 2];
    commaPos[0] = -1;
    for(i = 0;i < listLen;i++) {
        if(list[i] == ',') {
            commaPos[argCount++] = i;
        }
        if(list[i] == '=' || list[i] == '\0') {
            commaPos[argCount] = i;
            break;
        }
    }
    //Null terminated
    char** out = calloc(argCount + 1, sizeof(char*));
    if(out == NULL) { error(mallocError);return NULL; }
    int j;
    for(i = 0;i < argCount;i++) {
        int stringPos = 0;
        out[i] = calloc(commaPos[i + 1] - commaPos[i], 1);
        if(out[i] == NULL) { error(mallocError);return NULL; }
        for(j = commaPos[i] + 1;j < commaPos[i + 1];j++) {
            if((list[j] >= 'a' && list[j] <= 'z') || (list[j] >= '0' && list[j] <= '9')) out[i][stringPos++] = list[j];
            else if(list[j] >= 'A' && list[j] <= 'Z') out[i][stringPos++] = list[j] + 32;
            else if(list[j] == '(' || list[j] == ')' || list[j] == ' ' || list[j] == '=' || list[j] == '\0') continue;
            else error("invalid '%c' in argument list", list[j]);
        }
        if(out[i][0] < 'a') error("argument name '%s' starts with a numeral", out[i]);
        if(globalError && !ignoreError) {
            for(;i >= 0;i--) free(out[i]);
            free(out);
            return NULL;
        }
    }
    return out;
}
int argListAppend(char*** pointerToList, char* toAppend, int* pointerToSize) {
    if(*pointerToList == NULL) {
        *pointerToList = calloc(2, sizeof(char*));
        **pointerToList = toAppend;
        *pointerToSize = 2;
        return 0;
    }
    //Find length
    int i = -1;
    while((*pointerToList)[++i] != NULL) if(strcmp(toAppend, pointerToList[0][i]) == 0) {
        free(toAppend);
        return i;
    }
    //Expand if meets size
    if(i + 2 > *pointerToSize) *pointerToList = recalloc(*pointerToList, pointerToSize, 5, sizeof(char*));
    if(globalError) return -1;
    //Add to append to list
    pointerToList[0][i] = toAppend;
    return i;
}
int getArgListId(char** argList, const char* name) {
    if(argList == NULL) return -1;
    int i = -1;
    while(argList[++i] != NULL) {
        if(strcmp(name, argList[i]) == 0) return i;
    }
    return -1;
}
#pragma endregion