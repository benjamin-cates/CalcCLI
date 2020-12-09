/*
    This project is maintained by Benjamin Cates (github.com/Unfit-Donkey)
    This project is available on GitHub at https://github.com/Unfit-Donkey/CalcCLI
    See README.md for more information
    See Calc.h for information on each function
    Do ./buildCLI on Mac and Linux or buildCLI.bat on Windows
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
Value* history;
Value NULLVAL;
const unitStandard unitList[] = {
    {"m", -1.0, 0x1},                  //Meter
    {"kg", 1.0, 0x100},                //Kilogram
    {"g", -0.001, 0x100},            //Gram
    {"s", -1.0, 0x10000},              //Second
    {"A", -1.0, 0x1000000},            //Ampere
    {"K", -1.0, 0x100000000},          //Kelvin
    {"mol", -1.0, 0x10000000000},      //Mole
    {"$", 1.0, 0x1000000000000},       //Dollar
    {"b", -1.0, 0x100000000000000},    //Bit
    {"B", -8.0, 0x100000000000000},    //Byte
    {"bps", -1.0, 0x100000000FF0000}, //Bits per second
    {"Bps", -8.0, 0x100000000FF0000}, //Bytes per second
    {"J", -1.0, 0xFE0102},            //Joule
    {"W", -1.0, 0xFD0102},            //Watt
    {"V", -1.0, 0x01FD0102},          //Volt
    {"ohm", -1.0, 0xFEFD0102},        //Ohm
    {"H", -1.0, 0xFEFE0102},          //Henry
    {"Wb", -1.0, 0xFFFE0102},         //Weber
    {"Hz", -1.0, 0xFF0000},           //Hertz
    {"S", -1.0, 0x203FFFE},           //Siemens
    {"F", -1.0, 0x204FFFE},           //Farad
    {"T", -1.0, 0xFFFE0100},          //Tesla
    {"Pa", -1.0, 0xFE01FF},           //Pascal
    {"N", -1.0, 0xFE0101},            //Newton
    {"Sv", -1.0, 0xFE0002},           //Sievert
    {"kat", -1.0, 0x10000FF0000},     //Katal
    {"min",60.0,0x010000},
    {"hr",3600.0,0x010000},
    {"day",86400.0,0x010000},
    {"kph",0.277777777777777,0xFF0001},
    {"mph",0.4470388888888888,0xFF0001},
    {"mach",0.5144444444444,0xFF0001},
    {"c",299792458.0,0xFF0001},
    {"ft",0.3048,0x01},
    {"mi",1609.344,0x01},
    {"yd",0.9144,0x01},
    {"in",0.0254,0x01},
    {"nmi",1852.0,0x01},
    {"pc",-30857000000000000.0,0x01},
    {"acre",4046.8564224,0x02},
    {"are",-1000.0,0x02},
    {"ct",0.0002,0x0100},
    {"mi",1609.344,0x0100},
    {"st",6.35029318,0x0100},
    {"ln",0.45359237,0x0100},
    {"oz",0.028349523125,0x0100},
    {"tn",1000.0,0x0100},
    {"gallon",0.00454609,0x03},
    {"cup",0.0002365882365,0x03},
    {"floz",0.0000295735295625,0x03},
    {"tbsp",0.00001478676478125,0x03},
    {"tsp",0.000000492892159375,0x03},
    {"Ah",-3600.0,0x01010000},
    {"Wh",-3600.0,0xFE0102},
    {"eV",-0.0000000000000000001602176620898,0xFE0102},
    {"atm",101352.0,0xFE01FF},
    {"bar",-100000.0,0xFE01FF},
    {"psi",6894.75729316836133,0xFE01FF},
    {"btu",1054.3503,0xFE0102},
    {"F",0.55555555555555,0x0100000000},

};
Tree NULLOPERATION;
int functionArrayLength = 10;
int numFunctions = 0;
Function* customfunctions;
const char metricNums[] = "yzafpnumchkMGTPEZY";
const double metricNumValues[] = { 0.000000000000000000000001, 0.000000000000000000001, 0.000000000000000001, 0.000000000000001, 0.000000000001, 0.000000001, 0.000001, 0.001, 0.01, 100, 1000, 1000000.0, 1000000000.0, 1000000000000.0, 1000000000000000.0, 1000000000000000000.0, 1000000000000000000000.0, 1000000000000000000000000.0 };
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
                return newNum(fabs(unitList[i].multiplier) * metricNumValues[useMetric], 0, unitList[i].baseUnits);
            }
        if(strcmp(name, unitList[i].name) == 0) {
            return newNum(fabs(unitList[i].multiplier), 0, unitList[i].baseUnits);
        }
    }
    return newNum(1, 0, 0);
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
Number newNum(double r, double i, unit_t u) {
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
    if(isnan(num)) {
        int pos = 0;
        char* out = malloc(5);
        if(num < 0) out[pos++] = '-';
        out[pos] = 'N';
        out[pos + 1] = 'a';
        out[pos + 2] = 'N';
        out[pos + 3] = '\0';
        return out;
    }
    if(isinf(num)) {
        int pos = 0;
        char* out = malloc(5);
        if(num < 0) out[pos++] = '-';
        out[pos] = 'I';
        out[pos + 1] = 'n';
        out[pos + 2] = 'f';
        out[pos + 3] = '\0';
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
void appendToHistory(Value num, double base, bool print) {
    if(historySize - 1 == historyCount) {
        history = realloc(history, (historySize + 25) * sizeof(Value));
        historySize += 25;
    }
    if(print) {
        char* ansString = valueToString(num, base);
        printf("$%d = %s\n", historyCount, ansString);
        free(ansString);
    }
    history[historyCount] = num;
    historyCount++;
}
Number compAdd(Number one, Number two) {
    one.r += two.r;
    one.i += two.i;
    one.u = unitInteract(one.u, two.u, '+', 0);
    return one;
}
Number compSubtract(Number one, Number two) {
    one.r -= two.r;
    one.i -= two.i;
    one.u = unitInteract(one.u, two.u, '+', 0);
    return one;
}
Number compMultiply(Number one, Number two) {
    return newNum(one.r * two.r - one.i * two.i, one.r * two.i + one.i * two.r, unitInteract(one.u, two.u, '*', 0));
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
    return newNum(p1 * cos(cis), p1 * sin(cis), unitInteract(one.u, two.u, '^', two.r));
}
Number compDivide(Number one, Number two) {
    double denominator = two.r * two.r + (two.i * two.i);
    double numeratorR = one.r * two.r + one.i * two.i;
    double numeratorI = one.r * -two.i + one.i * two.r;
    return newNum(numeratorR / denominator, numeratorI / denominator, unitInteract(one.u, two.u, '/', 0));
}
Number compModulo(Number one, Number two) {
    double r = fmod(one.r, two.r);
    double i = fmod(one.i, two.i);
    if(isnan(r)) r = 0.0;
    if(isnan(i)) i = 0.0;
    return newNum(r, i, unitInteract(one.u, two.u, '+', 0));
}
Number compSine(Number one) {
    return newNum(sin(one.r) * cosh(one.i), cos(one.r) * sinh(one.i), one.u);
}
Number compSqrt(Number one) {
    double abs = sqrt(one.r * one.r + one.i * one.i);
    double sgnI = one.i / fabs(one.i);
    if(one.i == 0) sgnI = 1;
    return newNum(sqrt((abs + one.r) / 2), sgnI * sqrt((abs - one.r) / 2), unitInteract(one.u, 0, '^', 0.5));
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
        return newNum(3.14159265358979323 * denom.r / denomSq, 3.14159265358979323 * -denom.i / denomSq, one.u);
    }
    int g = 8;
    double p[] = { 676.5203681218851, -1259.1392167224028, 771.32342877765313, -176.61502916214059, 12.507343278686905, -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7 };
    Number x = newNum(0.99999999999980993, 0, 0);
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
    return newNum(out.r * exr - out.i * exi, out.i * exr + out.r * exi, one.u);
}
Number compTrig(int type, Number num) {
    //Trigonometric functions
    if(type < 18) {
        num.r *= degrat;
        num.i *= degrat;
        if(type == op_sin)
            return compSine(num);
        if(type == op_cos)
            return newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), num.u);
        if(type == op_tan)
            return compDivide(compSine(num), newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), 0));
        Number out;
        bool reciprocal = false;
        // csc, sec, tan
        if(type > 11) {
            reciprocal = true;
            type -= 3;
        }
        if(type == op_sin)
            out = compSine(num);
        if(type == op_cos)
            out = newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), num.u);
        if(type == op_tan)
            out = compDivide(compSine(num), newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), 0));
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
        int opID = type;
        Number out;
        if(type > 20 && type < 24) {
            unit_t unit = num.u;
            num = compDivide(newNum(1, 0, 0), num);
            num.u = unit;
            opID -= 3;
        }
        if(opID == op_asin || opID == op_acos) {
            Number Sq = compSqrt(newNum(1 - num.r * num.r - num.i * num.i, -2 * num.r * num.i, 0));
            double temp = num.r;
            num.r = Sq.r - num.i;
            num.i = Sq.i + temp;
            if(opID == 19)
                out = newNum(1.570796326794896619 - atan2(num.i, num.r), 0.5 * log(num.r * num.r + num.i * num.i), num.u);
            else out = newNum(atan2(num.i, num.r), -0.5 * log(num.r * num.r + num.i * num.i), num.u);
        }
        if(opID == op_tan) {
            Number out = compDivide(newNum(-num.r, 1 - num.i, 0), newNum(num.r, 1 + num.i, 0));
            out = newNum(0.5 * atan2(out.i, out.r), -0.25 * log(out.r * out.r + out.i * out.i), num.u);
        }
        //Hyperbolic Arccosine and hyperbolic sine
        if(type == op_acosh || type == op_asinh) {
            double sqR = num.r * num.r - num.i * num.i;
            double sqI = num.r * num.i * 2;
            if(type == op_asinh) sqR += 1;
            else sqR -= 1;
            Number sqrt = compSqrt(newNum(sqR, sqI, 0));
            sqrt.r += num.r;
            sqrt.i += num.i;
            out = newNum(0.5 * log(sqrt.r * sqrt.r + sqrt.i * sqrt.i), atan2(sqrt.i, sqrt.r), num.u);
        }
        //Hyperbolic Arctangent
        if(type == op_atanh) {
            Number p1 = newNum(num.r + 1, num.i, 0);
            Number p2 = newNum(1 - num.r, -num.i, 0);
            Number div = compDivide(p1, p2);
            out = newNum(0.25 * log(div.r * div.r + div.i * div.i), 0.5 * atan2(div.i, div.r), num.u);
        }
        out.r /= degrat;
        out.i /= degrat;
        return out;
    }
}
Number compBinOp(int type, Number one, Number two) {
    if(type == op_not) {
        if(one.r != 0)
            one.r = (double)~((long)one.r);
        if(one.i != 0)
            one.i = (double)~((long)one.i);
        return one;
    }
    else if(type == op_and) {
        one.r = (double)(((long)one.r) & ((long)two.r));
        one.i = (double)(((long)one.i) & ((long)two.i));
    }
    else if(type == op_or) {
        one.r = (double)(((long)one.r) | ((long)two.r));
        one.i = (double)(((long)one.i) | ((long)two.i));
    }
    else if(type == op_xor) {
        one.r = (double)(((long)one.r) ^ ((long)two.r));
        one.i = (double)(((long)one.i) ^ ((long)two.i));
    }
    else if(type == op_ls) {
        one.r = (double)(((long)one.r) << ((long)two.r));
        one.i = (double)(((long)one.i) << ((long)two.i));
    }
    else if(type == op_rs) {
        one.r = (double)(((long)one.r) >> ((long)two.r));
        one.i = (double)(((long)one.i) >> ((long)two.i));
    }
    return one;
}
#pragma endregion
#pragma region Vectors
Vector newVecScalar(Number num) {
    Vector out;
    out.width = out.height = out.total = 1;
    out.val = malloc(sizeof(Number));
    out.val[0] = num;
    return out;
}
Vector newVec(short width, short height) {
    Vector out;
    out.width = width;
    out.height = height;
    out.total = width * height;
    out.val = calloc(out.total, sizeof(Number));
    return out;
}
Number determinant(Vector vec) {
    if(vec.width == 1) {
        return vec.val[0];
    }
    if(vec.width == 2) {
        return compSubtract(compMultiply(vec.val[0], vec.val[3]), compMultiply(vec.val[1], vec.val[2]));
    }
    int i;
    Number out = NULLNUM;
    Number(*addOrSub[2])(Number, Number) = { &compAdd,&compSubtract };
    for(i = 0;i < vec.width;i++) {
        Vector subsec = subsection(vec, 0, i);
        out = (*(addOrSub[i % 2]))(out, compMultiply(determinant(subsec), vec.val[i]));
        free(subsec.val);
    }
    return out;
}
Vector subsection(Vector vec, int row, int column) {
    int width = vec.width - 1;
    if(row < 0 || row >= vec.height) width++;
    int height = vec.height - 1;
    if(column < 0 || column >= vec.width) height++;
    int i, j, rowID = 0, columnID;
    Vector out = newVec(width, height);
    for(j = 0;j < vec.height;j++) {
        if(j == row) continue;
        columnID = 0;
        for(i = 0;i < vec.width;i++) {
            if(i == column) continue;
            out.val[columnID + rowID * width] = vec.val[i + j * vec.width];
            columnID++;
        }
        rowID++;
    }
    return out;
}
Vector transpose(Vector one) {
    Vector out = newVec(one.height, one.width);
    int i, j;
    for(i = 0;i < one.width;i++) for(j = 0;j < one.height;j++) {
        out.val[j + i * one.height] = one.val[i + j * one.width];
    }
    return out;
}
Vector matMult(Vector one, Vector two) {
    Vector out = newVec(two.width, one.height);
    int i, j;
    for(i = 0;i < out.width;i++) {
        for(j = 0;j < out.height;j++) {
            Number cell = NULLNUM;
            int x;
            for(x = 0;x < one.width;x++) {
                cell = compAdd(cell, compMultiply(one.val[x + j * one.width], two.val[i + x * two.width]));
            }
            out.val[i + j * out.width] = cell;
        }
    }
    return out;
}
Vector matInv(Vector one) {
    if(one.width == 1) {
        return newVecScalar(compDivide(newNum(1, 0, 0), one.val[0]));
    }
    Vector minors = newVec(one.width, one.height);
    int i, j;
    for(i = 0;i < one.width;i++) for(j = 0;j < one.height;j++) {
        Vector sub = subsection(one, j, i);
        minors.val[i + j * one.width] = determinant(sub);
        free(sub.val);
    }
    Number det = NULLNUM;
    Number(*addOrSub[2])(Number, Number) = { &compAdd,&compSubtract };
    for(i = 0;i < one.width;i++) {
        det = addOrSub[i % 2](det, compMultiply(minors.val[i], one.val[i]));
    }
    Vector cofactors = transpose(minors);
    free(minors.val);
    for(i = 0;i < cofactors.width;i++) for(j = 0;j < cofactors.height;j++) {
        Number cell = cofactors.val[i + j * cofactors.width];
        if((i + j) % 2 == 1) {
            cell.r = -cell.r;
            cell.i = -cell.i;
        }
        cell = compDivide(cell, det);
        cofactors.val[i + j * cofactors.width] = cell;
    }
    return cofactors;
}
#pragma endregion
#pragma region Values
void valueConvert(int type, Value* one, Value* two) {
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
double getR(Value val) {
    if(val.type == value_num) {
        return val.r;
    }
    if(val.type == value_vec) {
        if(val.vec.val == NULL) return 0;
        return val.vec.val[0].r;
    }
    if(val.type == value_func) return 0;
    return 0;
}
Number getNum(Value val) {
    if(val.type == value_num) return val.num;
    if(val.type == value_vec) return val.vec.val[0];
    return NULLNUM;
}
void freeValue(Value val) {
    if(val.type == value_vec) free(val.vec.val);
    if(val.type == value_func) {
        freeArgList(val.argNames);
        freeTree(*val.tree);
        free(val.tree);
    }
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
        outTree.branch = val.tree;
        outTree.argNames = val.argNames;
        outTree.argCount = argListLen(val.argNames);
        outTree.optype = optype_anon;
        outTree.op = 0;
        return treeToString(outTree, false, NULL);
    }
    return NULL;
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
        out.tree = malloc(sizeof(Tree));
        *out.tree = treeCopy(*val.tree, NULL, false, false, false);
        out.argNames = argListCopy(val.argNames);
    }
    return out;
}
bool valIsEqual(Value one, Value two) {
    if(one.type != two.type) return false;
    if(one.type == value_num) {
        if(one.r != two.r) return false;
        if(one.i != two.i) return false;
        if(one.u != two.u) return false;
    }
    if(one.type == value_vec) {
        if(one.vec.width != two.vec.width) return false;
        if(one.vec.height != two.vec.height) return false;
        int i;
        for(i = 0;i < one.vec.total;i++) {
            if(one.vec.val[i].r != two.vec.val[i].r) return false;
            if(one.vec.val[i].i != two.vec.val[i].i) return false;
            if(one.vec.val[i].u != two.vec.val[i].u) return false;
        }
    }
    if(one.type == value_func) {

    }
    return true;
}
Value valMult(Value one, Value two) {
    if(one.type != two.type) valueConvert(op_mult, &one, &two);
    if(one.type == value_num) {
        Value out;
        out.type = 0;
        out.r = one.r * two.r - one.i * two.i;
        out.i = one.r * two.i + two.r * one.i;
        out.u = unitInteract(one.u, two.u, '*', 0);
        return out;
    }
    if(one.type == value_vec) {
        if(two.type == value_num) {
            Value out;
            out.type = value_vec;
            out.vec = newVec(one.vec.width, one.vec.height);
            int i;
            for(i = 0;i < out.vec.total;i++) {
                out.vec.val[i] = compMultiply(one.vec.val[i], two.num);
            }
            return out;
        }
        if(two.type == value_vec) {
            short width = two.vec.width < one.vec.width ? two.vec.width : one.vec.width;
            short height = two.vec.height < one.vec.height ? two.vec.height : one.vec.height;
            Value out;
            out.type = value_vec;
            out.vec = newVec(width, height);
            int j, i;
            for(i = 0;i < width;i++) for(j = 0;j < height;j++) {
                out.vec.val[i + j * width] = compMultiply(one.vec.val[i + j * one.vec.width], two.vec.val[i + j * two.vec.width]);
            }
            return out;
        }
    }
    return NULLVAL;
}
Value valAdd(Value one, Value two) {
    if(one.type != two.type) valueConvert(op_add, &one, &two);
    if(one.type == value_num) {
        Value out;
        out.type = value_num;
        out.r = one.r + two.r;
        out.i = one.i + two.i;
        if(one.u == 0) out.u = two.u;
        else if(two.u == 0) out.u = one.u;
        else error("cannot add two different units", NULL);
        return out;
    }
    if(one.type == value_vec) {
        int width = one.vec.width < two.vec.width ? two.vec.width : one.vec.width;
        int height = one.vec.height < two.vec.height ? two.vec.height : one.vec.height;
        Value out;
        out.type = one.type;
        out.vec = newVec(width, height);
        int i, j;
        for(i = 0;i < width;i++) for(j = 0;j < height;j++) {
            Number* cell = out.vec.val + i + j * width;
            if(i < one.vec.width && j < one.vec.height) {
                Number* cellOne = one.vec.val + i + j * one.vec.width;
                cell->r += cellOne->r;
                cell->i += cellOne->i;
                cell->u = cellOne->u;
            }
            if(i < two.vec.width && j < two.vec.height) {
                Number* cellTwo = two.vec.val + i + j * two.vec.width;
                cell->r += cellTwo->r;
                cell->i += cellTwo->i;
                if(cell->u == 0) cell->u = cellTwo->u;
            }
        }
        return out;
    }
    return NULLVAL;
}
Value valNegate(Value one) {
    if(one.type == value_num) {
        Value out;
        out.type = value_num;
        out.r = -one.r;
        out.i = -one.i;
        out.u = one.u;
        return out;
    }
    if(one.type == value_vec) {
        Value out;
        out.type = one.type;
        out.vec = newVec(one.vec.width, one.vec.height);
        int i;
        for(i = 0;i < out.vec.total;i++) {
            out.vec.val[i].r = -one.vec.val[i].r;
            out.vec.val[i].i = -one.vec.val[i].i;
            out.vec.val[i].u = one.vec.val[i].u;
        }
        return out;
    }
    if(one.type == value_func) {
        error("cannot negate functions");
        return NULLVAL;
    }
    return NULLVAL;
}
Value DivPowMod(Number(*function)(Number, Number), Value one, Value two, int type) {
    if(one.type != two.type) valueConvert(op_div, &one, &two);
    if(one.type == value_num) {
        Value out;
        out.type = two.type;
        if(two.type == value_num) {
            out.num = (*function)(one.num, two.num);
            return out;
        }
        if(two.type == value_vec) {
            out.vec = newVec(two.vec.width, two.vec.height);
            int i;
            for(i = 0;i < out.vec.total;i++) {
                out.vec.val[i] = (*function)(one.num, two.vec.val[i]);
            }
            return out;
        }
    }
    if(one.type == value_vec) {
        Value out;
        out.type = value_vec;
        if(two.type == value_num) {
            out.vec = newVec(one.vec.width, one.vec.height);
            int i;
            for(i = 0;i < out.vec.total;i++) {
                out.vec.val[i] = (*function)(one.vec.val[i], two.num);
            }
            return out;
        }
        // max width if type is 0, min width if type is 1
        int newWidth = (one.vec.width > two.vec.width) ^ type ? one.vec.width : two.vec.width;
        int newHeight = (one.vec.height > two.vec.height) ^ type ? one.vec.height : two.vec.height;
        out.vec = newVec(newWidth, newHeight);
        int i, j;
        for(i = 0;i < newWidth;i++) {
            for(j = 0;j < newHeight;j++) {
                Number oneNum = NULLNUM;
                Number twoNum = NULLNUM;
                if(i < one.vec.width && j < one.vec.height) oneNum = one.vec.val[i + j * one.vec.width];
                if(i < two.vec.width && j < two.vec.height) twoNum = two.vec.val[i + j * two.vec.width];
                out.vec.val[i + j * newWidth] = (*function)(oneNum, twoNum);
            }
        }
        return out;
    }
    return NULLVAL;
}
Value valDivide(Value one, Value two) {
    return DivPowMod(&compDivide, one, two, 1);
}
Value valPower(Value one, Value two) {
    return DivPowMod(&compPower, one, two, 0);
}
Value valModulo(Value one, Value two) {
    return DivPowMod(&compModulo, one, two, 1);
}
Value valLn(Value one) {
    if(one.type == value_num) {
        return newValNum(0.5 * log(one.r * one.r + one.i * one.i), atan2f(one.i, one.r), one.u);
    }
    if(one.type == value_func) {
        error("cannot ln functions");
        return NULLVAL;
    }
    return NULLVAL;
}
Value valAbs(Value one) {
    if(one.type == value_num) {
        return newValNum(sqrt(one.r * one.r + one.i * one.i), 0, one.u);
    }
    if(one.type == value_vec) {
        double out = 0;
        int i;
        for(i = 0;i < one.vec.total;i++) {
            Number val = one.vec.val[i];
            out += val.r * val.r + val.i * val.i;
        }
        return newValNum(sqrt(out), 0, 0);
    }
    if(one.type == value_func) {
        error("cannot abs functions");
        return NULLVAL;
    }
    return NULLVAL;
}
#pragma endregion
#pragma region Trees
//Comparison
bool treeIsZero(Tree in) {
    if(in.optype != optype_builtin)
        return false;
    if(in.op != op_val)
        return false;
    if(in.value.type != 0)
        return false;
    if(in.value.r != 0)
        return false;
    if(in.value.i != 0)
        return false;
    return true;
}
bool treeIsOne(Tree in) {
    if(in.optype != optype_builtin)
        return false;
    if(in.op != op_val)
        return false;
    if(in.value.type != 0)
        return false;
    if(in.value.r != 1)
        return false;
    if(in.value.i != 0)
        return false;
    return true;
}
bool treeEqual(Tree one, Tree two) {
    if(one.op != two.op || one.optype != two.optype)
        return false;
    if(one.op == op_val) {
        if(valIsEqual(one.value, two.value) == false) return false;
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
Tree* allocArg(Tree one, bool copy) {
    Tree* out = malloc(sizeof(Tree));
    if(copy)
        out[0] = treeCopy(one, NULL, 0, 0, 0);
    else
        out[0] = one;
    return out;
}
//Recursive functions
void freeTree(Tree tree) {
    if(tree.optype == optype_anon) {
        freeTree(tree.branch[0]);
        free(tree.branch);
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
char* treeToString(Tree tree, bool bracket, char** argNames) {
    if(tree.optype == optype_anon) {
        char* argListString = argListToString(tree.argNames);
        char** newArgNames = mergeArgList(argNames, tree.argNames);
        char* treeString = treeToString(tree.branch[0], true, newArgNames);
        int argListLen = strlen(argListString);
        char* out = calloc(argListLen + 2 + strlen(treeString) + 1, 1);
        strcpy(out, argListString);
        strcpy(out + argListLen, "=>");
        strcpy(out + argListLen + 2, treeString);
        free(argListString);
        free(treeString);
        free(newArgNames);
        return out;
    }
    //Arguments
    if(tree.optype == optype_argument) {
        if(argNames == NULL) {
            char* num = calloc(10, 1);
            sprintf(num, "{%d}", tree.op);
            return num;
        }
        else {
            int len = strlen(argNames[tree.op]);
            char* out = calloc(len + 1, 1);
            memcpy(out, argNames[tree.op], len);
            return out;
        }
    }
    //Numbers
    if(tree.optype == optype_builtin && tree.op == op_val)
        return valueToString(tree.value, 10);
    //Operations
    if(tree.optype == optype_builtin && tree.op > 1 && tree.op < 9) {
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
    //Vectors
    if(tree.op == op_vector) {
        char* values[tree.argCount];
        int i, j;
        int len = 3;
        for(i = 0;i < tree.argCount;i++) {
            values[i] = treeToString(tree.branch[i], false, argNames);
            len += 1 + strlen(values[i]);
        }
        char* out = calloc(len, sizeof(char));
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
    const char* functionName;
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
        argText[i] = treeToString(tree.branch[i], false, argNames);
        strLength += strlen(argText[i]) + 1;
    }
    //Compile together the strings of the branches
    char* out = calloc(strLength, 1);
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
Value computeTree(Tree tree, Value* args, int argLen) {
    if(tree.optype == optype_builtin) {
        if(tree.op == op_val)
            return copyValue(tree.value);
        //Vector Functions
        if(tree.op > 95 && tree.op < 101) {
            if(tree.op == op_vector) {
                int width = tree.argWidth;
                int height = tree.argCount / tree.argWidth;
                int i;
                Vector vec = newVec(width, height);
                for(i = 0;i < vec.total;i++) {
                    Value cell = computeTree(tree.branch[i], args, argLen);
                    if(cell.type == value_num) vec.val[i] = cell.num;
                    if(cell.type == value_vec) {
                        vec.val[i] = cell.vec.val[0];
                        freeValue(cell);
                    }
                }
                Value out;
                out.type = value_vec;
                out.vec = vec;
                return out;
            }
            if(tree.op == op_width || tree.op == op_height || tree.op == op_length) {
                int width = 0;
                int height = 0;
                Value vec;
                bool freeVec = false;
                if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_vector) {
                    width = tree.branch[0].argWidth;
                    height = tree.branch[0].argCount / width;
                }
                else if(tree.branch[0].optype == optype_argument) {
                    if(tree.branch[0].op >= argLen) {
                        error("argument error");
                        return NULLVAL;
                    }
                    vec = args[tree.branch[0].op];
                }
                else {
                    vec = computeTree(tree.branch[0], args, argLen);
                    freeVec = true;
                }
                if(width == 0 && height == 0) {
                    if(vec.type == value_num) return newValNum(1, 0, 0);
                    if(vec.type == value_vec) {
                        if(freeVec) freeValue(vec);
                        if(tree.op == op_width) return newValNum(vec.vec.width, 0, 0);
                        if(tree.op == op_height) return newValNum(vec.vec.height, 0, 0);
                        if(tree.op == op_length) return newValNum(vec.vec.total, 0, 0);
                    }
                }
                if(tree.op == op_width) return newValNum(width, 0, 0);
                if(tree.op == op_height) return newValNum(height, 0, 0);
                if(tree.op == op_length) return newValNum(width * height, 0, 0);
            }
            if(tree.op == op_ge) {
                int x = 0, y = 0;
                if(tree.argCount == 3) {
                    Value yVal = computeTree(tree.branch[2], args, argLen);
                    y = getR(yVal);
                    freeValue(yVal);
                }
                Value xVal = computeTree(tree.branch[1], args, argLen);
                x = getR(xVal);
                freeValue(xVal);
                if(x < 0 || y < 0) return NULLVAL;
                Value vec;
                bool freeVec = false;
                if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_vector) {
                    int width = tree.branch[0].argWidth;
                    if(tree.argCount == 3)
                        if(x >= width || y >= tree.branch[0].argCount / width) return NULLVAL;
                    return computeTree(tree.branch[0].branch[x + y * width], args, argLen);
                }
                else if(tree.branch[0].optype == optype_argument) {
                    if(tree.branch[0].op >= argLen) {
                        error("argument error");
                        return NULLVAL;
                    }
                    Value vec = args[tree.branch[0].op];
                }
                else if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_val) {
                    vec = tree.branch[0].value;
                }
                else if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_hist) {
                    Value index = computeTree(tree.branch[0].branch[0], args, argLen);
                    int i = getR(index);
                    freeValue(index);
                    if(i < 0) {
                        if(-i > historyCount) {
                            error("history too short", NULL);
                            return NULLVAL;
                        }
                        vec = history[historyCount + i];
                    }
                    if(i >= historyCount) {
                        error("history too short", NULL);
                        return NULLVAL;
                    }
                    vec = history[i];
                }
                else if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_ans) {
                    vec = history[historyCount - 1];
                }
                else {
                    freeVec = true;
                    vec = computeTree(tree.branch[0], args, argLen);
                }
                if(vec.type == value_num) {
                    if(x == 0 && y == 0) return vec;
                    else return newValNum(0, 0, 0);
                }
                if(vec.type == value_vec) {
                    int width = vec.vec.width;
                    Value out;
                    out.type = value_num;
                    out.num = vec.vec.val[x + y * width];
                    if(freeVec) freeValue(vec);
                    if(tree.argCount == 3)
                        if(x >= width || y >= vec.vec.height) return NULLVAL;
                    return out;
                }
                return NULLVAL;
            }
        }
        Value one, two;
        if(tree.argCount > 0) {
            one = computeTree(tree.branch[0], args, argLen);
            if(one.type == value_func && tree.optype == optype_builtin) {
                if(tree.op != op_run && tree.op != op_sum && tree.op != op_product && tree.op != op_fill) {
                    error("functions cannot be passed to %s", stdfunctions[tree.op].name);
                    freeValue(one);
                    return NULLVAL;
                }
            }
        }
        if(tree.argCount > 1) {
            two = computeTree(tree.branch[1], args, argLen);
            if(two.type == value_func && tree.optype == optype_builtin && tree.op != op_map) {
                if(tree.op != op_run && tree.op != op_sum && tree.op != op_product) {
                    error("cannot run %s with a function as an argument", stdfunctions[tree.op].name);
                    freeValue(two);
                    return NULLVAL;
                }
            }
        }
        if(globalError)
            return NULLVAL;
        //Basic operators
        if(tree.op < 9) {
            if(tree.op == op_i)
                return newValNum(0, 1, 0);
            Value out;
            if(tree.op == op_neg) out = valNegate(one);
            if(tree.op == op_pow) out = DivPowMod(&compPower, one, two, 0);
            if(tree.op == op_mod) out = DivPowMod(&compModulo, one, two, 1);
            if(tree.op == op_div) out = DivPowMod(&compDivide, one, two, 1);
            if(tree.op == op_mult) out = valMult(one, two);
            if(tree.op == op_add) out = valAdd(one, two);
            if(tree.op == op_sub) {
                Value negative = valNegate(two);
                out = valAdd(one, negative);
                freeValue(negative);
            }
            freeValue(one);
            if(tree.argCount > 1) freeValue(two);
            return out;
        }
        //Trigonometric functions
        if(tree.op < 30) {
            if(one.type == value_num) {
                Value out;
                out.type = one.type;
                out.num = compTrig(tree.op, one.num);
                return out;
            }
            if(one.type == value_vec) {
                int i;
                for(i = 0;i < one.vec.total;i++) {
                    one.vec.val[i] = compTrig(tree.op, one.vec.val[i]);
                }
                return one;
            }
        }
        //Log, arg, and abs
        if(tree.op < 46) {
            if(tree.op == op_sqrt || tree.op == op_cbrt) {
                double pow = (tree.op == op_sqrt) ? 1.0 / 2.0 : 1.0 / 3.0;
                Value out = valPower(one, newValNum(pow, 0, 0));
                freeValue(one);
                return out;
            }
            if(tree.op == op_exp) {
                if(one.type == value_num) {
                    double expr = exp(one.r);
                    one.r = expr * cos(one.i);
                    one.i = expr * sin(one.i);
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        double expr = exp(one.vec.val[i].r);
                        one.vec.val[i].r = expr * cos(one.vec.val[i].i);
                        one.vec.val[i].i = expr * sin(one.vec.val[i].i);
                    }
                    return one;
                }
            }
            if(tree.op == op_ln) {
                Value out = valLn(one);
                freeValue(one);
                return out;
            }
            if(tree.op == op_logten) {
                double logten = 1 / log(10);
                Value out = valLn(one);
                if(one.type == value_num) {
                    out.r *= logten;
                    out.i *= logten;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].r *= logten;
                        out.vec.val[i].i *= logten;
                    }
                }
                freeValue(one);
                return out;
            }
            if(tree.op == op_log) {
                Value LnOne = valLn(one);
                Value LnTwo = valLn(two);
                Value out = valDivide(LnOne, LnTwo);
                freeValue(LnOne);
                freeValue(LnTwo);
                return out;
            }
            if(tree.op == op_fact) {
                if(one.type == value_num) {
                    one.r += 1;
                    one.num = compGamma(one.num);
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].r += 1;
                        one.vec.val[i] = compGamma(one.vec.val[i]);
                    }
                    return one;
                }
            }
            if(tree.op == op_sgn) {
                Value abs = valAbs(one);
                Value out = valDivide(one, abs);
                freeValue(abs);
                freeValue(one);
                return out;
            }
            if(tree.op == op_abs) {
                Value out = valAbs(one);
                freeValue(one);
                return out;
            }
            if(tree.op == op_arg) {
                if(one.type == value_num) {
                    one.r = atan2(one.i, one.r);
                    one.i = 0;
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].r = atan2(one.vec.val[i].i, one.vec.val[i].r);
                        one.vec.val[i].i = 0;
                    }
                    return one;
                }
            }
            return NULLVAL;
        }
        //Rounding and conditionals
        if(tree.op < 59) {
            if(tree.op >= op_round && tree.op <= op_ceil) {
                double (*roundType)(double);
                if(tree.op == op_round) roundType = &round;
                if(tree.op == op_floor) roundType = &floor;
                if(tree.op == op_ceil) roundType = &ceil;
                if(one.type == value_num) {
                    one.r = (*roundType)(one.r);
                    one.i = (*roundType)(one.i);
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].r = (*roundType)(one.vec.val[i].r);
                        one.vec.val[i].i = (*roundType)(one.vec.val[i].i);
                    }
                    return one;
                }
            }
            if(tree.op == op_getr) {
                if(one.type == value_num) {
                    one.i = 0;
                    one.u = 0;
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].i = 0;
                        one.vec.val[i].u = 0;
                    }
                    return one;
                }
            }
            if(tree.op == op_geti) {
                if(one.type == value_num) {
                    one.r = one.i;
                    one.i = 0;
                    one.u = 0;
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].r = one.vec.val[i].i;
                        one.vec.val[i].i = 0;
                        one.vec.val[i].u = 0;
                    }
                    return one;
                }
            }
            if(tree.op == op_getu) {
                if(one.type == value_num) {
                    one.r = 1;
                    one.i = 0;
                    return one;
                }
                if(one.type == value_vec) {
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        one.vec.val[i].r = 1;
                        one.vec.val[i].i = 0;
                    }
                    return one;
                }
            }
            if(one.type != two.type) valueConvert(op_add, &one, &two);
            if(tree.op == op_equal) {
                if(one.type == value_num) {
                    if(one.r == two.r && one.r == two.r)
                        return newValNum(1, 0, 0);
                    else
                        return newValNum(0, 0, 0);
                }
                if(one.type == value_vec) {
                    if(one.vec.width != two.vec.width || one.vec.height != two.vec.height) {
                        freeValue(one);
                        freeValue(two);
                        return newValNum(0, 0, 0);
                    }
                    int i;
                    for(i = 0;i < one.vec.total;i++) {
                        if(one.r != two.r || one.i != two.i) {
                            freeValue(one);
                            freeValue(two);
                            return newValNum(0, 0, 0);
                        }
                    }
                    return newValNum(1, 0, 0);
                }
            }
            if(tree.op == op_min || tree.op == op_max || tree.op == op_grthan) {
                if(one.type == value_num) {
                    if(tree.op == op_grthan) return newValNum(one.r > two.r ? 1 : 0, 0, 0);
                    if(one.r > two.r)
                        return tree.op == op_max ? one : two;
                    else
                        return tree.op == op_max ? two : one;
                }
                if(one.type == value_vec) {
                    int width = one.vec.width > two.vec.width ? one.vec.width : two.vec.width;
                    int height = one.vec.height > two.vec.height ? one.vec.height : two.vec.height;
                    Value out;
                    out.type = value_vec;
                    out.vec = newVec(width, height);
                    int i, j;
                    for(i = 0;i < width;i++) for(j = 0;j < height;j++) {
                        Number oneNum = NULLNUM;
                        Number twoNum = NULLNUM;
                        if(i < one.vec.width && j < one.vec.height) oneNum = one.vec.val[i + j * one.vec.width];
                        if(i < two.vec.width && j < two.vec.height) twoNum = two.vec.val[i + j * two.vec.width];
                        if(tree.op == op_grthan) {
                            out.vec.val[i + j * width] = newNum(oneNum.r > twoNum.r ? 1 : 0, 0, 0);
                        }
                        else {
                            if((oneNum.r > twoNum.r) ^ (tree.op == op_max)) out.vec.val[i + j * width] = twoNum;
                            else out.vec.val[i + j * width] = oneNum;
                        }
                    }
                    freeValue(one);
                    freeValue(two);
                    return out;
                }
            }
            if(tree.op == op_lerp) {
                //(1 - c) * one + c * two;
                Value c = computeTree(tree.branch[2], args, argLen);
                Value negativeC = valNegate(c);
                Value oneSubC = valAdd(newValNum(1, 0, 0), negativeC);
                Value cTimesTwo = valMult(c, two);
                Value oneSubCTimesTwo = valMult(oneSubC, one);
                Value out = valAdd(cTimesTwo, oneSubCTimesTwo);
                freeValue(c);
                freeValue(negativeC);
                freeValue(oneSubC);
                freeValue(cTimesTwo);
                freeValue(oneSubCTimesTwo);
                return out;
            }
            if(tree.op == op_dist) {
                if(one.type == value_num) return newValNum(sqrt(pow(fabs(one.r - two.r), 2) + pow(fabs(one.i - two.i), 2)), 0, 0);
                if(one.type == value_vec) {
                    int i;
                    int length = one.vec.total > two.vec.total ? one.vec.total : two.vec.total;
                    double out = 0;
                    for(i = 0;i < length;i++) {
                        Number oneNum, twoNum;
                        if(i < one.vec.total) oneNum = one.vec.val[i];
                        else oneNum = NULLNUM;
                        if(i < two.vec.total) twoNum = two.vec.val[i];
                        else twoNum = NULLNUM;
                        out += pow(oneNum.r - twoNum.r, 2) + pow(oneNum.i - twoNum.i, 2);
                    }
                    freeValue(one);
                    freeValue(two);
                    return newValNum(sqrt(out), 0, 0);
                }
            }
        }
        //Binary Operations
        if(tree.op < 72) {
            if(one.type != two.type && tree.op != op_not) valueConvert(op_add, &one, &two);
            if(one.type == value_num) {
                Value out;
                out.type = value_num;
                out.num = compBinOp(tree.op, one.num, two.num);
                freeValue(one);
                freeValue(two);
                return out;
            }
        }
        //Constants
        if(tree.op < 88) {
            if(tree.op == op_pi)
                return newValNum(3.1415926535897932, 0, 0);
            if(tree.op == op_e)
                return newValNum(2.718281828459045, 0, 0);
            if(tree.op == op_phi)
                return newValNum(1.618033988749894, 0, 0);
            if(tree.op == op_ans) {
                if(historyCount == 0) {
                    error("no previous answer", NULL);
                    return NULLVAL;
                }
                return copyValue(history[historyCount - 1]);
            }
            if(tree.op == op_hist) {
                int i = (int)floor(getR(one));
                freeValue(one);
                if(i < 0) {
                    if(-i > historyCount) {
                        error("history too short", NULL);
                        return NULLVAL;
                    }
                    return copyValue(history[historyCount - i]);
                }
                if(i >= historyCount) {
                    error("history too short", NULL);
                    return NULLVAL;
                }
                return copyValue(history[i]);
            }
            if(tree.op == op_histnum)
                return newValNum(historyCount, 0, 0);
            if(tree.op == op_rand)
                return newValNum((double)rand() / RAND_MAX, 0, 0);
        }
        //Run, Sum, and Product
        if(tree.op < 93) {
            if(one.type != value_func) {
                error("first argument of %s is not a function", stdfunctions[tree.op].name);
                freeValue(one);
                freeValue(two);
            }
            if(tree.op == op_run) {
                int argCount = tree.argCount - 1;
                int requiredArgs = argListLen(one.argNames);
                if(argCount < requiredArgs) {
                    error("not enough args in run function");
                    freeValue(one);
                    if(tree.argCount > 1) freeValue(two);
                }
                Value* inputs = calloc(tree.argCount, sizeof(Value));
                inputs[0] = two;
                int i;
                for(i = 1;i < argCount;i++) {
                    inputs[i] = computeTree(tree.branch[i + 1], args, argLen);
                }
                Value out = computeTree(*one.tree, inputs, argCount);
                for(i = 0;i < argCount;i++) freeValue(inputs[i]);
                freeValue(one);
                return out;
            }
            Value tempArgs[2];
            memset(tempArgs, 0, (argLen + 1) * sizeof(Number));
            if(args != NULL) memcpy(tempArgs, args, sizeof(Number) * argLen);
            Value loopArgValues[3];
            loopArgValues[0] = two;
            loopArgValues[1] = computeTree(tree.branch[2], args, argLen);
            loopArgValues[2] = computeTree(tree.branch[3], args, argLen);
            double loopArgs[3];
            loopArgs[0] = getR(loopArgValues[0]);
            loopArgs[1] = getR(loopArgValues[1]);
            loopArgs[2] = getR(loopArgValues[2]);
            freeValue(loopArgValues[0]);
            freeValue(loopArgValues[1]);
            freeValue(loopArgValues[2]);
            Value out;
            double i;
            if((loopArgs[1] - loopArgs[1]) > loopArgs[2] * 100 || loopArgs[1] < loopArgs[0])
                return NULLVAL;
            if(tree.op == op_sum) {
                out = newValNum(0, 0, 0);
                tempArgs[0] = newValNum(0, 0, 0);
                for(i = loopArgs[0];i <= loopArgs[1];i += loopArgs[2]) {
                    tempArgs[0].r = i;
                    Value current = computeTree(one.tree[0], tempArgs, 1);
                    Value new = valAdd(out, current);
                    freeValue(out);
                    freeValue(current);
                    out = new;
                }
            }
            if(tree.op == op_product) {
                out = newValNum(1, 0, 0);
                tempArgs[0] = newValNum(0, 0, 0);
                for(i = loopArgs[0]; i <= loopArgs[1];i += loopArgs[2]) {
                    tempArgs[0].r = i;
                    Value current = computeTree(one.tree[0], tempArgs, 1);
                    Value new = valMult(out, current);
                    freeValue(out);
                    freeValue(current);
                    out = new;
                }
            }
            return out;
        }
        //Matrix functions
        if(tree.op < 107) {
            if(tree.op == op_fill) {
                int width = getR(two);
                freeValue(two);
                int height = 1;
                if(tree.argCount > 2) {
                    Value three = computeTree(tree.branch[2], args, argLen);
                    height = getR(three);
                    freeValue(three);
                }
                int i, j;
                Value out;
                out.type = value_vec;
                out.vec = newVec(width, height);
                if(one.type == value_func) {
                    Value funcArgs[3];
                    funcArgs[0] = NULLVAL;
                    funcArgs[1] = NULLVAL;
                    funcArgs[2] = NULLVAL;
                    for(j = 0;j < height;j++) for(i = 0;i < width;i++) {
                        funcArgs[0].r = i;
                        funcArgs[1].r = j;
                        funcArgs[2].r = i + j * width;
                        Value cell = computeTree(one.tree[0], funcArgs, 3);
                        out.vec.val[i + j * width] = getNum(cell);
                        freeValue(cell);
                        if(globalError) return NULLVAL;
                    }
                }
                if(one.type == value_vec || one.type == value_num) {
                    for(j = 0;j < height;j++) for(i = 0;i < width;i++) {
                        out.vec.val[i + j * width] = getNum(one);
                    }
                }
                freeValue(one);
                return out;
            }
            if(tree.op == op_map) {
                if(one.type == value_num) {
                    one = newValMatScalar(value_vec, one.num);
                }
                if(two.type != value_func) {
                    error("second argument of map must be a function");
                    freeValue(one);
                    freeValue(two);
                    return NULLVAL;
                }
                Value out;
                out.type = value_vec;
                out.vec = newVec(one.vec.width, one.vec.height);
                Value funcArgs[5];
                funcArgs[0] = NULLVAL;
                funcArgs[1] = NULLVAL;
                funcArgs[2] = NULLVAL;
                funcArgs[3] = NULLVAL;
                funcArgs[4] = one;
                int i, j;
                for(j = 0;j < one.vec.height;j++) for(i = 0;i < one.vec.width;i++) {
                    funcArgs[0].num = one.vec.val[i + j * one.vec.width];
                    funcArgs[1].r = i;
                    funcArgs[2].r = j;
                    funcArgs[3].r = i + j * one.vec.width;
                    Value cell = computeTree(*two.tree, funcArgs, 5);
                    out.vec.val[i + j * one.vec.width] = getNum(cell);
                    freeValue(cell);
                }
                freeValue(one);
                freeValue(two);
                return out;
            }
            if(tree.op == op_det) {
                if(one.type == value_num) {
                    one = newValMatScalar(value_vec, one.num);
                }
                if(one.type == value_vec) {
                    if(one.vec.width != one.vec.height) {
                        freeValue(one);
                        error("Cannot calculate determinant of non-square matrix", NULL);
                        return NULLVAL;
                    }
                    Value out;
                    out.type = value_num;
                    out.num = determinant(one.vec);
                    freeValue(one);
                    return out;
                }
            }
            if(tree.op == op_transpose) {
                if(one.type == value_num) one = newValMatScalar(value_vec, one.num);
                Value out;
                out.type = value_vec;
                out.vec = transpose(one.vec);
                freeValue(one);
                return out;
            }
            if(tree.op == op_mat_mult) {
                if(one.type == value_num) one = newValMatScalar(value_vec, one.num);
                if(two.type == value_num) two = newValMatScalar(value_vec, two.num);
                if(one.vec.width != two.vec.height) {
                    error("Matrix size error in mat_mult", NULL);
                    freeValue(one);
                    freeValue(two);
                    return NULLVAL;
                }
                Value out;
                out.type = value_vec;
                out.vec = matMult(one.vec, two.vec);
                freeValue(one);
                freeValue(two);
                return out;
            }
            if(tree.op == op_mat_inv) {
                if(one.type == value_num) one = newValMatScalar(value_vec, one.num);
                if(one.vec.width != one.vec.height) {
                    freeValue(one);
                    error("cannot take inverse of non-square matrix");
                    return NULLVAL;
                }
                Value out;
                out.type = value_vec;
                out.vec = matInv(one.vec);
                freeValue(one);
                return out;
            }
        }
    }
    if(tree.optype == optype_argument) {
        if(tree.op >= argLen) error("argument error", NULL);
        return copyValue(args[tree.op]);
    }
    if(tree.optype == optype_custom) {
        if(customfunctions[tree.op].tree == NULL) {
            error("this uses a nonexistent function", NULL);
            return NULLVAL;
        }
        Value funcArgs[tree.argCount];
        int i;
        //Crunch args
        for(i = 0; i < tree.argCount; i++)
            funcArgs[i] = computeTree(tree.branch[i], args, argLen);
        //Compute value
        Value out = computeTree(*customfunctions[tree.op].tree, funcArgs, tree.argCount);
        //Free args
        for(i = 0;i < tree.argCount;i++) freeValue(funcArgs[i]);
        if(globalError)
            return NULLVAL;
        return out;
    }
    if(tree.optype == optype_anon) {
        Value out;
        out.argNames = argListCopy(tree.argNames);
        out.type = value_func;
        out.tree = malloc(sizeof(Tree));
        Tree* replaceArgs = calloc(argLen, sizeof(Tree));
        int i;
        for(i = 0;i < argLen;i++) {
            replaceArgs[i] = newOpValue(args[i]);
        }
        *out.tree = treeCopy(tree.branch[0], replaceArgs, false, tree.argWidth, false);
        free(replaceArgs);
        return out;
    }
    return NULLVAL;
}
Tree treeCopy(Tree tree, Tree* args, bool unfold, int replaceArgs, bool optimize) {
    Tree out = tree;
    if(tree.optype == optype_anon) {
        out.argNames = argListCopy(tree.argNames);
        out.branch = malloc(sizeof(Tree));
        *out.branch = treeCopy(tree.branch[0], args, unfold, replaceArgs, optimize);
        out.argWidth -= replaceArgs;
        return out;
    }
    if(tree.optype == optype_builtin && tree.op == op_val) {
        out.value = copyValue(tree.value);
        return out;
    }
    //Example: if f(x)=x^2, copyTree(f(2x),NULL,true,false,false) will return (2x)^2
    //Replace arguments
    if(tree.optype == optype_argument) {
        if(replaceArgs > tree.op)
            return treeCopy(args[tree.op], NULL, unfold, 0, optimize);
        else out.op -= replaceArgs;
    }
    //Copy tree branches
    if(tree.argCount != 0) out.branch = malloc(tree.argCount * sizeof(Tree));
    int i;
    bool crunch = true;
    for(i = 0; i < tree.argCount; i++) {
        Tree* branch = out.branch + i;
        *branch = treeCopy(tree.branch[i], args, unfold, replaceArgs, optimize);
        if(branch->optype == optype_argument || branch->optype == optype_anon || (branch->argCount != 0 && !(branch->op == op_val && branch->optype == 0)))
            crunch = false;
    }
    if(crunch && optimize && tree.argCount != 0) {
        Tree ret = newOpValue(computeTree(tree, NULL, 0));
        freeTree(out);
        return ret;
    }
    //Unfold custom functions
    if(unfold)
        if(tree.optype == optype_custom) {
            Tree ret = treeCopy(*customfunctions[tree.op].tree, out.branch, true, true, optimize);
            free(out.branch);
            return ret;
        }
    //Return
    return out;
}
int getCharType(char in, int curType, int base, bool useUnits) {
    if((in >= '0' && in <= '9') || in == '.') return 0;
    if(in >= 'a' && in <= 'z') return 1;
    if(in == '_') return 1;
    if(useUnits && ((in >= 'A' && in <= 'Z') || in == '$') && (curType != 0 || (in > 'A' + (int)base - 10))) return 1;
    if((in >= '*' && in <= '/' && in != '.') || in == '%' || in == '^') return 6;
    return -1;
}
Tree generateTree(char* eq, char** argNames, double base) {
    bool useUnits = base != 0;
    if(base == 0) base = 10;
    if(verbose) {
        printf("Generating Operation Tree");
        if(useUnits)
            printf(" (units, base=%g)", base);
        printf("\nInput: %s\n", eq);
    }
    //Divide into sections
    int i, brackets = 0, sectionCount = 0, curType = -1, eqLength = strlen(eq);
    int sections[eqLength + 1];
    /*
        Section Types:
        0 - Number
        1 - Variable or function
        2 - Function
        3 - Bracket
        4 - Square Bracket
        5 - Square Bracket with Base
        6 - Operator
        7 - Vector
        8 - Anonymous Function
    */
    int sectionTypes[eqLength + 1];
    memset(sections, 0, sizeof(sections));
    memset(sectionTypes, 0, sizeof(sectionTypes));
    for(i = 0; i < eqLength; i++) {
        char ch = eq[i];
        //Functions
        if(ch == '(' && curType == 1 && sectionTypes[sectionCount - 1] == 1) {
            curType = 2;
            brackets++;
            sectionTypes[sectionCount - 1] = 2;
            continue;
        }
        //Open brackets
        if(ch == '(' || ch == '[' || ch == '<') {
            brackets++;
            if(brackets != 1) continue;
            int type = ch == '(' ? 3 : (ch == '[' ? 4 : 7);
            curType = type;
            sectionTypes[sectionCount] = type;
            sections[sectionCount++] = i;
        }
        //Closed square brackets with _
        if(ch == ']' && eq[i + 1] == '_' && brackets == 1) {
            sectionTypes[sectionCount - 1] = 5;
            curType = getCharType(eq[i + 2], curType, base, useUnits);
            brackets--;
            i++;
            continue;
        }
        // (x,y)=> arrow notation
        if(ch == ')' && brackets == 1 && eq[i + 1] == '=' && eq[i + 2] == '>') {
            sectionTypes[sectionCount - 1] = 8;
            i += 3;
            curType = getCharType(eq[i], curType, base, useUnits);
            brackets--;
            if(eq[i] == '(' || eq[i] == '[' || eq[i] == '<') {
                brackets++;
            }
            continue;
        }
        //Close brackets
        if(ch == ')' || (eq[i - 1] != '=' && ch == '>') || ch == ']') {
            if(--brackets < 0) {
                error("bracket mismatch 1", NULL);
                return NULLOPERATION;
            }
            continue;
        }
        if(brackets != 0) continue;
        //Arrow notation with a single variable
        if(ch == '=' && curType == 1 && eq[i + 1] == '>') {
            sectionTypes[sectionCount - 1] = 8;
            i += 2;
            curType = getCharType(eq[i], curType, base, useUnits);
            if(eq[i] == '(' || eq[i] == '[' || eq[i] == '<') {
                brackets++;
            }
            continue;
        }
        //Get character type
        int chType = getCharType(ch, curType, base, useUnits);
        //To start a new section
        bool createSection = false;
        if(chType == 0 && curType != 0 && curType != 1) createSection = true;
        if(chType == 6 && curType != 6) createSection = true;
        if(chType == 1 && curType != 1) {
            if(i != 0 && eq[i - 1] == '0')
                if(ch == 'b' || ch == 'x' || ch == 'd' || ch == 'o')
                    continue;
            createSection = true;
        }
        if(createSection) {
            curType = chType;
            sectionTypes[sectionCount] = chType;
            sections[sectionCount++] = i;
        }
        if(i == 0 && sectionCount == 0) sectionCount++;
    }
    sections[sectionCount] = eqLength;
    if(brackets != 0) {
        error("bracket mismatch 0", NULL);
        return NULLOPERATION;
    }
    //Generate operations from strings
    Tree ops[sectionCount];
    if(verbose) printf("Sections(%d): ", sectionCount);
    //set to true when it comes across *- or /- or ^- or %-
    bool nextNegative = false;
    for(i = 0; i < sectionCount; i++) {
        //Get section substring
        int j, sectionLength = sections[i + 1] - sections[i];
        char section[sectionLength + 1];
        memcpy(section, eq + sections[i], sectionLength);
        section[sectionLength] = '\0';
        if(verbose)
            printf("%s, ", section);
        //Parse section
        char first = eq[sections[i]];
        if(sectionTypes[i] == 0) {
            //Number
            double numBase = base;
            bool useBase = false;
            if(first == '0') {
                char base = eq[sections[i] + 1];
                if(base >= 'a') useBase = true;
                if(base == 'b') numBase = 2;
                if(base == 'o') numBase = 8;
                if(base == 'd') numBase = 10;
                if(base == 'x') numBase = 16;
            }
            double numr = parseNumber(section + (useBase ? 2 : 0), numBase);
            ops[i] = newOpVal(numr, 0, 0);
        }
        else if(sectionTypes[i] == 2) {
            //Function
            int j;
            int commas[sectionLength];
            int commaCount = 1;
            int openBracket = 0;
            int brackets = 0;
            //Iterate through all characters to find ','
            for(j = 0; j < sectionLength; j++) {
                if(openBracket == 0 && section[j] == '(')
                    openBracket = j;
                if(section[j] == '(' || section[j] == '<')
                    brackets++;
                else if(section[j] == ')' || (section[j - 1] != '=' && section[j] == '>'))
                    brackets--;
                if(section[j] == ',' && brackets == 1)
                    commas[commaCount++] = j;
            }
            commas[0] = openBracket;
            commas[commaCount] = sectionLength - 1;
            section[openBracket] = '\0';
            Tree funcID = findFunction(section);
            if(funcID.optype == 0 && funcID.op == 0) {
                error("function '%s' does not exist", section);
                return NULLOPERATION;
            }
            if(funcID.optype == optype_builtin && stdfunctions[funcID.op].argCount != commaCount && funcID.op != op_run && (funcID.op != op_ge || commaCount == 2) && (funcID.op != op_fill || commaCount != 3)) {
                error("wrong number of arguments for '%s'", stdfunctions[funcID.op].name);
                return NULLOPERATION;
            }
            if(funcID.optype == optype_custom && customfunctions[funcID.op].argCount != commaCount) {
                error("wrong number of arguments for '%s'", section);
                return NULLOPERATION;
            }
            Tree* args = calloc(commaCount, sizeof(Tree));
            for(j = 0; j < commaCount; j++) {
                char argText[commas[j + 1] - commas[j] + 1];
                memset(argText, 0, sizeof(argText));
                memcpy(argText, section + commas[j] + 1, commas[j + 1] - commas[j] - 1);
                args[j] = generateTree(argText, argNames, 0);
                if(globalError) {
                    free(args);
                    return NULLOPERATION;
                }
            }
            ops[i] = newOp(args, commaCount, funcID.op, funcID.optype);
        }
        else if(sectionTypes[i] == 1) {
            //Variables or units
            Tree op = findFunction(section);
            Number num = NULLNUM;
            if(argNames != NULL) {
                int j = 0;
                while(argNames[j++] != NULL) {
                    if(strcmp(argNames[j - 1], section) == 0) {
                        op.optype = optype_argument;
                        op.op = j - 1;
                    }
                }
            }
            if(op.op == op_val && op.optype == optype_builtin && useUnits) {
                num = getUnitName(section);
                if(num.u == 0) {
                    error("Variable or unit '%s' not found", section);
                    return NULLOPERATION;
                }
            }
            //Check for errors
            else if(op.optype == 0 && op.op == 0) {
                error("Variable '%s' not found", section);
                return NULLOPERATION;
            }
            if(op.optype == optype_builtin)
                if(stdfunctions[op.op].argCount != 0) {
                    error("no arguments for function '%s'", section);
                    return NULLOPERATION;
                }
            //Set operation
            if(op.optype == 0 && op.op == op_val)
                ops[i] = newOpVal(num.r, num.i, num.u);
            else
                ops[i] = newOp(NULL, 0, op.op, op.optype);
        }
        else if(sectionTypes[i] == 6) {
            int opID = 0;
            //For negative operations eg. *-
            if(first == '*' && section[1] == '*')
                opID = op_pow;
            else if(sectionLength > 1) {
                if(section[1] == '-')
                    nextNegative = true;
                else
                    error("'%s' not a valid operator", section);
            }
            else if(first == '-' && i == 0) {
                //This operation is multplied by the next value, it is a place holder
                ops[i] = newOpVal(1, 0, 0);
                nextNegative = true;
                continue;
            }
            else if(first == '^') opID = op_pow;
            else if(first == '%') opID = op_mod;
            else if(first == '*') opID = op_mult;
            else if(first == '/') opID = op_div;
            else if(first == '+') opID = op_add;
            else if(first == '-') opID = op_sub;
            else error("'%s' is not a valid operator", section);
            ops[i] = newOp(NULL, 0, opID, 0);
            continue;
        }
        else if(sectionTypes[i] == 8) {
            char** argList = parseArgumentList(section);
            char** appendedArgList = mergeArgList(argNames, argList);
            int eqPos = 0;
            while(section[++eqPos] != '=');
            if(section[eqPos + 2] == '(') {
                eqPos++;
                section[sectionLength - 1] = '\0';
            }
            Tree tree = generateTree(section + eqPos + 2, appendedArgList, base);
            free(appendedArgList);
            Tree out;
            out.branch = malloc(sizeof(Tree));
            *(out.branch) = tree;
            out.optype = optype_anon;
            out.argNames = argList;
            out.argCount = argListLen(argList);
            out.argWidth = argListLen(argNames);
            out.op = 0;
            ops[i] = out;
        }
        else if(first == '(') {
            //Round bracket
            section[sectionLength - 1] = '\0';
            ops[i] = generateTree(section + 1, argNames, 0);
            if(globalError)
                return NULLOPERATION;
        }
        else if(first == '[') {
            //Unit bracket
            double base = 10;
            if(sectionTypes[i] == 5) {
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
                Value baseNum = computeTree(tree, NULL, 0);
                if(globalError)
                    return NULLOPERATION;
                freeTree(tree);
                if(baseNum.type == value_num) base = baseNum.r;
                section[j - 1] = '\0';
            }
            else
                section[sectionLength - 1] = '\0';
            ops[i] = generateTree(section + 1, argNames, base);
            if(globalError)
                return NULLOPERATION;
        }
        else if(first == '<') {
            int width = 1;
            int height = 1;
            int x, y;
            int brackets = 0;
            int currWidth = 1;
            for(x = 1;x < sectionLength;x++) {
                if(section[x] == '(' || section[x] == '[' || section[x] == '<') brackets++;
                if(section[x] == ')' || section[x] == ']' || section[x] == '>') brackets--;
                if(brackets == 0 && section[x] == ',') {
                    currWidth++;
                }
                if(brackets == 0 && section[x] == ';') {
                    height++;
                    if(currWidth > width) width = currWidth;
                    currWidth = 1;
                }
            }
            if(currWidth > width) width = currWidth;
            int pos[height][width];
            memset(pos, 0, sizeof pos);
            int columnID = 1;
            int rowID = 0;
            pos[0][0] = 0;
            brackets = 0;
            for(x = 1;x < sectionLength;x++) {
                if(section[x] == '(' || section[x] == '[' || section[x] == '<') brackets++;
                if(section[x] == ')' || section[x] == ']' || section[x] == '>') brackets--;
                if(brackets == 0 && section[x] == ',') {
                    section[x] = '\0';
                    pos[rowID][columnID++] = x;
                }
                if(brackets == 0 && section[x] == ';') {
                    section[x] = '\0';
                    columnID = 1;
                    pos[++rowID][0] = x;
                }
            }
            section[sectionLength - 1] = '\0';
            Tree* args = calloc(width * height, sizeof(Tree));
            for(y = 0;y < height;y++) for(x = 0;x < width;x++) {
                if(pos[y][x] == 0 && x != 0) {
                    args[x + y * width] = NULLOPERATION;
                    continue;
                }
                args[x + y * width] = generateTree(section + pos[y][x] + 1, argNames, useUnits ? base : 0);
            }
            ops[i] = newOp(args, width * height, op_vector, 0);
            ops[i].argWidth = width;
        }
        else {
            error("unable to parse '%s'", section);
            return NULLOPERATION;
        }
        if(nextNegative && !(first > '*' && first < '/') && first != '^' && first != '%') {
            nextNegative = false;
            ops[i] = newOp(allocArg(ops[i], false), 1, op_neg, 0);
        }
    }
    //Compile operations into tree
    if(verbose) {
        printf("\nOperation List(%d): ", sectionCount);
        for(i = 0; i < sectionCount; i++) {
            if(i != 0)
                printf(", ");
            char* operationString = treeToString(ops[i], false, argNames);
            printf("%s", operationString);
            free(operationString);
        }
    }
    int offset = 0;
    //Side-by-side multiplication
    for(i = 1; i < sectionCount; i++) {
        ops[i] = ops[i + offset];
        //Check if ineligible
        if(ops[i].op >= op_pow && ops[i].op <= op_sub && ops[i].argCount == 0) continue;
        if(ops[i - 1].op >= op_pow && ops[i - 1].op <= op_sub && ops[i - 1].argCount == 0) continue;
        //Combine them
        ops[i - 1] = newOp(allocArgs(ops[i - 1], ops[i], 0, 0), 2, op_mult, 0);
        offset++;
        sectionCount--;
        i--;
    }
    ops[i] = ops[i + offset];
    //Condense operations: ^%*/+-
    for(i = 3; i < 9; i++) {
        offset = 0;
        int j;
        for(j = 0; j < sectionCount + offset; j++) {
            ops[j] = ops[j + offset];
            if(ops[j].op != i || ops[j].optype != optype_builtin || ops[j].argCount != 0) continue;
            if(j == 0 || j == sectionCount - 1) {
                error("missing argument in operation", NULL);
                return NULLOPERATION;
            }
            ops[j] = newOp(allocArgs(ops[j - 1], ops[j + 1 + offset], 0, 0), 2, i, 0);
            ops[j - 1] = ops[j];
            j--;
            sectionCount -= 2;
            offset += 2;
        }
    }
    if(verbose) {
        char* operationString = treeToString(ops[0], false, argNames);
        printf("\nFinal Tree(%d): %s\n", sectionCount, operationString);
        free(operationString);
    }
    return ops[0];
}
Tree derivative(Tree tree) {
    if(tree.optype == optype_anon) {
        error("anonymous functions are not supported in dx");
    }
    //returns the derivative of tree, output must be freeTree()ed
    //Source: https://en.wikipedia.org/wiki/Differentiation_rules
    //x, variables, and i
    if(tree.optype == optype_argument || tree.argCount == 0 || tree.op == op_hist || tree.op == op_val) {
        //return a derivative of 1 if it's x, or 0 if it's a constant
        return newOpVal(tree.op == 0 && tree.optype == optype_argument ? 1 : 0, 0, 0);
    }
    //Basic Operations
    if(tree.op < 9) {
        //DOne is the derivative of one
        //DTwo is the derivative of two
        Tree DOne = derivative(tree.branch[0]);
        if(tree.op == op_neg) return newOp(allocArg(DOne, 0), 1, op_neg, 0);
        Tree DTwo = derivative(tree.branch[1]);
        if(globalError)
            return NULLOPERATION;
        if(tree.op == op_add || tree.op == op_sub)
            return newOp(allocArgs(DOne, DTwo, 0, 0), 2, tree.op, 0);
        if(tree.op == op_mult) {
            //f(x)*g'(x) + g(x)*f'(x)
            Tree DoneTwo, DtwoOne;
            if(!treeIsZero(DOne))
                DoneTwo = newOp(allocArgs(tree.branch[1], DOne, 1, 0), 2, op_mult, 0);
            else
                DoneTwo = newOpVal(0, 0, 0);
            if(!treeIsZero(DTwo))
                DtwoOne = newOp(allocArgs(tree.branch[0], DTwo, 1, 0), 2, op_mult, 0);
            else
                return DoneTwo;
            if(treeIsZero(DoneTwo))
                return DtwoOne;
            else
                return newOp(allocArgs(DoneTwo, DtwoOne, 0, 0), 2, op_add, 0);
        }
        if(tree.op == op_div) {
            //(f'x*gx - g'x*fx) / (gx)^2
            Tree DoneTwo, DtwoOne, out;
            if(!treeIsZero(DOne))
                DoneTwo = newOp(allocArgs(DOne, tree.branch[1], 0, 1), 2, op_mult, 0);
            else
                DoneTwo = newOpVal(0, 0, 0);
            if(!treeIsZero(DTwo))
                DtwoOne = newOp(allocArgs(DTwo, tree.branch[0], 0, 1), 2, op_mult, 0);
            else
                DtwoOne = newOpVal(0, 0, 0);
            if(treeIsZero(DoneTwo))
                out = DtwoOne;
            else if(treeIsZero(DtwoOne))
                out = DoneTwo;
            else
                out = newOp(allocArgs(DoneTwo, DtwoOne, 0, 0), 2, op_sub, 0);
            Tree gxSq = newOp(allocArgs(tree.branch[1], newOpVal(2, 0, 0), 1, 0), 2, op_pow, 0);
            return newOp(allocArgs(out, gxSq, 0, 0), 2, op_div, 0);
        }
        if(tree.op == op_mod) {
            error("modulo not supported in dx.", NULL);
            return NULLOPERATION;
        }
        if(tree.op == op_pow) {
            //fx^gx * g'x * ln(fx)  +  fx^(g(x)-1) * gx * f'x
            Tree p1, p2;
            if(!treeIsZero(DTwo)) {
                Tree fxgx = newOp(allocArgs(tree.branch[0], tree.branch[1], 1, 1), 2, op_pow, 0);
                Tree lnfx = newOp(allocArg(tree.branch[0], 1), 1, op_ln, 0);
                Tree fxgxlnfx = newOp(allocArgs(fxgx, lnfx, 0, 0), 2, op_mult, 0);
                if(treeIsOne(DTwo))
                    p1 = fxgxlnfx;
                else
                    p1 = newOp(allocArgs(fxgxlnfx, DTwo, 0, 0), 2, op_mult, 0);
            }
            else
                p1 = newOpVal(0, 0, 0);
            if(!treeIsZero(DOne)) {
                Tree gm1 = newOp(allocArgs(tree.branch[1], newOpVal(1, 0, 0), 1, 0), 2, op_sub, 0);
                Tree fxgm1 = newOp(allocArgs(tree.branch[0], gm1, 1, 0), 2, op_pow, 0);
                Tree fxgm1gx = newOp(allocArgs(fxgm1, tree.branch[1], 0, 1), 2, op_mult, 0);
                if(treeIsOne(DOne))
                    p2 = fxgm1gx;
                else
                    p2 = newOp(allocArgs(fxgm1gx, DOne, 0, 0), 2, op_mult, 0);
            }
            else
                return p1;
            if(treeIsZero(p1))
                return p2;
            return newOp(allocArgs(p1, p2, 0, 0), 2, op_add, 0);
        }
    }
    //Trigonomety
    if(tree.op < 27) {
        Tree DOne = derivative(tree.branch[0]);
        if(tree.op < 15) {
            Tree out;
            if(tree.op == op_sin) {
                out = newOp(allocArg(tree.branch[0], 1), 1, op_cos, 0);
            }
            if(tree.op == op_cos) {
                out = newOp(allocArg(tree.branch[0], 1), 1, op_sin, 0);
                out = newOp(allocArg(out, 0), 1, op_neg, 0);
            }
            if(tree.op == op_tan) {
                out = newOp(allocArg(tree.branch[0], 1), 1, op_sec, 0);
                out = newOp(allocArgs(out, newOpVal(2, 0, 0), 0, 0), 2, op_pow, 0);
            }
            if(tree.op == op_cot) {
                out = newOp(allocArg(tree.branch[0], 1), 1, op_csc, 0);
                out = newOp(allocArgs(out, newOpVal(2, 0, 0), 0, 0), 2, op_pow, 0);
                out = newOp(allocArg(out, 0), 1, op_neg, 0);
            }
            if(tree.op == op_sec) {
                Tree sec = newOp(allocArg(tree.branch[0], 1), 1, op_sec, 0);
                out = newOp(allocArg(tree.branch[0], 1), 1, op_tan, 0);
                out = newOp(allocArgs(sec, out, 0, 0), 2, op_mult, 0);
            }
            if(tree.op == op_csc) {
                Tree csc = newOp(allocArg(tree.branch[0], 1), 1, op_csc, 0);
                out = newOp(allocArg(tree.branch[0], 1), 1, op_cot, 0);
                out = newOp(allocArgs(csc, out, 0, 0), 2, op_mult, 0);
                out = newOp(allocArg(out, 0), 1, op_neg, 0);
            }
            if(treeIsOne(DOne)) return out;
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_mult, 0);
        }
        if(tree.op < 18) {
            Tree out;
            if(tree.op == op_sinh)
                out = newOp(allocArg(tree.branch[0], 1), 1, op_cosh, 0);
            if(tree.op == op_cosh)
                out = newOp(allocArg(tree.branch[0], 1), 1, op_cosh, 0);
            if(tree.op == op_tanh) {
                out = newOp(allocArg(tree.branch[0], 1), 1, op_tanh, 0);
                out = newOp(allocArgs(out, newOpVal(2, 0, 0), 0, 0), 2, op_pow, 0);
                out = newOp(allocArgs(newOpVal(1, 0, 0), out, 0, 0), 2, op_sub, 0);
            }
            if(treeIsOne(DOne)) return out;
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_mult, 0);
        }
        if(tree.op < 24) {
            Tree out;
            if(tree.op == op_asin || tree.op == op_acos) {
                out = newOp(allocArgs(tree.branch[0], newOpVal(2, 0, 0), 1, 0), 2, op_pow, 0);
                out = newOp(allocArgs(newOpVal(1, 0, 0), out, 0, 0), 2, op_sub, 0);
                out = newOp(allocArg(out, 0), 1, op_sqrt, 0);
            }
            if(tree.op == op_atan || tree.op == op_acot) {
                out = newOp(allocArgs(tree.branch[0], newOpVal(2, 0, 0), 1, 0), 2, op_pow, 0);
                out = newOp(allocArgs(out, newOpVal(1, 0, 0), 0, 0), 2, op_add, 0);
            }
            if(tree.op == op_asec || tree.op == op_acsc) {
                Tree abs = newOp(allocArg(tree.branch[0], 1), 1, op_abs, 0);
                out = newOp(allocArgs(tree.branch[0], newOpVal(2, 0, 0), 1, 0), 2, op_pow, 0);
                out = newOp(allocArgs(out, newOpVal(1, 0, 0), 0, 0), 2, op_sub, 0);
                out = newOp(allocArg(out, 0), 1, op_sqrt, 0);
                out = newOp(allocArgs(abs, out, 0, 0), 2, op_div, 0);
            }
            //Negate
            if(tree.op == op_acos || tree.op == op_acot || tree.op == op_acsc)
                out = newOp(allocArg(out, 0), 1, op_neg, 0);
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_div, 0);
        }
        if(tree.op < 27) {
            Tree out = newOp(allocArgs(tree.branch[0], newOpVal(2, 0, 0), 1, 0), 2, op_pow, 0);
            if(tree.op == op_asinh) {
                out = newOp(allocArgs(out, newOpVal(1, 0, 0), 0, 0), 2, op_add, 0);
                out = newOp(allocArg(out, 0), 1, op_sqrt, 0);
            }
            if(tree.op == op_acosh) {
                out = newOp(allocArgs(out, newOpVal(1, 0, 0), 0, 0), 2, op_sub, 0);
                out = newOp(allocArg(out, 0), 1, op_sqrt, 0);
            }
            if(tree.op == op_atanh) {
                out = newOp(allocArgs(newOpVal(1, 0, 0), out, 0, 0), 2, op_sub, 0);
            }
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_div, 0);
        }
    }
    //Ln, arg, abs
    if(tree.op < 38) {
        Tree DOne = derivative(tree.branch[0]);
        if(tree.op == op_sqrt) {
            Tree out = newOp(allocArg(tree.branch[0], 1), 1, op_sqrt, 0);
            out = newOp(allocArgs(out, newOpVal(2, 0, 0), 0, 0), 2, op_mult, 0);
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_div, 0);
        }
        if(tree.op == op_cbrt) {
            freeTree(DOne);
            Tree cbrt = newOp(allocArgs(tree.branch[0], newOpVal(1 / 3, 0, 0), 1, 0), 2, op_pow, 0);
            Tree out = derivative(cbrt);
            freeTree(cbrt);
            return out;
        }
        if(tree.op == op_exp) {
            Tree out = newOp(allocArg(tree.branch[0], 1), 1, op_exp, 0);
            if(treeIsOne(DOne)) return out;
            return newOp(allocArgs(DOne, out, 0, 0), 2, op_div, 0);
        }
        if(tree.op == op_ln)
            return newOp(allocArgs(DOne, tree.branch[0], 0, 1), 2, op_div, 0);
        freeTree(DOne);
    }
    //Rounding
    if(tree.op < 50) {
        if(tree.op == op_round || tree.op == op_floor || tree.op == op_ceil)
            return newOpVal(0, 0, 0);
    }
    //Vectors
    if(tree.op < 66 && tree.op > 64) {
        if(tree.op == op_vector) {
            Tree out;
            out.op = op_vector;
            out.optype = optype_builtin;
            out.argCount = tree.argCount;
            out.argWidth = tree.argWidth;
            out.branch = malloc(tree.argCount * sizeof(Tree));
            int i;
            for(i = 0;i < tree.argCount;i++) {
                out.branch[i] = derivative(tree.branch[i]);
            }
            return out;
        }
    }
    error("not all functions are supported in dx currently");
    return NULLOPERATION;
}
Value calculate(char* eq, double base) {
    //Clean input
    char* cleanInput = inputClean(eq);
    if(globalError) return NULLVAL;
    //Generate tree
    Tree tree = generateTree(base == 0 ? cleanInput : eq, NULL, base);
    free(cleanInput);
    if(globalError) return NULLVAL;
    //Compute tree
    Value ans = computeTree(tree, NULL, 0);
    freeTree(tree);
    if(globalError) return NULLVAL;
    return ans;
}
#pragma endregion
#pragma region Functions
void freeArgList(char** argList) {
    if(argList == NULL) return;
    int i = -1;
    while(argList[++i]) free(argList[i]);
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
    int i;
    for(i = 0;i < len;i++) {
        int strLen = strlen(argList[i]);
        out[i] = calloc(strLen + 1, 1);
        strcpy(out[i], argList[i]);
    }
    return out;
}
char** mergeArgList(char** one, char** two) {
    int oneLen = 0, twoLen = 0;
    if(one != NULL) oneLen = argListLen(one);
    if(two != NULL) twoLen = argListLen(two);
    char** out = calloc(oneLen + twoLen + 1, sizeof(char*));
    memcpy(out, one, oneLen * sizeof(char*));
    memcpy(out + oneLen, two, twoLen * sizeof(char*));
    return out;
}
char* argListToString(char** argList) {
    int totalLen = 0;
    int i = -1;
    while(argList[++i]) {
        totalLen += strlen(argList[i]) + 1;
    }
    if(i == 1) {
        char* out = calloc(strlen(argList[0]), 1);
        strcpy(out, argList[0]);
        return out;
    }
    char* out = calloc(totalLen + 3, 1);
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
char** parseArgumentList(char* list) {
    if(list[0] == '=') {
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
    int j;
    for(i = 0;i < argCount;i++) {
        int stringPos = 0;
        out[i] = calloc(commaPos[i + 1] - commaPos[i], 1);
        for(j = commaPos[i] + 1;j < commaPos[i + 1];j++) {
            if((list[j] >= 'a' && list[j] <= 'z') || (list[j] >= '0' && list[j] <= '9')) out[i][stringPos++] = list[j];
            else if(list[j] >= 'A' && list[j] <= 'Z') out[i][stringPos++] = list[j] + 32;
            else if(list[j] == '(' || list[j] == ')' || list[j] == ' ' || list[j] == '=' || list[j] == '\0') continue;
            else error("invalid '%c' in argument list", list[j]);
        }
        if(out[i][0] < 'a') error("argument name '%s' starts with a numeral", out[i]);
        if(globalError) {
            for(;i >= 0;i--) free(out[i]);
            free(out);
            return NULL;
        }
    }
    return out;
}
Function newFunction(char* name, Tree* tree, char argCount, char** argNames) {
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
    //Compute tree
    Tree* tree = malloc(sizeof(Tree));
    char* cleaninput = inputClean(eq + equalPos + 1);
    *tree = generateTree(cleaninput, argNames, 0);
    free(cleaninput);
    if(globalError) {
        free(name);
        int i = 0;
        while(argNames[i] != NULL) free(argNames[i++]);
        free(argNames);
        return;
    }
    //Append to functions
    if(functionArrayLength == numFunctions) {
        customfunctions = realloc(customfunctions, (functionArrayLength + 10) * sizeof(Function));
        functionArrayLength += 10;
    }
    customfunctions[numFunctions++] = newFunction(name, tree, argCount, argNames);
}
Tree findFunction(char* name) {
    Tree out;
    //Get function id from name
    int len = strlen(name), i;
    out.optype = 0;
    for(i = 1; i < immutableFunctions; i++) {
        if(stdfunctions[i].nameLen != len)
            continue;
        if(strcmp(name, stdfunctions[i].name) == 0) {
            out.op = i;
            return out;
        }

    }
    out.optype = 1;
    for(i = 0;i < numFunctions;i++) {
        if(customfunctions[i].nameLen != len) continue;
        if(strcmp(customfunctions[i].name, name) != 0) continue;
        out.op = i;
        return out;
    }
    out.op = 0;
    out.optype = 0;
    return out;
}
const char* getFunctionName(int optype, int op) {
    return NULL;
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
    {1, 3, "sgn"}, {1, 3, "abs"}, {1, 3, "arg"}, {1, 4, "norm"},
    {1, 5, "round"}, {1, 5, "floor"}, {1, 4, "ceil"}, {1, 4, "getr"}, {1, 4, "geti"}, {1, 4, "getu"}, {2, 6, "grthan"}, {2, 5, "equal"}, {2, 3, "min"}, {2, 3, "max"}, {3, 4, "lerp"}, {2, 4, "dist"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {1, 3, "not"}, {0, 0, " "}, {2, 3, "and"}, {2, 2, "or"}, {2, 3, "xor"}, {2, 2, "ls"}, {2, 2, "rs"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 2, "pi"}, {0, 3, "phi"}, {0, 1, "e"},
{0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 3, "ans"}, {1, 4, "hist"}, {0, 6, "histnum"}, {0, 4, "rand"},
    {0, 0, " "}, {0, 0, " "},
    {1, 3, "run"}, {4, 3, "sum"}, {4, 7, "product"},
    {0, 0, " "}, {0, 0, " "}, {0, 0, " "},
    {0, 0, " "}, {1, 5, "width"}, {1, 6, "height"}, {1, 6, "length"}, {3, 2, "ge"}, {2, 4, "fill"}, {2, 3, "map"}, {1, 3, "det"}, {1, 9, "transpose"}, {2, 8, "mat_mult"}, {1, 7, "mat_inv"}
};
#pragma endregion
#pragma region Main Program
char* inputClean(char* input) {
    if(input[0] == '\0') {
        error("no equation", NULL);
        return NULL;
    }
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
        if((in > '9' && in < 'A' && in != '>' && in != '<' && in != ';' && in != '=') || (in < '$' && in > ' ') || in == '&' || in == '\'' || in == '\\' || in == '`' || in > 'z' || (in == '$' && insideSquare == false)) {
            error("invalid character '%c'", in);
            free(out);
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
    for(i = 0; i < numFunctions; i++) {
        if(customfunctions[i].tree == NULL) continue;
        free(customfunctions[i].name);
        freeTree(*customfunctions[i].tree);
        free(customfunctions[i].tree);
        char** argNames = customfunctions[i].argNames;
        int j = -1;
        while(argNames[++j] != NULL) free(argNames[j]);
        free(argNames);
    }
    //Free history
    for(i = 0;i < historyCount;i++) {
        freeValue(history[i]);
    }
    //Free global array
    free(customfunctions);
    free(history);
}
void startup() {
    //Constants
    NULLNUM = newNum(0, 0, 0);
    NULLOPERATION = newOpVal(0, 0, 0);
    NULLVAL = newValNum(0, 0, 0);
    //Allocate history
    history = calloc(10, sizeof(Value));
    historySize = 10;
    //Allocate functions
    customfunctions = calloc(functionArrayLength, sizeof(Function));
}
#pragma endregion
#pragma region Misc
int* primeFactors(int num) {
    int* out = calloc(11, sizeof(int));
    int outSize = 10;
    int outPos = 0;
    int max = num / 2;
    //Remove factors of 2
    while(num % 2 == 0) {
        out[outPos++] = 2;
        num = num / 2;
        if(outPos == outSize) {
            out = realloc(out, (outSize + 11) * sizeof(int));
            memset(out + outSize, 0, 11 * sizeof(int));
            outSize += 10;
        }
    }
    int i;
    for(i = 3;i <= max;i += 2) {
        while(num % i == 0) {
            out[outPos++] = i;
            num = num / i;
            if(outPos == outSize) {
                out = realloc(out, (outSize + 11) * sizeof(int));
                memset(out + outSize, 0, 11 * sizeof(int));
                outSize += 10;
            }
        }
    }
    return out;
}
bool isPrime(int num) {
    if(num < 0) num = -num;
    //If i is a multiple of 6i, 6i+2, 6i+4
    if(num % 2 == 0) return false;
    //If i is a multiple of 6i+3
    if(num % 3 == 0) return false;
    //If i is a multiple of 5
    if(num % 5 == 0) return false;
    int i;
    int sqrtN = sqrt(num) + 1;
    for(i = 7;i < sqrtN;i += 6) {
        //If num is a multiple of 6i+1
        if(num % i == 0) return false;
        //If num is a multiple of 6i+5
        if(num % (i + 4) == 0) return false;
    }
    return true;
}
void getRatio(double num, int* numerOut, int* denomOut) {
    if(num == floor(num)) return;
    if(num < 0) num = -num;
    if(num > 1) num = fmod(num, 1);
    double origNum = num;
    int contFraction[40];
    memset(contFraction, 0, 40 * sizeof(int));
    num = 1 / num;
    int i;
    for(i = 0;i < 20;i++) {
        if((int)num <= 0) {
            i--;
            break;
        }
        contFraction[i] = (int)num;
        num -= (int)num;
        if(num < 1e-6) break;
        num = 1 / num;
    }
    if(i == 20) return;
    int numer = contFraction[i--];
    int denom = 1;
    while(i >= 0) {
        int newNumer = contFraction[i] * numer + denom;
        denom = numer;
        numer = newNumer;
        i--;
    }
    *numerOut = denom;
    *denomOut = numer;
}
#pragma endregion