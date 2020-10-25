/*
    This project is maintained by Benjamin Cates (github.com/Unfit-Donkey)
    This project is available on GitHub at https://github.com/Unfit-Donkey/CalcCLI
    See README.md for more information
    See Calc.h for information on each function
    Do ./build on Mac and Linux or ./build.bat on Windows
    Then run ./calc
*/
#include "Calc.h"
#pragma region Global Variables
double degrat = 1;
int historySize;
int historyCount = 0;
bool verbose = false;
bool globalError = false;
Number NULLNUM;
Number* history;
unitStandard* unitList;
Tree NULLOPERATION;
int numFunctions = immutableFunctions;
int functionArrayLength = immutableFunctions + 10;
Function* functions;
const char metricNums[] = "yzafpnumckMGTPEZY";
const double metricNumValues[] = { 0.000000000000000000000001, 0.000000000000000000001, 0.000000000000000001, 0.000000000000001, 0.000000000001, 0.000000001, 0.000001, 0.001, 0.01, 1000, 1000000.0, 1000000000.0, 1000000000000.0, 1000000000000000.0, 1000000000000000000.0, 1000000000000000000000.0, 1000000000000000000000000.0 };
const char* baseUnits[] = { "m", "kg", "s", "A", "K", "mol", "$", "bit" };
const char numberChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#pragma endregion
#pragma region Units
unitStandard newUnit(char* name, double mult, unit_t units) {
    unitStandard out;
    out.name = name;
    out.multiplier = mult;
    out.baseUnits = units;
    return out;
}
Number getUnitName(char* name) {
    int useMetric = -1;
    int i;
    for(i = 0; i < metricCount; i++)
        if(name[0] == metricNums[i]) {
            useMetric = i;
            break;
        }
    for(i = 0; i < unitCount; i++) {
        if(useMetric != -1 && unitList[i].multiplier < 0)
            if(strcmp(name + 1, unitList[i].name) == 0) {
                return newValue(fabs(unitList[i].multiplier) * metricNumValues[useMetric], 0, unitList[i].baseUnits);
            }
        if(strcmp(name, unitList[i].name) == 0) {
            return newValue(fabs(unitList[i].multiplier), 0, unitList[i].baseUnits);
        }
    }
    return newValue(1, 0, 0);
}
char* toStringUnit(unit_t unit) {
    if(unit == 0)
        return NULL;
    int i;
    //Find applicable metric unit
    for(i = 0; i < unitCount; i++) {
        if(unitList[i].multiplier == -1 && unit == unitList[i].baseUnits) {
            //Copy its name into a dynamic memory address
            char* out = calloc(strlen(unitList[i].name), 1);
            strcat(out, unitList[i].name);
            return out;
        }
    }
    //Else generate a custom string
    char* out = calloc(54, 1);
    bool mult = false;
    for(i = 0; i < 8; i++) {
        char val = (unit >> (i * 8)) & 255;
        if(val != 0) {
            if(mult)
                strcat(out, "*");
            strcat(out, baseUnits[i]);
            if(val != 1) {
                strcat(out, "^");
                char buffer[5];
                snprintf(buffer, 4, "%d", val);
                strcat(out, buffer);
            }
            mult = true;
        }
    }
    return out;
}
unit_t unitInteract(unit_t one, unit_t two, char op, double twor) {
    int i;
    if(op == '^') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(char)((float)o * twor)) << (i * 8);
        }
        return out;
    }
    if(two == 0)
        return one;
    if(op == '/') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            char t = (two >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(o - t)) << (i * 8);
        }
        return out;
    }
    if(one == 0)
        return two;
    if(op == '+') {
        if(one != two) {
            error("Cannot add different units", NULL);
        }
        return one;
    }
    if(op == '*') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            char t = (two >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(o + t)) << (i * 8);
        }
        return out;
    }
    return 0;
}
#pragma endregion
#pragma region Numbers
Number newValue(double r, double i, unit_t u) {
    Number out;
    out.r = r;
    out.i = i;
    out.u = u;
    return out;
}
double parseNumber(char* num, double base) {
    int i;
    int numLength = strlen(num);
    int periodPlace = numLength;
    double out = 0;
    double power = 1;
    for(i = 0; i < numLength; i++)
        if(num[i] == '.') {
            periodPlace = i;
            break;
        }
    for(i = periodPlace - 1; i > -1; i--) {
        double n = (double)(num[i] - 48);
        if(n > 10)
            n -= 7;
        out += power * n;
        power *= base;
    }
    power = 1 / base;
    for(i = periodPlace + 1; i < numLength; i++) {
        double n = (double)(num[i] - 48);
        if(n > 10)
            n -= 7;
        out += power * n;
        power /= base;
    }
    return out;
}
char* toStringNumber(Number num, double base) {
    char* real = doubleToString(num.r, base);
    char* imag = doubleToString(num.i, base);
    char* unit = toStringUnit(num.u);
    int outLength = (num.u != 0 ? strlen(unit) + 2 : 0) + (num.i == 0 ? 0 : strlen(imag)) + strlen(real) + 3;
    char* out = calloc(outLength, 1);
    memset(out, 0, outLength);
    if(num.r != 0 || num.i == 0) strcat(out, real);
    if(num.r != 0 && num.i > 0) strcat(out, "+");
    if(num.i != 0) {
        strcat(out, imag);
        strcat(out, "i");
    }
    if(num.u != 0) {
        strcat(out, "[");
        strcat(out, unit);
        strcat(out, "]");
    }
    free(real);
    free(imag);
    free(unit);
    return out;
}
char* doubleToString(double num, double base) {
    if(num == 0 || base <= 1 || base > 36) {
        char* out = calloc(2, 1);
        out[0] = '0';
        return out;
    }
    //Allocate string
    char* out = calloc(24, 1);
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
    if(magnitude > 14 || magnitude < -14) exp = floor(magnitude);
    if(exp != 0) {
        snprintf(expString, 7, "e%d", exp);
        expLen = strlen(expString);
        num /= pow(base, exp);
    }
    //Calculate power of highest digit
    double power;
    if(exp == 0) power = pow(base, floor(magnitude));
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
void appendToHistory(Number num, double base, bool print) {
    if(historySize - 1 == historyCount) {
        history = realloc(history, (historySize + 25) * sizeof(Number));
        historySize += 25;
    }
    char* ansString = toStringNumber(num, base);
    if(print) printf("$%d = %s\n", historyCount, ansString);
    free(ansString);
    history[historyCount] = num;
    historyCount++;
}
Number compMultiply(Number one, Number two) {
    return newValue(one.r * two.r - one.i * two.i, one.r * two.i + one.i * two.r, unitInteract(one.u, two.u, '*', 0));
}
Number compPower(Number one, Number two) {
    double logabs = log(one.r * one.r + one.i * one.i);
    double arg = atan2(one.i, one.r);
    if(one.r == 0 && one.i == 0)
        arg = 0;
    double p1 = exp(two.r * 0.5 * logabs - two.i * arg);
    double cis = two.i * 0.5 * logabs + two.r * arg;
    if(isnan(cis))
        cis = 0;
    return newValue(p1 * cos(cis), p1 * sin(cis), unitInteract(one.u, two.u, '^', two.r));
}
Number compDivide(Number one, Number two) {
    double denominator = two.r * two.r + (two.i * two.i);
    double numeratorR = one.r * two.r + one.i * two.i;
    double numeratorI = one.r * -two.i + one.i * two.r;
    return newValue(numeratorR / denominator, numeratorI / denominator, unitInteract(one.u, two.u, '/', 0));
}
Number compSine(Number one) {
    return newValue(sin(one.r) * cosh(one.i), cos(one.r) * sinh(one.i), one.u);
}
Number compSubtract(Number one, Number two) {
    return newValue(one.r - two.r, one.i - two.i, unitInteract(one.u, two.u, '+', 0));
}
Number compSqrt(Number one) {
    double abs = sqrt(one.r * one.r + one.i * one.i);
    double sgnI = one.i / fabs(one.i);
    if(one.i == 0) sgnI = 1;
    return newValue(sqrt((abs + one.r) / 2), sgnI * sqrt((abs - one.r) / 2), unitInteract(one.u, 0, '^', 0.5));
}
Number compGamma(Number one) {
    //https://en.wikipedia.org/wiki/Lanczos_approximation#Simple_implementation
    // Reflection formula
    if(one.r < 0.5) {
        //return pi / (sin(pi * z) * gamma(1 - z))
        one.r *= 3.14159265358979323;
        one.i *= 3.14159265358979323;
        Number denom = compSine(one);
        one.r = 1 - (one.r / 3.14159265358979323);
        one.i = -(one.i / 3.14159265358979323);
        denom = compMultiply(denom, compGamma(one));
        double denomSq = denom.r * denom.r + denom.i * denom.i;
        if(denomSq == 0) {
            error("cannot call factorial of a negative integer", NULL);
            return NULLNUM;
        }
        return newValue(3.14159265358979323 * denom.r / denomSq, 3.14159265358979323 * -denom.i / denomSq, one.u);
    }
    int g = 8;
    double p[] = { 676.5203681218851, -1259.1392167224028, 771.32342877765313, -176.61502916214059, 12.507343278686905, -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7 };
    Number x = newValue(0.99999999999980993, 0, 0);
    int i;
    for(i = 0; i < g; i++) {
        double denom = (one.r + i) * (one.r + i) + one.i * one.i;
        x.r += (p[i] * (one.r + i)) / denom;
        x.i += (p[i] * -one.i) / denom;
    }
    Number t = one;
    t.r += g - 1.5;
    one.r -= 0.5;
    Number out = compMultiply(compPower(t, one), x);
    double ex = exp(-t.r) * 2.5066282746310002;
    double exr = cos(-t.i) * ex;
    double exi = sin(-t.i) * ex;
    return newValue(out.r * exr - out.i * exi, out.i * exr + out.r * exi, one.u);
}
#pragma endregion
#pragma region Trees
//Comparison
bool treeIsZero(Tree in) {
    if(in.op != op_val)
        return false;
    if(in.value.r != 0)
        return false;
    if(in.value.i != 0)
        return false;
    return true;
}
bool treeIsOne(Tree in) {
    if(in.op != op_val)
        return false;
    if(in.value.r != 1)
        return false;
    if(in.value.i != 0)
        return false;
    return true;
}
bool treeEqual(Tree one, Tree two) {
    if(one.op != two.op)
        return false;
    if(one.op == op_val) {
        if(one.value.r != two.value.r)
            return false;
        if(one.value.i != two.value.i)
            return false;
        if(one.value.u != two.value.u)
            return false;
        return true;
    }
    if(one.argCount != two.argCount)
        return false;
    int i;
    for(i = 0; i < one.argCount; i++)
        if(!treeEqual(one.branch[i], two.branch[i]))
            return false;
    return true;
}
//Constructors
Tree newOperation(Tree* branches, int argCount, int opID) {
    Tree out;
    out.branch = branches;
    out.argCount = argCount;
    out.op = opID;
    return out;
}
Tree newOpValue(Number value) {
    Tree out;
    out.op = op_val;
    out.value = value;
    return out;
}
Tree newOpVal(double r, double i, unit_t u) {
    Tree out;
    out.op = op_val;
    out.value.r = r;
    out.value.i = i;
    out.value.u = u;
    return out;
}
Tree* allocateArgs(Tree one, Tree two, bool copyOne, bool copyTwo) {
    Tree* out = malloc(2 * sizeof(Tree));
    if(copyOne)
        out[0] = treeCopy(one, NULL, 0, 0, 0);
    else
        out[0] = one;
    if(copyTwo)
        out[1] = treeCopy(two, NULL, 0, 0, 0);
    else
        out[1] = two;
    return out;
}
Tree* allocateArg(Tree one, bool copy) {
    Tree* out = malloc(sizeof(Tree));
    if(copy)
        out[0] = treeCopy(one, NULL, 0, 0, 0);
    else
        out[0] = one;
    return out;
}
//Recursive functions
void freeTree(Tree tree) {
    //Frees a tree's args
    if(tree.op != op_val && tree.branch != NULL) {
        int i = 0;
        for(i = 0; i < tree.argCount; i++)
            freeTree(tree.branch[i]);
        free(tree.branch);
    }
}
char* treeToString(Tree tree, bool bracket, char* argNames) {
    //Numbers
    if(tree.op == op_val)
        return toStringNumber(tree.value, 10);
    //Arguments
    if(tree.op < 0) {
        if(argNames == NULL) {
            char* num = calloc(10, 1);
            sprintf(num, "{%d}", -tree.op - 1);
            return num;
        }
        else {
            char* num = calloc(2, 1);
            num[0] = argNames[-tree.op - 1];
            return num;
        }
    }
    //Operations
    if(tree.op > 1 && tree.op < 9) {
        char* op;
        if(tree.op == op_neg || tree.op == op_sub) op = "-";
        if(tree.op == op_pow) op = "^";
        if(tree.op == op_mod) op = "%";
        if(tree.op == op_mult) op = "*";
        if(tree.op == op_div) op = "/";
        if(tree.op == op_add) op = "+";
        if(tree.branch == NULL) {
            char* out = calloc(10, 1);
            snprintf(out, 10, "NULL%sNULL", op);
            return out;
        }
        //Tostring one and two
        char* one = treeToString(tree.branch[0], true, argNames);
        char* two = "";
        if(tree.op != op_neg) two = treeToString(tree.branch[1], true, argNames);
        //Allocate string
        int len = strlen(one) + strlen(two) + 2 + bracket * 2;
        char* out = calloc(len, 1);
        //Print -(one) if tree.op==op_neg, or (one op two) otherwise
        snprintf(out, len, "%s%s%s%s%s%s", tree.op == op_neg ? "-" : "", bracket ? "(" : "", one, tree.op == op_neg ? "" : op, two, bracket ? ")" : "");
        //Free one and two, return
        free(one);
        if(tree.argCount == 2) free(two);
        return out;
    }
    //Functions
    char* argText[tree.argCount];
    int i;
    int strLength = functions[tree.op].nameLen + 2;
    char* argNamesTemp = argNames;
    //N argument name for sum and product
    if(tree.op == op_sum || tree.op == op_product) {
        int len = 0;
        if(argNames != NULL) len += strlen(argNames);
        argNamesTemp = calloc(len + 2, 1);
        if(argNames != NULL) memcpy(argNamesTemp, argNames, len);
        argNamesTemp[len] = 'n';
    }
    for(i = 0; i < tree.argCount; i++) {
        argText[i] = treeToString(tree.branch[i], false, argNamesTemp);
        strLength += strlen(argText[i]) + 1;
    }
    char* out = calloc(strLength, 1);
    strcat(out, functions[tree.op].name);
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
Number computeTree(Tree tree, Number* args, int argLen) {
    //Returns a number from a tree and its arguments
    if(tree.op == op_val)
        return tree.value;
    if(tree.op < op_val) {
        if(argLen < -tree.op) error("argument error", NULL);
        return args[-tree.op - 1];
    }
    if(tree.argCount != functions[tree.op].argCount) {
        error("Argument mismatch", NULL);
        return NULLNUM;
    }
    //Sum and product
    if(tree.op < 65 && tree.op > 62) {
        Number sumArgs[argLen + 1];
        memset(sumArgs, 0, (argLen + 1) * sizeof(Number));
        if(args != NULL) memcpy(sumArgs, args, sizeof(Number) * argLen);
        Number start = computeTree(tree.branch[1], args, argLen);
        Number end = computeTree(tree.branch[2], args, argLen);
        Number step = computeTree(tree.branch[3], args, argLen);
        Number out;
        if((end.r - start.r) > step.r * 100 || end.r < start.r)
            return NULLNUM;
        if(tree.op == op_sum) {
            out = newValue(0, 0, 0);
            sumArgs[argLen] = newValue(0, 0, 0);
            double i;
            for(i = start.r;i <= end.r;i += step.r) {
                sumArgs[argLen].r = i;
                Number current = computeTree(tree.branch[0], sumArgs, argLen + 1);
                out.u = current.u;
                out.r += current.r;
                out.i += current.i;
            }
        }
        if(tree.op == op_product) {
            out = newValue(1, 0, 0);
            double i;
            for(i = start.r;i <= end.r;i += step.r) {
                sumArgs[argLen].r = i;
                Number current = computeTree(tree.branch[0], sumArgs, argLen + 1);
                double temp = out.r * current.r - out.i * current.i;
                out.i = out.i * current.r + out.r * current.i;
                out.r = temp;
                out.u = current.u;
            }
        }
        return out;
    }
    Number one, two;
    if(tree.argCount > 0)
        one = computeTree(tree.branch[0], args, argLen);
    if(tree.argCount > 1)
        two = computeTree(tree.branch[1], args, argLen);
    if(globalError)
        return NULLNUM;
    //Basic operators
    if(tree.op < 9) {
        if(tree.op == op_i)
            return newValue(0, 1, 0);
        if(tree.op == op_neg) {
            one.r = -one.r;
            one.i = -one.i;
            return one;
        }
        if(tree.op == op_pow)
            return compPower(one, two);
        if(tree.op == op_mod)
            return newValue(fmod(one.r, two.r), fmod(one.i, two.r), unitInteract(one.u, two.u, '+', 0));
        if(tree.op == op_mult)
            return compMultiply(one, two);
        if(tree.op == op_div)
            return compDivide(one, two);
        if(tree.op == op_add)
            return newValue(one.r + two.r, one.i + two.i, unitInteract(one.u, two.u, '+', 0));
        if(tree.op == op_sub)
            return newValue(one.r - two.r, one.i - two.i, unitInteract(one.u, two.u, '+', 0));
    }
    //Trigonometric functions
    if(tree.op < 27) {
        //Trigonometric functions
        if(tree.op < 18) {
            one.r *= degrat;
            one.i *= degrat;
            if(tree.op == op_sin)
                return compSine(one);
            if(tree.op == op_cos)
                return newValue(cos(one.r) * cosh(one.i), sin(one.r) * sinh(one.i), one.u);
            if(tree.op == op_tan)
                return compDivide(compSine(one), newValue(cos(one.r) * cosh(one.i), sin(one.r) * sinh(one.i), 0));
            Number out;
            bool reciprocal = false;
            // csc, sec, tan
            if(tree.op > 11) {
                reciprocal = true;
                tree.op -= 3;
            }
            if(tree.op == op_sin)
                out = compSine(one);
            if(tree.op == op_cos)
                out = newValue(cos(one.r) * cosh(one.i), sin(one.r) * sinh(one.i), one.u);
            if(tree.op == op_tan)
                out = compDivide(compSine(one), newValue(cos(one.r) * cosh(one.i), sin(one.r) * sinh(one.i), 0));
            if(reciprocal) {
                double denom = out.r * out.r + (out.i * out.i);
                out.r /= denom;
                out.i /= -denom;
            }
            return out;
        }
        //Inverse trigonometric functions
        else {
            //https://proofwiki.org/wiki/Definition:Inverse_Sine/Arcsine
            int opID = tree.op;
            Number out;
            if(tree.op > 20 && tree.op < 24) {
                unit_t unit = one.u;
                one = compDivide(newValue(1, 0, 0), one);
                one.u = unit;
                opID -= 3;
            }
            if(opID == op_asin || opID == op_acos) {
                Number Sq = compSqrt(newValue(1 - one.r * one.r - one.i * one.i, -2 * one.r * one.i, 0));
                double temp = one.r;
                one.r = Sq.r - one.i;
                one.i = Sq.i + temp;
                if(opID == 19)
                    out = newValue(1.570796326794896619 - atan2(one.i, one.r), 0.5 * log(one.r * one.r + one.i * one.i), one.u);
                else out = newValue(atan2(one.i, one.r), -0.5 * log(one.r * one.r + one.i * one.i), one.u);
            }
            if(opID == op_tan) {
                Number out = compDivide(newValue(-one.r, 1 - one.i, 0), newValue(one.r, 1 + one.i, 0));
                out = newValue(0.5 * atan2(out.i, out.r), -0.25 * log(out.r * out.r + out.i * out.i), one.u);
            }
            //Hyperbolic Arccosine and hyperbolic sine
            if(tree.op == op_acosh || tree.op == op_asinh) {
                double sqR = one.r * one.r - one.i * one.i;
                double sqI = one.r * one.i * 2;
                if(tree.op == op_asinh) sqR += 1;
                else sqR -= 1;
                Number sqrt = compSqrt(newValue(sqR, sqI, 0));
                sqrt.r += one.r;
                sqrt.i += one.i;
                out = newValue(0.5 * log(sqrt.r * sqrt.r + sqrt.i * sqrt.i), atan2(sqrt.i, sqrt.r), one.u);
            }
            //Hyperbolic Arctangent
            if(tree.op == op_atanh) {
                Number p1 = newValue(one.r + 1, one.i, 0);
                Number p2 = newValue(1 - one.r, -one.i, 0);
                Number div = compDivide(p1, p2);
                out = newValue(0.25 * log(div.r * div.r + div.i * div.i), 0.5 * atan2(div.i, div.r), one.u);
            }
            out.r /= degrat;
            out.i /= degrat;
            return out;
        }
    }
    //Log, rounding, arg, and abs
    if(tree.op < 38) {
        if(tree.op == op_sqrt) return compPower(one, newValue(0.5, 0, 0));
        if(tree.op == op_cbrt) return compPower(one, newValue(0.333333333333333333, 0, 0));
        if(tree.op == op_exp) {
            double epowr = exp(one.r);
            return newValue(epowr * cos(one.i), epowr * sin(one.i), one.u);
        }
        if(tree.op == op_ln)
            return newValue(0.5 * log(one.r * one.r + one.i * one.i), atan2f(one.i, one.r), one.u);
        if(tree.op == op_logten) {
            double logten = 1 / log(10);
            return newValue(0.5 * log(one.r * one.r + one.i * one.i) * logten, atan2f(one.i, one.r), one.u);
        }
        if(tree.op == op_log) {
            one = newValue(0.5 * log(one.r * one.r + one.i * one.i), atan2f(one.i, one.r), one.u);
            two = newValue(0.5 * log(two.r * two.r + two.i * two.i), atan2f(two.i, two.r), two.u);
            return compDivide(one, two);
        }
        if(tree.op == op_fact) {
            one.r += 1;
            return compGamma(one);
        }
        if(tree.op == op_sgn) {
            if(one.r > 0)
                one.r = 1;
            if(one.r < 0)
                one.r = -1;
            if(one.i > 0)
                one.i = 1;
            if(one.i < 0)
                one.i = -1;
            return one;
        }
        if(tree.op == op_abs)
            return newValue(sqrt(one.i * one.i + one.r * one.r), 0, one.u);
        if(tree.op == op_arg)
            return newValue(atan2(one.i, one.r), 0, one.u);
        return NULLNUM;
    }
    //Rounding and conditionals
    if(tree.op < 50) {
        if(tree.op == op_round) {
            one.r = round(one.r);
            one.i = round(one.i);
            return one;
        }
        if(tree.op == op_floor) {
            one.r = floor(one.r);
            one.i = floor(one.i);
            return one;
        }
        if(tree.op == op_ceil) {
            one.r = ceil(one.r);
            one.i = ceil(one.i);
            return one;
        }
        if(tree.op == op_getr) {
            one.i = 0;
            return one;
        }
        if(tree.op == op_geti) {
            one.r = one.i;
            one.i = 0;
            return one;
        }
        if(tree.op == op_getu) {
            one.r = 1;
            one.i = 0;
            return one;
        }
        if(tree.op == op_grthan) {
            if(one.r > two.r)
                return newValue(1, 0, 0);
            else
                return newValue(0, 0, 0);
        }
        if(tree.op == op_equal) {
            if(one.r == two.r && one.r == two.r)
                return newValue(1, 0, 0);
            else
                return newValue(0, 0, 0);
        }
        if(tree.op == op_min) {
            if(one.r > two.r)
                return two;
            else
                return one;
        }
        if(tree.op == op_max) {
            if(one.r > two.r)
                return one;
            else
                return two;
        }
        if(tree.op == op_lerp) {
            //(1 - c) * one + c * two;
            Number c = computeTree(tree.branch[2], args, argLen);
            Number p1 = newValue((1 - c.r) * one.r + one.i * c.i, (1 - c.r) * one.i - one.r * c.i, 0);
            Number p2 = newValue(c.r * two.r - c.i * two.i, c.r * two.i + two.r * c.i, 0);
            return newValue(p1.r + p2.r, p1.i + p2.i, unitInteract(one.u, two.u, '+', 0));
        }
        if(tree.op == op_dist) {
            return newValue(sqrt(pow(fabs(one.r - two.r), 2) + pow(fabs(one.i - two.i), 2)), 0, 0);
        }
    }
    //Binary Operations
    if(tree.op < 56) {
        if(tree.op == op_not) {
            if(one.r != 0)
                one.r = (double)~((long)one.r);
            if(one.i != 0)
                one.i = (double)~((long)one.i);
            return one;
        }
        else if(tree.op == op_and) {
            one.r = (double)(((long)one.r) & ((long)two.r));
            one.i = (double)(((long)one.i) & ((long)two.i));
        }
        else if(tree.op == op_or) {
            one.r = (double)(((long)one.r) | ((long)two.r));
            one.i = (double)(((long)one.i) | ((long)two.i));
        }
        else if(tree.op == op_xor) {
            one.r = (double)(((long)one.r) ^ ((long)two.r));
            one.i = (double)(((long)one.i) ^ ((long)two.i));
        }
        else if(tree.op == op_ls) {
            one.r = (double)(((long)one.r) << ((long)two.r));
            one.i = (double)(((long)one.i) << ((long)two.i));
        }
        else if(tree.op == op_rs) {
            one.r = (double)(((long)one.r) >> ((long)two.r));
            one.i = (double)(((long)one.i) >> ((long)two.i));
        }
        return one;
    }
    //Constants
    if(tree.op < 63) {
        if(tree.op == op_pi)
            return newValue(3.14159265358979, 0, 0);
        if(tree.op == op_e)
            return newValue(1.618033988749894, 0, 0);
        if(tree.op == op_phi)
            return newValue(2.718281828459045, 0, 0);
        if(tree.op == op_ans) {
            if(historyCount == 0) {
                error("no previous answer", NULL);
                return NULLNUM;
            }
            return history[historyCount - 1];
        }
        if(tree.op == op_hist) {
            if(one.r < 0) {
                if((int)floor(-one.r) > historyCount) {
                    error("history too short", NULL);
                    return NULLNUM;
                }
                return history[historyCount - (int)floor(-one.r)];
            }
            if((int)floor(one.r) >= historyCount) {
                error("history too short", NULL);
                return NULLNUM;
            }
            return history[(int)floor(one.r)];
        }
        if(tree.op == op_histnum)
            return newValue(historyCount, 0, 0);
        if(tree.op == op_rand)
            return newValue((double)rand() / RAND_MAX, 0, 0);
    }
    //Custom functions
    if(functions[tree.op].tree == NULL) {
        error("this uses a nonexistent function", NULL);
        return NULLNUM;
    }
    Number funcArgs[tree.argCount];
    int i;
    //Crunch args
    if(tree.argCount > 0)
        funcArgs[0] = one;
    if(tree.argCount > 1)
        funcArgs[1] = two;
    for(i = 2; i < tree.argCount; i++)
        funcArgs[i] = computeTree(tree.branch[i], args, argLen);
    Number out = computeTree(*functions[tree.op].tree, funcArgs, tree.argCount);
    if(globalError)
        return NULLNUM;
    return out;
}
Tree treeCopy(Tree tree, Tree* args, bool unfold, bool replaceArgs, bool optimize) {
    Tree out = tree;
    //Example: if f(x)=x^2, copyTree(f(2x),NULL,true,false,false) will return (2x)^2
    //Replace arguments
    if(replaceArgs)
        if(tree.op < 0) {
            return treeCopy(args[-tree.op - 1], NULL, unfold, false, optimize);
        }
    if(optimize && out.op > 0 && out.argCount == 0)
        return newOpValue(computeTree(tree, NULL, 0));
    //Copy tree args
    if(tree.argCount != 0) {
        out.branch = malloc(tree.argCount * sizeof(Tree));
        int i;
        bool crunch = true;
        for(i = 0; i < tree.argCount; i++) {
            out.branch[i] = treeCopy(tree.branch[i], args, unfold, replaceArgs, optimize);
            if(out.branch[i].op < 0 || out.branch[i].argCount != 0)
                crunch = false;
        }
        if(crunch && optimize) {
            Tree ret = newOpValue(computeTree(tree, NULL, 0));
            freeTree(out);
            return ret;
        }
    }
    //Unfold custom functions
    if(unfold)
        if(tree.op >= immutableFunctions) {
            Tree ret = treeCopy(*functions[tree.op].tree, out.branch, true, true, optimize);
            free(out.branch);
            return ret;
        }
    //Return
    return out;
}
Tree generateTree(char* eq, char* argNames, double base) {
    bool useUnits = base != 0;
    if(base == 0) base = 10;
    if(verbose) {
        printf("Generating Operation Tree");
        if(useUnits)
            printf(" (units, base=%g)", base);
        printf("\nInput: %s\n", eq);
    }
    if(strlen(eq) == 0) {
        error("no equation", NULL);
        return NULLOPERATION;
    }
    //Divide into sections
    int i, brackets = 0, sectionCount = 0, pType = 0, eqLength = strlen(eq);
    int sections[eqLength + 1];
    memset(sections, 0, sizeof(sections));
    for(i = 0; i < eqLength; i++) {
        //pTypes: 1: numbers, 2: operators (+-*/), 3: variable or function
        if(eq[i] == '(' || eq[i] == '[') {
            if(brackets++ == 0 && pType != 3)
                sections[sectionCount++] = i;
            pType = 0;
        }
        if(eq[i] == ')')
            if(--brackets < 0) {
                error("bracket mismatch", NULL);
                return NULLOPERATION;
            }
        if(eq[i] == ']') {
            if(brackets-- < 0) {
                error("bracket mismatch", NULL);
                return NULLOPERATION;
            }
            if(i != eqLength - 1)
                if(eq[i + 1] == '_') {
                    if(i == eqLength - 2) {
                        error("floating underscore", NULL);
                        return NULLOPERATION;
                    }
                    if(eq[i + 2] >= '0' && eq[i + 2] <= '9')
                        pType = 1;
                    else if(eq[i + 2] >= 'a' && eq[i + 2] <= 'z')
                        pType = 3;
                    else if(eq[i + 2] == '(') {
                        brackets++;
                        i++;
                    }
                    else
                        pType = 2;
                    i++;
                    continue;
                }
        }
        if(brackets == 0) {
            if(((eq[i] >= '0' && eq[i] <= '9') || eq[i] == '.') && pType != 1) {
                pType = 1;
                sections[sectionCount++] = i;
            }
            else if(((eq[i] >= '*' && eq[i] <= '/' && eq[i] != '.') || eq[i] == '%' || eq[i] == '^') && pType != 2) {
                pType = 2;
                sections[sectionCount++] = i;
            }
            else if(((eq[i] >= 'a' && eq[i] <= 'z') || (useUnits && ((eq[i] >= 'A' && eq[i] <= 'Z') || eq[i] == '$') && (pType != 1 || (eq[i] > 'A' + (int)base - 10)))) && pType != 3) {
                if(i != 0 && eq[i - 1] == '0' && (eq[i] == 'b' || eq[i] == 'x' || eq[i] == 'd' || eq[i] == 'o'))
                    continue;
                pType = 3;
                sections[sectionCount++] = i;
            }
        }
        if(i == 0 && sectionCount == 0)
            sectionCount++;
    }
    sections[sectionCount] = strlen(eq);
    if(brackets != 0) {
        error("bracket mismatch", NULL);
        return NULLOPERATION;
    }
    //Generate operations from strings
    Tree ops[sectionCount];
    if(verbose)
        printf("Sections(%d): ", sectionCount);
    //set to true when it comes across *- or /- or ^- or %-
    bool nextNegative = false;
    for(i = 0; i < sectionCount; i++) {
        //Get section substring
        int j, sectionLength = sections[i + 1] - sections[i];
        char section[sectionLength + 1];
        for(j = sections[i]; j < sections[i + 1]; j++)
            section[j - sections[i]] = eq[j];
        section[sectionLength] = '\0';
        if(verbose)
            printf("%s, ", section);
        //Parse section
        char first = eq[sections[i]];
        if(first >= '0' && first <= '9') {
            //Number
            double numBase = base;
            bool useBase = false;
            if(first == '0') {
                if(eq[sections[i] + 1] >= 'a')
                    useBase = true;
                if(eq[sections[i] + 1] == 'b')
                    numBase = 2;
                if(eq[sections[i] + 1] == 'o')
                    numBase = 8;
                if(eq[sections[i] + 1] == 'd')
                    numBase = 10;
                if(eq[sections[i] + 1] == 'x')
                    numBase = 16;
            }
            double numr = parseNumber(section + (useBase ? 2 : 0), numBase);
            ops[i] = newOpVal(numr, 0, 0);
        }
        else if(first >= 'a' && first <= 'z' || (useUnits && ((first >= 'A' && first <= 'Z') || first == '$'))) {
            if(eq[sections[i + 1] - 1] == ')') {
                //Function
                int j;
                int commas[sectionLength];
                memset(commas, 0, sizeof(commas));
                int commaCount = 1;
                int openBracket = 0;
                int brackets = 0;
                //Iterate through all characters to find ','
                for(j = 0; j < sectionLength; j++) {
                    if(openBracket == 0 && section[j] == '(')
                        openBracket = j;
                    if(section[j] == '(')
                        brackets++;
                    if(section[j] == ')')
                        brackets--;
                    if(section[j] == ',' && brackets == 1)
                        commas[commaCount++] = j;
                }
                commas[0] = openBracket;
                commas[commaCount] = sectionLength - 1;
                char name[openBracket + 1];
                memcpy(name, section, openBracket);
                name[openBracket] = '\0';
                int funcID = findFunction(name);
                if(funcID == 0) {
                    error("function '%s' does not exist", name);
                    return NULLOPERATION;
                }
                if(functions[funcID].argCount != commaCount) {
                    error("Wrong number of arguments for '%s'", functions[funcID].name);
                    return NULLOPERATION;
                }
                Tree* args = calloc(commaCount, sizeof(Tree));
                for(j = 0; j < commaCount; j++) {
                    char argText[commas[j + 1] - commas[j] + 1];
                    memset(argText, 0, sizeof(argText));
                    memcpy(argText, section + commas[j] + 1, commas[j + 1] - commas[j] - 1);
                    char* argNameTemp = argNames;
                    //N variable in sum and product
                    if(j == 0) if(funcID == op_sum || funcID == op_product) {
                        int len = 0;
                        if(argNames != NULL) len = strlen(argNames);
                        argNameTemp = calloc(len + 2, 1);
                        memcpy(argNameTemp, argNames, len);
                        argNameTemp[len] = 'n';
                    }
                    args[j] = generateTree(argText, argNameTemp, 0);
                    if(argNameTemp != argNames) free(argNameTemp);
                    if(globalError) {
                        free(args);
                        return NULLOPERATION;
                    }
                }
                ops[i] = newOperation(args, commaCount, funcID);
            }
            else {
                int opID = findFunction(section);
                Number num = NULLNUM;
                if(sectionLength == 1 && argNames != NULL) {
                    int j;
                    for(j = 0; j < strlen(argNames); j++)
                        if(argNames[j] == section[0])
                            opID = -j - 1;
                }
                if(opID == 0 && useUnits) {
                    num = getUnitName(section);
                    if(num.u == 0) {
                        error("Variable or unit '%s' not found", section);
                        return NULLOPERATION;
                    }
                }
                else if(opID == 0) {
                    error("Variable '%s' not found", section);
                    return NULLOPERATION;
                }
                if(opID > 0)
                    if(functions[opID].argCount != 0) {
                        error("no arguments for function '%s'", section);
                        return NULLOPERATION;
                    }
                if(opID == 0)
                    ops[i] = newOpValue(num);
                else
                    ops[i] = newOperation(NULL, 0, opID);
            }
        }
        else if((first > 41 && first < 48) || first == '^' || first == '%') {
            int opID = 0;
            //For negative operations eg. *-
            if(strlen(section) > 1) {
                if(section[1] == '-')
                    nextNegative = true;
                else
                    error("'%s' not a valid operator", section);
            }
            if(first == '-' && i == 0) {
                ops[i] = newOpVal(1, 0, 0);
                nextNegative = true;
                continue;
            }
            if(first == '^')
                opID = op_pow;
            else if(first == '%')
                opID = op_mod;
            else if(first == '*' && section[1] == '*')
                opID = op_pow;
            else if(first == '*')
                opID = op_mult;
            else if(first == '/')
                opID = op_div;
            else if(first == '+')
                opID = op_add;
            else if(first == '-')
                opID = op_sub;
            else
                error("'%s' is not a valid operator", section);
            ops[i] = newOperation(NULL, 0, opID);
            continue;
        }
        else if(first == '(') {
            //Round bracket
            section[sectionLength - 1] = '\0';
            char exp[sectionLength - 1];
            memset(exp, 0, sizeof(exp));
            memcpy(exp, section + 1, sectionLength - 2);
            ops[i] = generateTree(exp, argNames, 0);
            if(globalError)
                return NULLOPERATION;
        }
        else if(first == '[') {
            //Unit bracket
            double base = 10;
            if(section[sectionLength - 1] != ']') {
                //use alternate base
                int j, brackets = 0;
                for(j = sectionLength - 1; j >= 0; j--) {
                    if(section[j] == ')')
                        brackets++;
                    if(section[j] == '(')
                        brackets--;
                    if(brackets == 0 && section[j] == '_')
                        break;
                }
                if(j == 0)
                    error("could not find underscore", NULL);
                Tree tree = generateTree(section + j + 1, NULL, 0);
                if(globalError) {
                    error("Base type must be a runtime constant", NULL);
                    return NULLOPERATION;
                }
                Number baseNum = computeTree(tree, NULL, 0);
                if(globalError)
                    return NULLOPERATION;
                freeTree(tree);
                base = baseNum.r;
                section[j - 1] = '\0';
            }
            else
                section[sectionLength - 1] = '\0';
            char exp[strlen(section) + 1];
            memset(exp, 0, sizeof(exp));
            memcpy(exp, section + 1, strlen(section) - 1);
            ops[i] = generateTree(exp, argNames, base);
            if(globalError)
                return NULLOPERATION;
        }
        else {
            error("unable to parse '%s'", section);
            return NULLOPERATION;
        }
        if(nextNegative && !(first > 41 && first < 48) && first != '^' && first != '%') {
            nextNegative = false;
            ops[i] = newOperation(allocateArg(ops[i], false), 1, op_neg);
        }
    }
    //Compile operations into operation tree
    int operationCount = sectionCount;
    int totalOpsCount = operationCount;
    if(verbose) {
        printf("\nOperation List(%d): ", sectionCount);
        for(i = 0; i < operationCount; i++) {
            if(i != 0)
                printf(", ");
            char* operationString = treeToString(ops[i], false, argNames);
            printf("%s", operationString);
            free(operationString);
        }
    }
    int offset = 0;
    //Side-by-side multiplication
    for(i = 0; i < operationCount - offset - 1; i++) {
        ops[i] = ops[i + offset];
        if((ops[i].op < 3 || ops[i].op > 8 || (ops[i].op != 0 && ops[i].argCount != 0)) && (ops[i + offset + 1].op < 3 || ops[i + offset + 1].op > 8 || (ops[i + offset + 1].op != 0 && ops[i + offset + 1].argCount != 0))) {
            ops[i] = newOperation(allocateArgs(ops[i], ops[i + offset + 1], 0, 0), 2, op_mult);
            offset++;
            totalOpsCount--;
        }
    }
    ops[i] = ops[i + offset];
    //Condense operations: ^%*/+-
    for(i = 3; i < 9; i++) {
        offset = 0;
        operationCount = totalOpsCount;
        int j;
        for(j = 0; j < operationCount - offset; j++) {
            ops[j] = ops[j + offset];
            if(ops[j].op == i && ops[j].argCount == 0) {
                if(j == 0 || j == operationCount - 1 - offset) {
                    error("missing argument in operation", NULL);
                    return NULLOPERATION;
                }
                ops[j] = newOperation(allocateArgs(ops[j - 1], ops[j + 1 + offset], 0, 0), 2, i);
                ops[j - 1] = ops[j];
                j--;
                totalOpsCount -= 2;
                offset += 2;
            }
        }
    }
    if(verbose) {
        char* operationString = treeToString(ops[0], false, argNames);
        printf("\nFinal Tree: %s\n", operationString);
        printf("Tree Count: %d\n", totalOpsCount);
        free(operationString);
    }
    return ops[0];
}
Tree derivative(Tree tree) {
    //returns the derivative of op, output must be freeTree()ed
    //Source: https://en.wikipedia.org/wiki/Differentiation_rules
    //x, variables, and i
    if(tree.op <= op_i || tree.argCount == 0 || tree.op == op_hist) {
        //return a derivative of 1 if it's x, or 0 if it's a constant
        return newOpVal(tree.op == -1 ? 1 : 0, 0, 0);
    }
    //Basic Operations
    if(tree.op < 9) {
        //DOne is the derivative of one
        //DTwo is the derivative of two
        Tree DOne = derivative(tree.branch[0]);
        if(tree.op == op_neg) return newOperation(allocateArg(DOne, 0), 1, op_neg);
        Tree DTwo = derivative(tree.branch[1]);
        if(globalError)
            return NULLOPERATION;
        if(tree.op == op_add || tree.op == op_sub)
            return newOperation(allocateArgs(DOne, DTwo, 0, 0), 2, tree.op);
        if(tree.op == op_mult) {
            //f(x)*g'(x) + g(x)*f'(x)
            Tree DoneTwo, DtwoOne;
            if(!treeIsZero(DOne))
                DoneTwo = newOperation(allocateArgs(tree.branch[1], DOne, 1, 0), 2, op_mult);
            else
                DoneTwo = newOpVal(0, 0, 0);
            if(!treeIsZero(DTwo))
                DtwoOne = newOperation(allocateArgs(tree.branch[0], DTwo, 1, 0), 2, op_mult);
            else
                return DoneTwo;
            if(treeIsZero(DoneTwo))
                return DtwoOne;
            else
                return newOperation(allocateArgs(DoneTwo, DtwoOne, 0, 0), 2, op_add);
        }
        if(tree.op == op_div) {
            //(f'x*gx - g'x*fx) / (gx)^2
            Tree DoneTwo, DtwoOne, out;
            if(!treeIsZero(DOne))
                DoneTwo = newOperation(allocateArgs(DOne, tree.branch[1], 0, 1), 2, op_mult);
            else
                DoneTwo = newOpVal(0, 0, 0);
            if(!treeIsZero(DTwo))
                DtwoOne = newOperation(allocateArgs(DTwo, tree.branch[0], 0, 1), 2, op_mult);
            else
                DtwoOne = newOpVal(0, 0, 0);
            if(treeIsZero(DoneTwo))
                out = DtwoOne;
            else if(treeIsZero(DtwoOne))
                out = DoneTwo;
            else
                out = newOperation(allocateArgs(DoneTwo, DtwoOne, 0, 0), 2, op_sub);
            Tree gxSq = newOperation(allocateArgs(tree.branch[1], newOpVal(2, 0, 0), 1, 0), 2, op_pow);
            return newOperation(allocateArgs(out, gxSq, 0, 0), 2, op_div);
        }
        if(tree.op == op_mod) {
            error("modulo not supported in dx.", NULL);
            return NULLOPERATION;
        }
        if(tree.op == op_pow) {
            //fx^gx * g'x * ln(fx)  +  fx^(g(x)-1) * gx * f'x
            Tree p1, p2;
            if(!treeIsZero(DTwo)) {
                Tree fxgx = newOperation(allocateArgs(tree.branch[0], tree.branch[1], 1, 1), 2, op_pow);
                Tree lnfx = newOperation(allocateArg(tree.branch[0], 1), 1, op_ln);
                Tree fxgxlnfx = newOperation(allocateArgs(fxgx, lnfx, 0, 0), 2, op_mult);
                if(treeIsOne(DTwo))
                    p1 = fxgxlnfx;
                else
                    p1 = newOperation(allocateArgs(fxgxlnfx, DTwo, 0, 0), 2, op_mult);
            }
            else
                p1 = newOpVal(0, 0, 0);
            if(!treeIsZero(DOne)) {
                Tree gm1 = newOperation(allocateArgs(tree.branch[1], newOpVal(1, 0, 0), 1, 0), 2, op_sub);
                Tree fxgm1 = newOperation(allocateArgs(tree.branch[0], gm1, 1, 0), 2, op_pow);
                Tree fxgm1gx = newOperation(allocateArgs(fxgm1, tree.branch[1], 0, 1), 2, op_mult);
                if(treeIsOne(DOne))
                    p2 = fxgm1gx;
                else
                    p2 = newOperation(allocateArgs(fxgm1gx, DOne, 0, 0), 2, op_mult);
            }
            else
                return p1;
            if(treeIsZero(p1))
                return p2;
            return newOperation(allocateArgs(p1, p2, 0, 0), 2, op_add);
        }
    }
    error("not all functions are supported in dx currently", NULL);
    return NULLOPERATION;
}
Number calculate(char* eq, double base) {
    //Clean input
    char* cleanInput = inputClean(eq);
    if(globalError) return NULLNUM;
    //Generate tree
    Tree tree = generateTree(base==0?cleanInput:eq, NULL, base);
    free(cleanInput);
    if(globalError) return NULLNUM;
    //Compute tree
    Number ans = computeTree(tree, NULL, 0);
    freeTree(tree);
    if(globalError) return NULLNUM;
    return ans;
}
#pragma endregion
#pragma region Functions
Function newFunction(char* name, Tree* tree, char argCount, char* argNames) {
    Function out;
    out.name = name;
    if(name == NULL) out.nameLen = 0;
    else out.nameLen = strlen(name);
    out.argNames = argNames;
    out.argCount = argCount;
    out.tree = tree;
    return out;
}
void generateFunction(char* eq) {
    //Parse first half a(x)
    int i, equalPos, openBracket, argCount = 1;
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
    if(openBracket > 15) {
        error("function name too long", NULL);
        return;
    }
    //Get function name
    char* name = calloc(openBracket + 1, 1);
    eq[openBracket] = '\0';
    strcpy(name, eq);
    //Verify name is not already used
    for(i = 0; i < numFunctions; i++) {
        if(functions[i].nameLen == 0)
            continue;
        else if(strcmp(name, functions[i].name) != 0)
            continue;
        error("function name '%s' is already taken", name);
        free(name);
        return;
    }
    //Get argument names
    char* argNames = NULL;
    if(argCount > 0) {
        argNames = calloc(((equalPos - openBracket - 1) / 2) + 1, 1);
        for(i = openBracket + 1; i < equalPos; i += 2)
            argNames[(i - openBracket - 1) / 2] = eq[i];
    }
    //Compute tree
    Tree* tree = malloc(sizeof(Tree));
    char* cleaninput = inputClean(eq + equalPos + 1);
    Tree generateTree(char* eq, char* argNames, double base);
    *tree = generateTree(cleaninput, argNames, 0);
    free(cleaninput);
    if(globalError)
        return;
    //Append to functions
    if(functionArrayLength == numFunctions) {
        functions = realloc(functions, functionArrayLength + 50);
        functionArrayLength += 50;
    }
    functions[numFunctions] = newFunction(name, tree, argCount, argNames);
    numFunctions++;
}
int findFunction(char* name) {
    //Get function id from name
    int len = strlen(name), i;
    for(i = 1; i < numFunctions; i++) {
        if(functions[i].nameLen != len)
            continue;
        if(strcmp(name, functions[i].name) == 0)
            return i;
    }
    return 0;
}
#pragma endregion
#pragma region Main Program
char* inputClean(char* input) {
    //Allocate out
    char* out = calloc(strlen(input) + 1, 1);
    int outPos = 0, i = 0;
    char prev = 0;
    bool allowHex = false, insideSquare = false;
    for(i = 0; i < strlen(input); i++) {
        char in = input[i];
        if(in == '\n') continue;
        //Allow hexadecimal if meets "0x"
        if(prev == '0' && in == 'x') {
            out[outPos++] = 'x';
            allowHex = true;
            continue;
        }
        //Lower case A-F if not allow hex
        if(in >= 'A' && in <= 'F' && (!allowHex) && (!insideSquare)) {
            out[outPos++] = in + 32;
            continue;
        }
        //Add number to output
        if(((in >= '0' && in <= '9') || (in >= 'A' && in <= 'F')) && allowHex) {
            out[outPos++] = in;
            continue;
        }
        //Else allowHex = false
        allowHex = false;
        if(in == ' ') continue;
        //Lower case G-Z if not inside square
        if(in >= 'G' && in <= 'Z' && (!insideSquare)) {
            out[outPos++] = in + 32;
            continue;
        }
        //Check for invalid character
        if((in > '9' && in < 'A') || (in < '$' && in > ' ') || in == '&' || in == '\'' || in == '\\' || in == '`' || in > 'z' || (in == '$' && insideSquare == false)) {
            printf("Error: invalid character '%c'\n", in);//TODO
            free(out);
            globalError = true;
            break;
        }
        //Else add it to the output
        out[outPos++] = in;
        //Check for square brackets
        if(in == '[')
            insideSquare = true;
        if(in == ']')
            insideSquare = false;
        //Set previous
        prev = in;
    }
    if(verbose)
        printf("Input cleaned to '%s'\n", out);
    return out;
}
void cleanup() {
    int i;
    //Free functions
    for(i = immutableFunctions; i < numFunctions; i++) {
        free(functions[i].name);
        freeTree(*functions[i].tree);
        free(functions[i].tree);
        free(functions[i].argNames);
    }
    //Free global array
    free(functions);
    free(history);
    free(unitList);
}
void startup() {
    //Constants
    NULLNUM = newValue(0, 0, 0);
    NULLOPERATION = newOpVal(0, 0, 0);
    //Allocate history
    history = calloc(10, sizeof(Number));
    historySize = 10;
    //Set immutable function args
    functions = calloc(sizeof(Function), immutableFunctions + 10);
    functions[op_val] = newFunction(" ", NULL, 0, NULL);
    functions[op_i] = newFunction("i", NULL, 0, NULL);
    functions[op_neg] = newFunction("neg", NULL, 1, NULL);
    functions[op_pow] = newFunction("pow", NULL, 2, NULL);
    functions[op_mod] = newFunction("mod", NULL, 2, NULL);
    functions[op_mult] = newFunction("mult", NULL, 2, NULL);
    functions[op_div] = newFunction("div", NULL, 2, NULL);
    functions[op_add] = newFunction("add", NULL, 2, NULL);
    functions[op_sub] = newFunction("sub", NULL, 2, NULL);
    functions[op_sin] = newFunction("sin", NULL, 1, NULL);
    functions[op_cos] = newFunction("cos", NULL, 1, NULL);
    functions[op_tan] = newFunction("tan", NULL, 1, NULL);
    functions[op_csc] = newFunction("csc", NULL, 1, NULL);
    functions[op_sec] = newFunction("sec", NULL, 1, NULL);
    functions[op_cot] = newFunction("cot", NULL, 1, NULL);
    functions[op_sinh] = newFunction("sinh", NULL, 1, NULL);
    functions[op_cosh] = newFunction("cosh", NULL, 1, NULL);
    functions[op_tanh] = newFunction("tanh", NULL, 1, NULL);
    functions[op_asin] = newFunction("asin", NULL, 1, NULL);
    functions[op_acos] = newFunction("acos", NULL, 1, NULL);
    functions[op_atan] = newFunction("atan", NULL, 1, NULL);
    functions[op_acsc] = newFunction("acsc", NULL, 1, NULL);
    functions[op_asec] = newFunction("asec", NULL, 1, NULL);
    functions[op_acot] = newFunction("acot", NULL, 1, NULL);
    functions[op_asinh] = newFunction("asinh", NULL, 1, NULL);
    functions[op_acosh] = newFunction("acosh", NULL, 1, NULL);
    functions[op_atanh] = newFunction("atanh", NULL, 1, NULL);
    functions[op_sqrt] = newFunction("sqrt", NULL, 1, NULL);
    functions[op_cbrt] = newFunction("cbrt", NULL, 1, NULL);
    functions[op_exp] = newFunction("exp", NULL, 1, NULL);
    functions[op_ln] = newFunction("ln", NULL, 1, NULL);
    functions[op_logten] = newFunction("logten", NULL, 1, NULL);
    functions[op_log] = newFunction("log", NULL, 2, NULL);
    functions[op_fact] = newFunction("fact", NULL, 1, NULL);
    functions[op_sgn] = newFunction("sgn", NULL, 1, NULL);
    functions[op_abs] = newFunction("abs", NULL, 1, NULL);
    functions[op_arg] = newFunction("arg", NULL, 1, NULL);
    functions[op_norm] = newFunction("norm", NULL, 1, NULL);
    functions[op_round] = newFunction("round", NULL, 1, NULL);
    functions[op_floor] = newFunction("floor", NULL, 1, NULL);
    functions[op_ceil] = newFunction("ceil", NULL, 1, NULL);
    functions[op_getr] = newFunction("getr", NULL, 1, NULL);
    functions[op_geti] = newFunction("geti", NULL, 1, NULL);
    functions[op_getu] = newFunction("getu", NULL, 1, NULL);
    functions[op_grthan] = newFunction("grthan", NULL, 2, NULL);
    functions[op_equal] = newFunction("equal", NULL, 2, NULL);
    functions[op_min] = newFunction("min", NULL, 2, NULL);
    functions[op_max] = newFunction("max", NULL, 2, NULL);
    functions[op_lerp] = newFunction("lerp", NULL, 3, NULL);
    functions[op_dist] = newFunction("dist", NULL, 2, NULL);
    functions[op_not] = newFunction("not", NULL, 1, NULL);
    functions[op_and] = newFunction("and", NULL, 2, NULL);
    functions[op_or] = newFunction("or", NULL, 2, NULL);
    functions[op_xor] = newFunction("xor", NULL, 2, NULL);
    functions[op_ls] = newFunction("ls", NULL, 2, NULL);
    functions[op_rs] = newFunction("rs", NULL, 2, NULL);
    functions[op_pi] = newFunction("pi", NULL, 0, NULL);
    functions[op_phi] = newFunction("phi", NULL, 0, NULL);
    functions[op_e] = newFunction("e", NULL, 0, NULL);
    functions[op_ans] = newFunction("ans", NULL, 0, NULL);
    functions[op_hist] = newFunction("hist", NULL, 1, NULL);
    functions[op_histnum] = newFunction("histnum", NULL, 0, NULL);
    functions[op_rand] = newFunction("rand", NULL, 0, NULL);
    functions[op_sum] = newFunction("sum", NULL, 4, NULL);
    functions[op_product] = newFunction("product", NULL, 4, NULL);
    //Define unit standards
    unitList = calloc(26, sizeof(unitStandard));
    unitList[0] = newUnit("m", -1, 0x1);                  //Meter
    unitList[1] = newUnit("kg", 1, 0x100);                //Kilogram
    unitList[2] = newUnit("g", -0.001, 0x100);            //Gram
    unitList[3] = newUnit("s", -1, 0x10000);              //Second
    unitList[4] = newUnit("A", -1, 0x1000000);            //Ampere
    unitList[5] = newUnit("K", -1, 0x100000000);          //Kelvin
    unitList[6] = newUnit("mol", -1, 0x10000000000);      //Mole
    unitList[7] = newUnit("$", 1, 0x1000000000000);       //Dollar
    unitList[8] = newUnit("b", -1, 0x100000000000000);    //Bit
    unitList[9] = newUnit("B", -8, 0x100000000000000);    //Byte
    unitList[10] = newUnit("bps", -1, 0x100000000FF0000); //Bits per second
    unitList[11] = newUnit("Bps", -8, 0x100000000FF0000); //Bytes per second
    unitList[12] = newUnit("J", -1, 0xFE0102);            //Joule
    unitList[13] = newUnit("W", -1, 0xFD0102);            //Watt
    unitList[14] = newUnit("V", -1, 0x01FD0102);          //Volt
    unitList[15] = newUnit("ohm", -1, 0xFEFD0102);        //Ohm
    unitList[16] = newUnit("H", -1, 0xFEFE0102);          //Henry
    unitList[17] = newUnit("Wb", -1, 0xFFFE0102);         //Weber
    unitList[18] = newUnit("Hz", -1, 0xFF0000);           //Hertz
    unitList[19] = newUnit("S", -1, 0x203FFFE);           //Siemens
    unitList[20] = newUnit("F", -1, 0x204FFFE);           //Farad
    unitList[21] = newUnit("T", -1, 0xFFFE0100);          //Tesla
    unitList[22] = newUnit("Pa", -1, 0xFE01FF);           //Pascal
    unitList[23] = newUnit("N", -1, 0xFE0101);            //Newton
    unitList[24] = newUnit("Sv", -1, 0xFE0002);           //Sievert
    unitList[25] = newUnit("kat", -1, 0x10000FF0000);     //Katal
}
void runLine(char* input) {
    int i;
    //If command
    if(input[0] == '-') {
        if(input[1] == 'd' && input[2] == 'e' && input[3] == 'f' && input[4] == ' ') {
            //Define function
            generateFunction(input + 5);
        }
        else if(input[1] == 'd' && input[2] == 'e' && input[3] == 'l' && input[4] == ' ') {
            int strLen = strlen(input);
            if(input[strLen - 1] == '\n') input[strLen - 1] = 0;
            //Delete function or variable
            for(i = 0; i < numFunctions; i++) if(functions[i].name != NULL)
                if(strcmp(input + 5, functions[i].name) == 0) {
                    if(i < immutableFunctions) {
                        error("Error: '%s' is immutable\n", input + 5);
                        globalError=false;
                        return;
                    }
                    printf("Function '%s' has been deleted.\n", functions[i].name);
                    free(functions[i].name);
                    functions[i].name = NULL;
                    functions[i].nameLen = 0;
                    freeTree(*functions[i].tree);
                    functions[i].tree = NULL;
                    functions[i].argCount = 0;
                    return;
                }
            error("Function '%s' not found\n", input + 5);
            globalError=false;
        }
        else if(input[1] == 'g' && input[2] == ' ') {
            //Graph
            char* cleanInput = inputClean(input + 3);
            if(globalError) {
                globalError = false;
                return;
            }
            graphEquation(cleanInput, -10, 10, 10, -10, 20, 50);
            free(cleanInput);
        }
        else if(input[1] == 'f' && input[2] == ' ') {
            int strLen = strlen(input);
            if(input[strLen - 1] == '\n') input[strLen - 1] = 0;
            //Read lines from a file
            FILE* file = fopen(input + 3, "r");
            unsigned long lineSize = 100;
            char* line = malloc(100);
            while(getline(&line, &lineSize, file) != -1) {
                printf("%s", line);
                runLine(line);
            }
            free(line);
            fclose(file);
        }
        else if(input[1] == 'q' && input[2] == 'u' && input[3] == 'i' && input[4] == 't') {
            //Exit the program
            cleanup();
            exit(0);
        }
        else if(input[1] == 'l' && input[2] == 's') {
            //ls lists all user-defined functions
            int num = 0;
            for(i = immutableFunctions; i < numFunctions; i++) {
                if(functions[i].nameLen == 0) continue;
                num++;
                char* equation = treeToString(*(functions[i].tree), false, functions[i].argNames);
                //Print name
                printf("%s", functions[i].name);
                //Print arguments (if it has them)
                if(functions[i].argNames != NULL) {
                    printf("(");
                    int j;
                    for(j = 0;j < functions[i].argCount;j++) {
                        if(j != 0) printf(",");
                        printf("%c", functions[i].argNames[j]);
                    }
                    printf(")");
                }
                //Print equation
                printf(" = %s\n", equation);
                free(equation);
            }
            printf("There %s %d user-defined function%s.\n", num == 1 ? "is" : "are", num, num == 1 ? "" : "s");
        }
        else if(input[1] == 'd' && input[2] == 'x' && input[3] == ' ') {
            char* cleanInput = inputClean(input + 4);
            if(globalError) {
                globalError = false;
                return;
            }
            Tree ops = generateTree(cleanInput, "x", 0);
            free(cleanInput);
            Tree cleanedOps = treeCopy(ops, NULL, true, false, true);
            Tree dx = derivative(cleanedOps);
            Tree dxClean = treeCopy(dx, NULL, false, false, true);
            char* out = treeToString(dxClean, false, "x");
            printf("=%s\n", out);
            free(out);
            freeTree(cleanedOps);
            freeTree(ops);
            freeTree(dxClean);
            freeTree(dx);
        }
        else if(input[1] == 'b' && input[2] == 'a' && input[3] == 's' && input[4] == 'e') {
            //format: -base(16) 46 will return 2E
            int i, expStart = 0;
            for(i = 5;i < strlen(input);i++) if(input[i] == ' ') {
                expStart = i + 1;
                input[i] = '\0';
                break;
            }
            Number base = calculate(input + 5, 0);
            if(base.r > 36 || base.r < 1) {
                error("base out of bounds", NULL);
                globalError = false;
                return;
            }
            Number out = calculate(input + expStart, 0);
            appendToHistory(out, base.r, true);
        }
        else if(input[1] == 'd' && input[2] == 'e' && input[3] == 'g' && input[4] == 's' && input[5] == 'e' && input[6] == 't' && input[7] == ' ') {
            if(input[8] == 'r' && input[9] == 'a' && input[10] == 'd') degrat = 1;
            else if(input[8] == 'd' && input[9] == 'e' && input[10] == 'g') degrat = M_PI / 180;
            else if(input[8] == 'g' && input[9] == 'r' && input[10] == 'a' && input[11] == 'd') degrat = M_PI / 200;
            else degrat = calculate(input + 7, 0).r;
            printf("Degree ratio set to %g\n", degrat);
        }
        else if(input[1]=='u'&&input[2]=='n'&&input[3]=='i'&&input[4]=='t') {
            int i,unitStart=0;
            for(i=5;i<strlen(input);i++) if(input[i]==' ') {
                unitStart=i+1;
                input[i]='\0';
                break;
            }
            Number unit=calculate(input+5,10);
            Number value=calculate(input+unitStart,0);
            if(unit.u!=value.u) {
                char* unitOne=toStringUnit(unit.u);
                char* unitTwo=toStringUnit(value.u);
                printf("Error: units %s and %s are not compatible\n",unitOne,unitTwo);
                free(unitOne);
                free(unitTwo);
                return;
            }
            Number out=compDivide(value,unit);
            char* numString=toStringNumber(out,10);
            printf("= %s %s\n",numString,input+5);
            free(numString);
        }
        else {
            printf("Error: command '%s' not recognized.\n", input + 1);
        }
    }
    //Else compute it as a value
    else {
        Number out = calculate(input, 0);
        if(!globalError) appendToHistory(out, 10, true);
    }
    globalError = false;
}
#pragma endregion