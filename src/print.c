#include "general.h"
#include "functions.h"
#include "arb.h"
#include <string.h>
#include <math.h>
char* doubleToString(double num, double base) {
    if(num == 0 || base <= 1 || base > 36) {
        char* out = calloc(2, 1);
        if(out == NULL) error(mallocError);
        out[0] = '0';
        return out;
    }
    if(isnan(num)) {
        char* out = malloc(5);
        if(out == NULL) { error(mallocError);return NULL; }
        if(num < 0) strcpy(out, "-NaN");
        else strcpy(out, "NaN");
        return out;
    }
    if(isinf(num)) {
        char* out = malloc(5);
        if(out == NULL) { error(mallocError);return NULL; }
        if(num < 0) strcpy(out, "-Inf");
        else strcpy(out, "Inf");
        return out;
    }
    //Allocate string
    char* out = calloc(25, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    int outPos = 0;
    //Negative numbers
    if(num < 0) {
        out[outPos++] = '-';
        num = fabs(num);
    }
    //calculate log(num,base)
    double magnitude = log(num) / log(base);
    //Create exponent suffix, if necessary
    int expLen = 0, exp = 0;
    char expString[8];
    if(magnitude > 14 || magnitude < -14) exp = floor(magnitude * 1.000000000000001);
    if(exp != 0) {
        snprintf(expString, 7, "e%d", exp);
        expLen = strlen(expString);
        num /= pow(base, exp);
    }
    //Calculate power of highest digit
    double power;
    if(exp == 0) power = pow(base, floor(magnitude * 1.000000000000001));
    else power = 1;
    if(power < 1) power = 1;
    //Main loop
    int i;
    for(i = 0;i < 20;i++) {
        if(num < 0.000000000001) {
            power /= base;
            if(power * base < 0.9) break;
            out[outPos++] = '0';
            continue;
        }
        int digit = floor(num / power + 0.000000001);
        num -= power * digit;
        if(power * base < 1.01 && power * base>0.99) out[outPos++] = '.';
        out[outPos++] = numberChars[digit];
        power /= base;
        if(outPos + expLen >= 24) break;
    }
    //Append exponent suffix
    if(exp != 0) strcat(out, expString);
    return out;
}
char* toStringNumber(Number num, double base) {
    char* real = doubleToString(num.r, base);
    char* imag = doubleToString(num.i, base);
    char* unit = toStringUnit(num.u);
    int outLength = (num.u != 0 ? strlen(unit) + 2 : 0) + (num.i == 0 ? 0 : strlen(imag)) + strlen(real) + 3;
    char* out = calloc(outLength, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    if(num.r != 0 || num.i == 0) strcat(out, real);
    if(num.r != 0 && num.i > 0) strcat(out, "+");
    if(num.i != 0) {
        strcat(out, imag);
        strcat(out, "i");
    }
    if(unit != NULL) {
        strcat(out, "[");
        strcat(out, unit);
        strcat(out, "]");
    }
    free(real);
    free(imag);
    free(unit);
    return out;
}
char* toStringAsRatio(Number num) {
    //Print ratio for R
    char* r = NULL;
    if(num.r != 0) r = printRatio(num.r, false);
    //Print ratio for I
    char* i = NULL;
    if(num.i != 0) i = printRatio(num.i, num.r != 0);
    //Print unit
    char* u = NULL;
    if(num.u != 0) u = toStringUnit(num.u);
    //Compile into out
    //Calculate Length
    int outlen = 3;
    if(r != NULL) outlen += strlen(r);
    if(i != NULL) outlen += strlen(i) + 3;
    if(u != NULL) outlen += strlen(u) + 3;
    char* out = calloc(outlen, 1);
    //Append r
    if(r != NULL) {
        strcpy(out, r);
        free(r);
    }
    //Append i
    if(i != NULL) {
        if(r != NULL)strcat(out, " ");
        strcat(out, i);
        strcat(out, " i");
        free(i);
    }
    //Append u
    if(u != NULL) {
        strcat(out, " [");
        strcat(out, u);
        strcat(out, "]");
        free(u);
    }
    return out;
}
char* valueToString(Value val, double base) {
    if(val.type == value_num) {
        return toStringNumber(val.num, base);
    }
    if(val.type == value_vec) {
        Vector vec = val.vec;
        char* values[vec.width * vec.height];
        int i, j;
        int len = 3;
        for(i = 0;i < vec.width;i++) for(j = 0;j < vec.height;j++) {
            values[i + j * vec.width] = toStringNumber(vec.val[i + j * vec.width], base);
            len += 1 + strlen(values[i + j * vec.width]);
        }
        char* out = calloc(len, sizeof(char));
        if(out == NULL) { error(mallocError);return NULL; }
        strcat(out, "<");
        for(j = 0;j < vec.height;j++) for(i = 0;i < vec.width;i++) {
            if(i != 0) strcat(out, ",");
            else if(j != 0) strcat(out, ";");
            strcat(out, values[i + j * vec.width]);
            free(values[i + j * vec.width]);
        }
        strcat(out, ">");
        return out;
    }
    if(val.type == value_func) {
        Tree outTree;
        outTree.code = val.code;
        outTree.argNames = val.argNames;
        outTree.argCount = argListLen(val.argNames);
        outTree.optype = optype_anon;
        outTree.op = 0;
        return treeToString(outTree, false, NULL, globalLocalVariables);
    }
    if(val.type == value_arb) {
        char* out;
        if(val.numArb == NULL) {
            out = calloc(2, 1);
            out[0] = '0';
            return out;
        }
        char* r = NULL;
        char* i = NULL;
        char* u = NULL;
        int length = 1;
        int digitCount = getArbDigitCount((int)base);
        if(val.numArb->r.mantissa != NULL) {
            r = arbToString(val.numArb->r, base, digitCount);
            length += strlen(r);
        }
        if(val.numArb->i.mantissa != NULL) {
            i = arbToString(val.numArb->i, base, digitCount);
            length += 4 + strlen(i);
        }
        if(val.numArb->u != 0) {
            u = toStringUnit(val.numArb->u);
            length += 3 + strlen(u);
        }
        out = calloc(length, 1);
        if(r != NULL && strcmp("0", r) != 0) {
            strcat(out, r);
            if(i != NULL && strcmp("0", i) != 0) {
                if(val.numArb->i.sign == 0) {
                    strcat(out, " + ");
                    strcat(out, i);
                }
                else {
                    strcat(out, " - ");
                    strcat(out, i + 1);
                }
                strcat(out, "i");
            }
        }
        else if(i != NULL && strcmp("0", i) != 0) {
            strcat(out, i);
            strcat(out, "i");
        }
        if(val.numArb->u != 0) {
            strcat(out, "[");
            strcat(out, u);
            strcat(out, "]");
        }
        if(r != NULL) free(r);
        if(i != NULL) free(i);
        if(u != NULL) free(u);
        return out;
    }
    if(val.type == value_string) {
        char* string = val.string;
        int strLen = strlen(string);
        char* out = calloc(strLen * 2 + 3, 1);
        out[0]='"';
        int outPos = 1;
#define printEscape(char) {out[outPos++]='\\';out[outPos++]=char;}
        for(int i = 0;i < strLen;i++) {
            if(string[i] == '\\') printEscape('\\')
            else if(string[i] == '"') printEscape('"')
            else if(string[i] == '\r') printEscape('r')
            else if(string[i] == '\t') printEscape('t')
            else if(string[i] == '\n') printEscape('n')
            else out[outPos++] = string[i];
        }
#undef printEscape
        out[outPos]='"';
        return out;
    }
    return NULL;
}
char* treeToString(Tree tree, bool bracket, char** argNames, char** localVars) {
    if(tree.optype == optype_anon) {
        char* argListString = argListToString(tree.argNames);
        char** newArgNames = mergeArgList(argNames, tree.argNames);
        char* code;
        bool isBlock = true;
        if((*tree.code).list[0].id == action_return) {
            code = treeToString(*(*tree.code).list[0].tree, true, newArgNames, localVars);
            isBlock = false;
        }
        else code = codeBlockToString(*tree.code, localVars, newArgNames);
        int argListLen = strlen(argListString);
        char* out = calloc(argListLen + 2 + strlen(code) + 3, 1);
        if(out == NULL) { error(mallocError);return NULL; }
        strcpy(out, argListString);
        strcpy(out + argListLen, "=>");
        if(isBlock) strcpy(out + argListLen + 2, "{");
        strcat(out + argListLen + 2, code);
        if(isBlock) strcat(out + argListLen + 2, "}");
        free(argListString);
        free(code);
        free(newArgNames);
        return out;
    }
    //Arguments
    if(tree.optype == optype_argument) {
        if(argNames == NULL) {
            char* num = calloc(10, 1);
            if(num == NULL) { error(mallocError);return NULL; }
            sprintf(num, "{%d}", tree.op);
            return num;
        }
        else {
            int len = strlen(argNames[tree.op]);
            char* out = calloc(len + 1, 1);
            if(out == NULL) { error(mallocError);return NULL; }
            memcpy(out, argNames[tree.op], len);
            return out;
        }
    }
    if(tree.optype == optype_localvar) {
        if(localVars == NULL) { error("Fatal error in tree to string: no local variables");return NULL; }
        char* name = localVars[tree.op];
        char* out = calloc(strlen(name + 1), 1);
        strcpy(out, name);
        return out;
    }
    //Numbers
    if(tree.optype == optype_builtin && tree.op == op_val)
        return valueToString(tree.value, 10);
    //Operations
    if(tree.optype == optype_builtin && (tree.op >= op_neg && tree.op <= op_sub) || (tree.op >= op_equal && tree.op <= op_gt_equal)) {
        char* op;
        if(tree.op == op_neg || tree.op == op_sub) op = "-";
        else if(tree.op == op_pow) op = "^";
        else if(tree.op == op_mod) op = "%";
        else if(tree.op == op_mult) op = "*";
        else if(tree.op == op_div) op = "/";
        else if(tree.op == op_add) op = "+";
        else if(tree.op == op_equal) op = "==";
        else if(tree.op == op_not_equal) op = "!=";
        else if(tree.op == op_gt) op = ">";
        else if(tree.op == op_lt) op = "<";
        else if(tree.op == op_gt_equal) op = ">=";
        else if(tree.op == op_lt_equal) op = "<=";
        else if(tree.branch == NULL) {
            char* out = calloc(10, 1);
            if(out == NULL) { error(mallocError);return NULL; }
            snprintf(out, 10, "NULL%sNULL", op);
            return out;
        }
        //Tostring one and two
        char* one = treeToString(tree.branch[0], true, argNames, localVars);
        char* two = "";
        if(tree.op != op_neg) two = treeToString(tree.branch[1], true, argNames, localVars);
        //Allocate string
        int len = strlen(one) + strlen(two) + 3 + bracket * 2;
        char* out = calloc(len, 1);
        if(out == NULL) { error(mallocError);return NULL; }
        //Print -(one) if tree.op==op_neg, or (one op two) otherwise
        snprintf(out, len, "%s%s%s%s%s%s", tree.op == op_neg ? "-" : "", bracket ? "(" : "", one, tree.op == op_neg ? "" : op, two, bracket ? ")" : "");
        //Free one and two, return
        free(one);
        if(tree.argCount == 2) free(two);
        return out;
    }
    //Vectors
    if(tree.op == op_vector) {
        char* values[tree.argCount];
        int i, j;
        int len = 3;
        for(i = 0;i < tree.argCount;i++) {
            values[i] = treeToString(tree.branch[i], false, argNames, localVars);
            len += 1 + strlen(values[i]);
        }
        char* out = calloc(len, sizeof(char));
        if(out == NULL) { error(mallocError);return NULL; }
        strcat(out, "<");
        int height = tree.argCount / tree.argWidth;
        for(j = 0;j < height;j++) for(i = 0;i < tree.argWidth;i++) {
            if(i != 0) strcat(out, ",");
            else if(j != 0) strcat(out, ";");
            strcat(out, values[i + j * tree.argWidth]);
            free(values[i + j * tree.argWidth]);
        }
        strcat(out, ">");
        return out;
    }
    //Functions
    char* argText[tree.argCount];
    int i;
    //Get function name and name length
    int strLength;
    const char* functionName = "ERROR";
    if(tree.optype == optype_builtin) {
        strLength = stdfunctions[tree.op].nameLen + 2;
        functionName = stdfunctions[tree.op].name;
    }
    if(tree.optype == optype_custom) {
        strLength = customfunctions[tree.op].nameLen + 2;
        functionName = customfunctions[tree.op].name;
    }
    //Generate the treeToString of the branches while counting the string length
    for(i = 0; i < tree.argCount; i++) {
        argText[i] = treeToString(tree.branch[i], false, argNames, localVars);
        strLength += strlen(argText[i]) + 1;
    }
    //Compile together the strings of the branches
    char* out = calloc(strLength, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    strcat(out, functionName);
    if(tree.argCount == 0) return out;
    strcat(out, "(");
    for(i = 0; i < tree.argCount; i++) {
        strcat(out, argText[i]);
        strcat(out, ",");
        free(argText[i]);
    }
    out[strLength - 2] = ')';
    return out;
}