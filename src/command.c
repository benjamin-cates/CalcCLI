//command.c contains the function runCommand
#include "general.h"
#include "compute.h"
#include "functions.h"
#include "parser.h"
#include "arb.h"
//Header files for all of these functions are in general.h
int historySize=0;
int historyCount = 0;
Value* history;
char* appendToHistory(Value num, double base, bool print) {
    if(historySize - 1 == historyCount) history = recalloc(history, &historySize, 25, sizeof(Value));
    if(globalError) return NULL;
    history[historyCount] = num;
    historyCount++;
    char* ansString = valueToString(num, base);
    if(print) {
        printf("$%d = %s\n", historyCount - 1, ansString);
        free(ansString);
        return NULL;
    }
    else {
        char* out = calloc(strlen(ansString) + 20, 1);
        snprintf(out, strlen(ansString) + 20, "$%d = %s", historyCount - 1, ansString);
        free(ansString);
        return out;
    }
}
//Header file for runCommand is in general.h
char* runCommand(char* input) {
    if(startsWith(input, "-ls")) {
        char* type = input + 4;
        if(startsWith(type, "ls")) {
            const char* lsTypes = "-ls ls ; list ls inputs\n-ls ; list custom functions";
            char* out = calloc(strlen(lsTypes) + 2, 1);
            strcpy(out, lsTypes);
            return out;
        }
        //List all custom function
        if(type[0] == 0) {
            char* functionContents[functionArrayLength];
            char* functionInputs[functionArrayLength];
            int outLen = 50;
            for(int i = 0;i < numFunctions;i++) {
                Function func = customfunctions[i];
                if(func.code.list == NULL) continue;
                outLen += 8;
                outLen += customfunctions[i].nameLen;
                if(func.code.list[0].id == action_return) functionContents[i] = treeToString(func.code.list[0].tree[0], false, func.args, NULL);
                else functionContents[i] = codeBlockToString(customfunctions[i].code, NULL, customfunctions[i].args);
                outLen += strlen(functionContents[i]);
                functionInputs[i] = argListToString(customfunctions[i].args);
                outLen += strlen(functionInputs[i]);
            }
            char* out = calloc(outLen, 1);
            int outPos = 0;
            int totalCount = 0;
            for(int i = 0;i < numFunctions;i++) {
                if(customfunctions[i].code.list == NULL) continue;
                totalCount++;
                strcpy(out + outPos, customfunctions[i].name);
                outPos += customfunctions[i].nameLen;
                bool addBrackets = functionInputs[i][0] != '(' && functionInputs[i][0] != '\0';
                if(addBrackets) out[outPos++] = '(';
                strcpy(out + outPos, functionInputs[i]);
                outPos += strlen(functionInputs[i]);
                if(addBrackets) out[outPos++] = ')';
                free(functionInputs[i]);
                strcpy(out + outPos, " = ");
                outPos += 3;
                strcpy(out + outPos, functionContents[i]);
                outPos += strlen(functionContents[i]);
                free(functionContents[i]);
                out[outPos++] = '\n';
            }
            snprintf(out + outPos, outLen - outPos, "There are %d custom functions.", totalCount);
            return out;
        }
        if(startsWith(type, "include")) {
            int outLen = 60;
            for(int i = 0;i < includeFuncsLen;i++) {
                outLen += strlen(includeFuncs[i].name);
                outLen += strlen(includeFuncs[i].arguments);
                outLen += strlen(includeFuncs[i].equation);
                outLen += 5;
            }
            char* out = calloc(outLen, 1);
            for(int i = 0;i < includeFuncsLen;i++) {
                strcat(out, includeFuncs[i].name);
                strcat(out, includeFuncs[i].arguments);
                strcat(out, " = ");
                strcat(out, includeFuncs[i].equation);
                strcat(out, "\n");
            }
            char* message = calloc(50, 1);
            snprintf(message, 50, "There are %d includable functions", includeFuncsLen);
            strcat(out, message);
            free(message);
            return out;
        }
        if(startsWith(type, "local")) {
            int outLen = 60;
            int i = -1, totalVars = 0;
            //Calculate string length and cache value to string returns
            char* equations[globalLocalVariableSize];
            char* name;
            while((name = globalLocalVariables[++i]) != NULL) {
                if(name == emptyArg) {
                    totalVars--;
                    equations[i] = NULL;
                    continue;
                }
                outLen += strlen(name) + 3;
                equations[i] = valueToString(globalLocalVariableValues[i], 10);
                outLen += strlen(equations[i]);
            }
            int numVars = i;
            totalVars += i;
            //Compile names and equations into a string list
            char* out = calloc(outLen, 1);
            int outPos = 0;
            for(i = 0;i < numVars;i++) {
                char* name = globalLocalVariables[i];
                if(name == emptyArg) continue;
                //Print name
                memcpy(out + outPos, name, strlen(name));
                outPos += strlen(name);
                //Print =
                out[outPos++] = '=';
                //Print equation and free it
                memcpy(out + outPos, equations[i], strlen(equations[i]));
                outPos += strlen(equations[i]);
                free(equations[i]);
                //Print new line
                out[outPos++] = '\n';
            }
            //Print quantity of local variables
            snprintf(out + outPos, 50, "There %s %d local variable%s", totalVars == 1 ? "is" : "are", totalVars, totalVars == 1 ? "" : "s");
            return out;
        }
        else {
            error("ls type '%s' not recognized", type);
            return NULL;
        }
    }
    if(startsWith(input, "-def")) {
        generateFunction(input + 5);
        char* out = calloc(20, 1);
        strcpy(out, "Function defined.");
        return out;
    }
    else if(startsWith(input, "-del")) {
        char* name = input + 5;
        int nameLen = strlen(name);
        Tree func = findFunction(name, false, NULL, globalLocalVariables);
        if(func.optype == optype_custom) {
            char* out = calloc(nameLen + 33, 1);
            snprintf(out, nameLen + 33, "Function '%s' has been deleted.", customfunctions[func.op].name);
            deleteCustomFunction(func.op);
            return out;
        }
        else if(func.optype == optype_localvar) {
            char* out = calloc(nameLen + 38, 1);
            snprintf(out, nameLen + 38, "Local Variable '%s' has been deleted.", globalLocalVariables[func.op]);
            freeValue(globalLocalVariableValues[func.op]);
            globalLocalVariableValues[func.op] = NULLVAL;
            free(globalLocalVariables[func.op]);
            globalLocalVariables[func.op] = emptyArg;
            return out;
        }
        else if(func.optype == optype_custom) {
            error("Function '%s' is immutable", name);
            return calloc(1, 1);
        }
        error("Function '%s' does not exist", input + 5);
        return calloc(1, 1);
    }
    else if(startsWith(input, "-quit")) {
        cleanup();
        exit(0);
        return NULL;
    }
    else if(startsWith(input, "-dx")) {
        //Clean input
        char clean[strlen(input + 4) + 1];
        strcpy(clean, input + 4);
        inputClean(clean);
        if(globalError) return NULL;
        //Set x
        char** x = calloc(2, sizeof(char*));
        if(x == NULL) { error(mallocError);return NULL; }
        x[0] = calloc(2, 1);
        if(x[0] == NULL) { error(mallocError);return NULL; }
        x[0][0] = 'x';
        //Get tree
        Tree ops = generateTree(clean, x, globalLocalVariables, 0);
        //Clean tree
        Tree cleanedOps = copyTree(ops, NULL, true, false);
        freeTree(ops);
        //Get derivative and clean it
        Tree dx = derivative(cleanedOps);
        freeTree(cleanedOps);
        Tree dxClean = copyTree(dx, NULL, false, false);
        freeTree(dx);
        //Print output
        Value ret;
        ret.type = value_func;
        ret.code = calloc(1, sizeof(CodeBlock));
        ret.code->listLen = 1;
        ret.code->list = calloc(1, sizeof(FunctionAction));
        ret.code->list[0].tree = malloc(sizeof(Tree));
        ret.code->list[0].id = 1;
        *(ret.code->list[0].tree) = dxClean;
        ret.argNames = x;
        return appendToHistory(ret, 10, false);
    }
    else if(startsWith(input, "-degset")) {
        char* type = input + 8;
        //If "rad"
        if(startsWith(type, "rad")) degrat = 1;
        //If "deg"
        else if(startsWith(type, "deg")) degrat = M_PI / 180;
        //If "grad"
        else if(startsWith(type, "grad")) degrat = M_PI / 200;
        //Else custom value
        else {
            Value deg = calculate(input + 8, 0);
            degrat = getR(deg);
            freeValue(deg);
        }
        char* out = calloc(50, 1);
        snprintf(out, 50, "Degree ratio set to %g", degrat);
        return out;
    }
    else if(startsWith(input, "-parse")) {
        //Clean and generate tree
        char clean[strlen(input + 7) + 1];
        strcpy(clean, input + 7);
        inputClean(clean);
        Tree tree = generateTree(clean, NULL, globalLocalVariables, 0);
        if(globalError) return NULL;
        //Convert to string, free, and return
        char* out = treeToString(tree, false, NULL, globalLocalVariables);
        freeTree(tree);
        return out;
    }
    else if(startsWith(input, "-setaccu")) {
        useArb = false;
        Value accu = calculate(input + 9, 10);
        double accuR = getR(accu);
        freeValue(accu);
        globalAccuracy = ((accuR + 5) * log(10) / log(256));
        digitAccuracy = accuR;
        if(accuR < 11) {
            char* out = calloc(25, 1);
            strcpy(out, "Exited accurate mode.");
            return out;
        }
        else useArb = true;
        char* out = calloc(300, 1);
        if(globalAccuracy > 30000) {
            strcat(out, "Warning: Accuracy has been capped at 60000 hexadecimal digits.\n");
            globalAccuracy = 30000;
            digitAccuracy = 72244;
        }
        else snprintf(out + strlen(out), 150, "Accuracy set to %d hexadecimal digits\n", globalAccuracy * 2);
        strcat(out, "Warning: this feature is experimental and may not be accurate. Some features are not implemented. To go back to normal mode, type \"-setaccu 0\".");
        return out;
    }
    else if(startsWith(input, "-getaccu")) {
        char* out = calloc(70, 1);
        if(!useArb) strcpy(out, "Currently not in accurate mode (13 hexadecimal digits).");
        else snprintf(out, 70, "Current accuracy is %d hexadecimal digits.", globalAccuracy);
        return out;
    }
    else if(startsWith(input, "-base")) {
        //format: -base(16) 46 will return 2E
        int i, expStart = 0;
        //Find end of base
        for(i = 5;i < strlen(input);i++) if(input[i] == ' ') {
            expStart = i + 1;
            input[i] = '\0';
            break;
        }
        //Calculate base
        Value baseVal = calculate(input + 5, 0);
        int base = getR(baseVal);
        freeValue(baseVal);
        if(base > 36 || base < 1) {
            error("base out of bounds", NULL);
            return calloc(1, 1);
        }
        //Calculate and append to history
        Value out = calculate(input + expStart, 0);
        return appendToHistory(out, base, false);
    }
    else if(startsWith(input, "-factors") || startsWith(input, "-factor")) {
        char* expression = input[7] == 's' ? input + 9 : input + 8;
        Value val = calculate(expression, 0);
        double num = getR(val);
        freeValue(val);
        if(num > 2147483647 || num < -2147483647) {
            error("%g is out of bounds", num);
        }
        int* factors = primeFactors((int)num);
        //If num is prime
        if(factors[0] == 0) {
            free(factors);
            char* out = calloc(20, 1);
            snprintf(out, 20, "%d is prime", (int)num);
            return out;
        }
        //Else list factors
        else {
            char* out = calloc(200, 1);
            snprintf(out, 200, "Factors of %d:", (int)num);
            int outPos = strlen(out);
            int outSize = 200;
            int i = -1;
            int prev = factors[0];
            int count = 0;
            //List through each factor
            while(factors[++i] != 0) {
                if(factors[i] != prev) {
                    char temp[50];
                    if(count != 1) snprintf(temp, 49, " %d^%d *", prev, count);
                    else snprintf(temp, 49, " %d *", prev);
                    int tempLen = strlen(temp);
                    if(tempLen + outPos >= outSize - 50) out = recalloc(out, &outSize, 200, 1);
                    strcpy(out + outPos, temp);
                    outPos += tempLen;
                    count = 1;
                }
                else count++;
                prev = factors[i];
            }
            if(count == 1) snprintf(out + outPos, outSize - outPos - 1, " %d", prev);
            else snprintf(out + outPos, outSize - outPos - 1, " %d^%d", prev, count);
            free(factors);
            return out;
        }
    }
    else if(startsWith(input, "-ratio")) {
        Value val = calculate(input + 7, 0);
        //If out is a number
        if(val.type == value_num) {
            char* ratioString = toStringAsRatio(val.num);
            free(appendToHistory(val, 10, false));
            int outLen = strlen(ratioString) + 10;
            char* out = calloc(outLen, 1);
            snprintf(out, outLen, "$%d = %s", historyCount - 1, ratioString);
            free(ratioString);
            return out;
        }
        //If it is a vector
        if(val.type == value_vec) {
            char* out = calloc(100, 1);
            int outSize = 100;
            free(appendToHistory(val, 10, false));
            sprintf(out, "$%d = <", historyCount - 1);
            int outPos = strlen(out);
            int width = val.vec.width;
            //Loop through all numbers
            int i, j;
            for(j = 0;j < val.vec.height;j++) for(i = 0;i < width;i++) {
                Number num = val.vec.val[i + j * width];
                //Add ; or , if applicable
                if(i == 0 && j != 0) out[outPos++] = ';';
                if(i != 0) out[outPos++] = ',';
                //Append ratio
                char* ratio = toStringAsRatio(num);
                int ratioLen = strlen(ratio);
                if(ratioLen + outPos > outSize - 10) out = recalloc(out, &outSize, 100, 1);
                strcpy(out + outPos, ratio);
                outPos += ratioLen;
            }
            out[outPos++] = '>';
            return out;
        }
        freeValue(val);

    }
    else if(startsWith(input, "-unit")) {
        int i, unitStart = 0;
        //Find end of unit
        for(i = 5;i < strlen(input);i++) if(input[i] == ' ') {
            unitStart = i + 1;
            input[i] = '\0';
            break;
        }
        //Calculate unit and value
        Value unit = calculate(input + 5, 10);
        Value value = calculate(input + unitStart, 0);
        //Check if they are compatible
        if(unit.u != value.u) {
            char* unitOne = toStringUnit(unit.u);
            char* unitTwo = toStringUnit(value.u);
            error("units %s and %s are not compatible", unitOne, unitTwo);
            free(unitOne);
            free(unitTwo);
            return NULL;
        }
        //Divide them
        Value out = valDivide(value, unit);
        //Get string
        char* numString = valueToString(out, 10);
        //Free values
        freeValue(value);
        freeValue(unit);
        freeValue(out);
        //Create output output
        int retLen = strlen(numString) + strlen(input + 5) + 10;
        char* ret = calloc(retLen, 1);
        snprintf(ret, retLen, "= %s %s", numString, input + 5);
        if(numString != NULL) free(numString);
        return ret;
    }
    else if(startsWith(input, "-include")) {
        char name[strlen(input) - 8];
        strcpy(name, input + 9);
        inputClean(name);
        int includeID = -1;
        for(int i = 0;i < includeFuncsLen;i++) {
            if(strcmp(name, includeFuncs[i].name) == 0) {
                includeID = i;
                int formLen = 5;
                formLen += strlen(includeFuncs[i].name);
                formLen += strlen(includeFuncs[i].arguments);
                formLen += strlen(includeFuncs[i].equation);
                char* formula = calloc(formLen, 1);
                strcat(formula, includeFuncs[i].name);
                strcat(formula, includeFuncs[i].arguments);
                strcat(formula, "=");
                strcat(formula, includeFuncs[i].equation);
                generateFunction(formula);
                free(formula);
                break;
            }
        }
        if(includeID == -1) {
            error("cannot find function '%s'", name);
            return NULL;
        }
        char* out = calloc(20, 1);
        strcpy(out, "Function included");
        return out;
    }
    else return NULL;
}