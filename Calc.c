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
bool ignoreError = false;
int globalAccuracy = 0;
int digitAccuracy = 0;
bool useArb = false;
Number NULLNUM;
Value* history;
Value NULLVAL;
Tree NULLOPERATION;
Function* customfunctions;
int functionArrayLength = 10;
int numFunctions = 0;
const char numberChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char* mallocError = "malloc returned null";
#pragma endregion
#pragma region Units
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
    {"mach",273,0xFF0001},
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
    {"st",6.35029318,0x0100},
    {"lb",0.45359237,0x0100},
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
};
const char metricNums[] = "yzafpnumchkMGTPEZY";
const double metricNumValues[] = { 0.000000000000000000000001, 0.000000000000000000001, 0.000000000000000001, 0.000000000000001, 0.000000000001, 0.000000001, 0.000001, 0.001, 0.01, 100, 1000, 1000000.0, 1000000000.0, 1000000000000.0, 1000000000000000.0, 1000000000000000000.0, 1000000000000000000000.0, 1000000000000000000000000.0 };
const char* baseUnits[] = { "m", "kg", "s", "A", "K", "mol", "$", "bit" };
unitStandard newUnit(const char* name, double mult, unit_t units) {
    unitStandard out;
    out.name = name;
    out.multiplier = mult;
    out.baseUnits = units;
    return out;
}
Number getUnitName(const char* name) {
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
            if(out == NULL) { error(mallocError);return NULL; }
            strcat(out, unitList[i].name);
            return out;
        }
    }
    //Else generate a custom string
    char* out = calloc(54, 1);
    if(out == NULL) { error(mallocError);return NULL; }
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
#pragma region Arbitrary Precision
int getArbDigitCount(int base) {
    if(base == 10) return digitAccuracy;
    return (int)(digitAccuracy * log(10) / log(base)) + 1;
}
Arb arbCTR(unsigned char* mant, short len, short exp, char sign, short accu) {
    Arb out;
    out.mantissa = mant;
    out.len = len;
    out.exp = exp;
    out.sign = sign;
    out.accu = accu;
    return out;
}
Arb copyArb(Arb arb) {
    Arb out = arb;
    out.mantissa = malloc(out.len);
    memcpy(out.mantissa, arb.mantissa, out.len);
    return out;
}
void freeArb(Arb arb) {
    if(arb.mantissa != NULL) free(arb.mantissa);
    return;
}
void trimZeroes(Arb* arb) {
    int leftCount = 0;
    int i = -1;
    while(arb->mantissa[++i] == 0 && i != arb->len) leftCount++;
    if(leftCount != 0) {
        if(leftCount == arb->len) leftCount--;
        arb->exp -= leftCount;
        arb->len -= leftCount;
        memmove(arb->mantissa, arb->mantissa + leftCount, arb->len);
    }
    while(arb->mantissa[arb->len - 1] == 0 && arb->len != 1) arb->len--;
    if(arb->len > arb->accu) {
        arb->len = arb->accu;
        if(arb->mantissa[arb->len] > 128) {
            while(arb->mantissa[arb->len - 1] == 255) arb->len--;
            if(arb->len == 0) {
                arb->len = 1;
                arb->mantissa[0] = 1;
                arb->exp++;
            }
            else arb->mantissa[arb->len - 1]++;
        }
    }
}
void arbRightShift(Arb* arb, int count) {
    arb->exp += count;
    arb->len += count;
    if(arb->len > arb->accu) arb->len = arb->accu;
    memmove(arb->mantissa + count, arb->mantissa, arb->len - count);
    memset(arb->mantissa, 0, count);
}
int arbCmp(Arb one, Arb two) {
    //Note: this function ignores the sign of a number
    if(one.len == 1 && one.mantissa[0] == 0) {
        if(two.len == 1 && two.mantissa[0] == 0) return 0;
        return -1;
    }
    if(two.len == 1 && two.mantissa[0] == 0) return 1;
    if(one.exp > two.exp) return 1;
    if(one.exp < two.exp) return -1;
    int len = one.len > two.len ? two.len : one.len;
    int cmp = memcmp(one.mantissa, two.mantissa, len);
    if(cmp != 0) return cmp;
    if(one.len == two.len) return 0;
    if(one.len > two.len) {
        int i = len;
        while(++i != one.len) if(one.mantissa[i] != 0) return 1;
    }
    if(two.len > one.len) {
        int i = len - 1;
        while(++i != two.len) if(two.mantissa[i] != 0) return -1;
    }
    return 0;
}
#pragma region Arbitrary Precision Conversion
Arb parseArb(char* string, int base, int accu) {
    int strLen = strlen(string);
    int digits[strlen(string) + 1];
    int power = -1;
    int maxDigits = (accu + 2) * log(256) / log(base);
    bool hasDecimal = false;
    int digitIndex = 0;
    int stringIndex = 0;
    //Read digits to array
    while(digitIndex != maxDigits) {
        char ch = string[stringIndex++];
        if(ch == '.') {
            hasDecimal = true;
            continue;
        }
        if(ch == '\0') break;
        if(!hasDecimal) power++;
        if(ch == '0' && digitIndex == 0) {
            power--;
            continue;
        }
        if(ch <= '9') digits[digitIndex] = ch - '0';
        else if(ch <= 'Z') digits[digitIndex] = ch - 'A' + 10;
        else if(ch <= 'z') digits[digitIndex] = ch - 'a' + 10;
        digitIndex++;
    }
    unsigned char* baseMant = calloc(1, 1);
    baseMant[0] = base;
    //Set base numbers
    Arb baseArb = arbCTR(baseMant, 1, 0, 0, accu + 10);
    Arb baseInv = arb_recip(baseArb);
    //Set out
    Arb out = arbCTR(calloc(1, 1), 1, 0, 0, accu);
    //Set current base
    Arb curBase = arbCTR(calloc(1, 1), 1, 0, 0, accu);
    curBase.mantissa[0] = 1;
    //Integer digits
    int i;
    for(i = power;i >= 0;i--) {
        if(i < maxDigits) {
            Arb val = multByInt(curBase, digits[i]);
            Arb newOut = arb_add(out, val);
            freeArb(val);
            freeArb(out);
            out = newOut;
        }
        if(i != 0) {
            Arb newCurBase = multByInt(curBase, base);
            freeArb(curBase);
            curBase = newCurBase;
        }
    }
    //Fractional digits
    freeArb(curBase);
    curBase = copyArb(baseInv);
    for(i = power + 1;i < digitIndex;i++) {
        if(i < maxDigits && i >= 0) {
            Arb val = multByInt(curBase, digits[i]);
            Arb newOut = arb_add(out, val);
            freeArb(val);
            freeArb(out);
            out = newOut;
        }
        if(i + 1 != maxDigits && i + 1 != digitIndex) {
            Arb newCurBase = arb_mult(curBase, baseInv);
            freeArb(curBase);
            curBase = newCurBase;
            if(-curBase.exp > accu + 1) break;
        }
    }
    //Free values and return
    freeArb(baseArb);
    freeArb(baseInv);
    freeArb(curBase);
    out.accu = accu;
    trimZeroes(&out);
    return out;
}
Arb doubleToArb(double val, int accu) {
    Arb out = arbCTR(calloc(10, 1), 1, 0, val < 0 ? 1 : 0, accu);
    if(val < 0) val = -val;
    while(val > 256) {
        val /= 256;
        out.exp++;
    }
    while(out.len < 10) {
        if(val == 0) break;
        out.len++;
        out.mantissa[out.len - 1] = floor(val);
        val -= floor(val);
        val *= 256;
    }
    return out;
}
double arbToDouble(Arb arb) {
    double out = 0;
    long i;
    int maxLen = 8;
    if(arb.len < 8) maxLen = arb.len;
    for(i = 0;i < maxLen;i++) {
        out += ((double)arb.mantissa[i]) / (double)(1L << (i * 8L));
    }
    out *= pow(256.0, (double)arb.exp);
    if(arb.sign == 1) out = -out;
    return out;
}
char* arbToString(Arb arb, int base, int digitCount) {
    if(arb.len == 0 || (arb.len == 1 && arb.mantissa[0] == 0)) {
        char* out = calloc(2, 1);
        out[0] = '0';
        return out;
    }
    if(base == 16) {
        int outPos = 0;
        char* out = calloc(arb.len * 2 + 2, 1);
        if(arb.exp < 0) {
            //out = realloc(
        }
        int i;
        for(i = 0;i < arb.len;i++) {
            unsigned char val = arb.mantissa[i];
            if(outPos != 0 || val >> 4 != 0) out[outPos++] = numberChars[val >> 4];
            if(i != arb.len - 1 || (val & 15) == 0) out[outPos++] = numberChars[val & 15];
            if(arb.exp - i) out[outPos++] = '.';
        }
        return out;
    }
    Arb floor = arb_floor(arb);
    Arb fraction = arb_subtract(arb, floor);
    unsigned char digits[digitCount + 10];
    memset(digits, 0, digitCount + 10);
    int i = digitCount + 10;
    int exponent = 0;
    //Add integer digits
    while(floor.len != 1 || floor.mantissa[0] != 0) {
        i--;
        int digit = 0;
        Arb new = arb_divModInt(floor, base, &digit);
        freeArb(floor);
        digits[i] = numberChars[digit];
        floor = new;
        if(i == 0) {
            memmove(digits + 10, digits, digitCount);
            i = 10;
        }
    }
    freeArb(floor);
    if(i != digitCount + 10) {
        memmove(digits, digits + i, digitCount + 10 - i);
        memset(digits + digitCount + 10 - i, 0, i);
    }
    i = digitCount + 10 - i;
    exponent = i - 1;
    //Remove trailing zeros fraction if floor==0
    while(digits[0] == '\0') {
        Arb newFraction = multByInt(fraction, base);
        freeArb(fraction);
        fraction = newFraction;
        //If newfraction is not zero: append to list and break
        if(fraction.exp == 0) {
            digits[0] = numberChars[fraction.mantissa[0]];
            fraction.mantissa[0] = 0;
            i++;
            break;
        }
        exponent--;
    }
    //Fractional Digits (repeatedly multByInt)
    for(;i < digitCount;i++) {
        Arb newFraction = multByInt(fraction, base);
        freeArb(fraction);
        fraction = newFraction;
        //If (fraction equals zero) break;
        if(fraction.len == 1 && fraction.mantissa[0] == 0) break;
        //Get digit to add
        char digitToAdd = '0';
        if(fraction.exp == 0) {
            digitToAdd = numberChars[fraction.mantissa[0]];
            fraction.mantissa[0] = 0;
        }
        digits[i] = digitToAdd;
    }
    freeArb(fraction);
    //Turn digits into printable number
    int digitLen = strlen((char*)digits);
    //Write in exponent notation
    if(exponent < -15 || exponent > digitCount) {
        char* out = calloc(digitLen + 12, 1);
        out[0] = digits[0];
        out[1] = '.';
        memcpy(out + 2, digits + 1, digitLen - 1);
        out[digitLen + 1] = 'e';
        snprintf(out + digitLen + 2, 8, "%d", exponent);
        return out;
    }
    //Write fractional number with zero padding
    else if(exponent < 0) {
        char* out = calloc(digitLen + 5 - exponent, 1);
        out[0] = '0';
        out[1] = '.';
        memset(out + 2, '0', -exponent - 1);
        memcpy(out - exponent + 1, digits, digitLen);
        return out;
    }
    //Write normally
    else {
        char* out = calloc(digitLen + 3, 1);
        int outPos = 0;
        for(int i = 0;i < digitLen;i++) {
            out[outPos++] = digits[i];
            if(i == exponent && i != digitLen - 1) out[outPos++] = '.';
        }
        return out;
    }
}
#pragma endregion
#pragma region Arbitary Precision Functions
Arb multByInt(Arb one, int two) {
    int carry = 0;
    int i;
    char sign = two < 0 ? 1 : 0;
    Arb out = arbCTR(calloc(one.len + 4, 1), one.len, one.exp, one.sign ^ sign, one.accu);
    if(two < 0) two = -two;
    for(i = one.len - 1;i >= 0;i--) {
        int new = carry + (int)one.mantissa[i] * two;
        carry = new / 256;
        out.mantissa[i] = new;
    }
    while(carry != 0) {
        int new = carry + (int)one.mantissa[i] * two;
        carry = new / 256;
        out.len++;
        if(out.len > out.accu) out.len = out.accu;
        memmove(out.mantissa + 1, out.mantissa, out.len - 1);
        out.mantissa[0] = new;
        out.exp++;
    }
    trimZeroes(&out);
    return out;
}
Arb arb_divModInt(Arb one, unsigned char two, int* carryOut) {
    //Returs one/two and sets carryOut to the remainder.
    //Return value is an integer Arb
    //one is treated as an integer Arb
    int outlen = one.exp + 1;
    if(outlen > one.accu) outlen = one.accu;
    Arb out = arbCTR(calloc(outlen, 1), outlen, outlen - 1, one.sign ^ (two < 0 ? 1 : 0), one.accu);
    int carry = 0;
    for(int i = 0; i < outlen; i++) {
        int cur = carry * 256;
        cur += one.mantissa[i];
        out.mantissa[i] = cur / two;
        carry = cur % two;
    }
    trimZeroes(&out);
    // Repeated squaring modular arithmetic to find the carry out if exp >= accu
    if(one.exp >= one.accu) {
        //Number of powers
        int exp = one.exp - one.accu + 1;
        int i = 0;
        //Current power (256^(2^i))
        int curPow = 256 % two;
        //Calculate Cout as (256^exp)%base
        int Cout = 1;
        while(exp != 0) {
            //If ith bit is one
            if((exp & (1 << i)) != 0) {
                //Set ith bit to zero
                exp ^= 1 << i;
                //Multiply CarryOut by 256^(2^i)
                Cout *= curPow;
                Cout %= two;
                if(Cout == 0) break;
            }
            //Square current power
            curPow *= curPow;
            curPow %= two;
            if(curPow == 0) break;
            i++;
        }
        // carryOut = mod(carry*(256^exp),base)
        *carryOut = (carry * Cout) % two;
    }
    else *carryOut = carry;
    return out;
}
Arb arb_floor(Arb one) {
    //If one is greater than 0
    if(one.exp >= 0) {
        int outlen = one.len;
        if(outlen > one.exp + 1) outlen = one.exp + 1;
        if(outlen > one.accu) outlen = one.accu;
        Arb out = arbCTR(calloc(outlen + 1, 1), outlen, one.exp, one.sign, one.accu);
        memcpy(out.mantissa, one.mantissa, outlen);
        return out;
    }
    //else return 0
    else {
        return arbCTR(calloc(1, 1), 1, 0, 0, one.accu);
    }
}
Arb arb_add(Arb one, Arb two) {
    if(one.sign != two.sign) {
        Arb out;
        if(one.sign == 1) {
            one.sign = 0;
            out = arb_subtract(two, one);
            one.sign = 1;
        }
        if(one.sign == 0) {
            two.sign = 0;
            out = arb_subtract(one, two);
            two.sign = 1;
        }
        return out;
    }
    Arb out;
    out.accu = one.accu;
    out.exp = one.exp > two.exp ? one.exp : two.exp;
    out.sign = one.sign;
    int carry = 0, oneDiff = 0, twoDiff = 0;
    int oneLen = one.len, twoLen = two.len;
    if(one.exp > two.exp) {
        twoDiff = one.exp - two.exp;
        twoLen += twoDiff;
    }
    else if(one.exp < two.exp) {
        oneDiff = two.exp - one.exp;
        oneLen += oneDiff;
    }
    out.len = oneLen > twoLen ? oneLen : twoLen;
    if(out.len > out.accu) out.len = out.accu;
    unsigned char* outmant = calloc(out.len + 1, 1);
    one.mantissa -= oneDiff;
    two.mantissa -= twoDiff;
    int i;
    for(i = out.len - 1;i >= 0;i--) {
        int oneCell = one.mantissa[i];
        int twoCell = two.mantissa[i];
        if(i > twoLen || i < twoDiff) twoCell = 0;
        if(i > oneLen || i < oneDiff) oneCell = 0;
        int new = oneCell + twoCell + carry;
        outmant[i] = new;
        carry = new / 256;
    }
    one.mantissa += oneDiff;
    two.mantissa += twoDiff;
    if(carry != 0) {
        if(out.len == out.accu) out.len--;
        memmove(outmant + 1, outmant, out.len);
        out.len++;
        out.exp++;
        outmant[0] = carry;
    }
    out.mantissa = outmant;
    trimZeroes(&out);
    return out;
}
Arb arb_subtract(Arb one, Arb two) {
    if(one.sign != two.sign) {
        two.sign = one.sign;
        Arb out = arb_add(one, two);
        two.sign = one.sign == 0 ? 1 : 0;
        out.sign = one.sign;
        return out;
    }
    if(arbCmp(one, two) == -1) {
        Arb out = arb_subtract(two, one);
        out.sign ^= 1;
        return out;
    }
    Arb out;
    out.sign = one.sign;
    out.exp = one.exp;
    int twoDiff = one.exp - two.exp;
    out.accu = one.accu > two.accu ? one.accu : two.accu;
    out.len = two.len + twoDiff > one.len ? two.len + twoDiff : one.len;
    if(out.len > out.accu) out.len = out.accu;
    two.mantissa -= twoDiff;
    unsigned char* outMant = calloc(out.len, 1);
    int i, carry = 0;
    for(i = out.len - 1;i >= 0;i--) {
        int oneCell = one.mantissa[i];
        int twoCell = two.mantissa[i];
        if(i > one.len) oneCell = 0;
        if(i >= two.len + twoDiff || i < twoDiff) twoCell = 0;
        int new = oneCell - twoCell + carry;
        outMant[i] = new;
        carry = new < 0 ? -1 : 0;
    }
    two.mantissa += twoDiff;
    if(carry != 0) error("Fatal error in subtraction");
    out.mantissa = outMant;
    trimZeroes(&out);
    return out;
}
Arb arb_mult(Arb one, Arb two) {
    if(one.len > two.len) {
        Arb tmp = one;
        one = two;
        two = tmp;
    }
    //Set factors
    Arb out;
    out.accu = one.accu > two.accu ? one.accu : two.accu;
    out.len = one.len + two.len + 1;
    if(out.len > out.accu) out.len = out.accu + 1;
    long mantissaOut[out.len + 8];
    memset(mantissaOut, 0, (out.len + 8) * sizeof(long));
    out.exp = one.exp + two.exp;
    out.sign = one.sign ^ two.sign;
    int i, j;
    //Multiply
    for(i = 0;i < one.len;i++) {
        long* m;
        m = mantissaOut + i;
        long oneVal = one.mantissa[i];
        int count = out.len - i - 1;
        for(j = 0;j < count;j++) m[j] += oneVal * two.mantissa[j];
    }
    //Carry overflow
    i = out.len;
    while(--i >= 1) {
        if(mantissaOut[i] > 255) {
            mantissaOut[i - 1] += mantissaOut[i] / 256;
            mantissaOut[i] %= 256;
        }
    }
    while(mantissaOut[0] > 255) {
        if(out.len == out.accu) out.len--;
        memmove(mantissaOut + 1, mantissaOut, out.len);
        mantissaOut[0] = mantissaOut[1] / 256;
        mantissaOut[1] %= 256;
        out.len++;
        out.exp++;
    }
    //Copy cells
    unsigned char* mant = calloc(out.len, 1);
    //Memcpy cannot be used here because mantissaOut is a long*
    for(int i = 0;i < out.len;i++) mant[i] = mantissaOut[i];
    out.mantissa = mant;
    out.len--;
    trimZeroes(&out);
    //Return
    return out;
    //https://en.wikipedia.org/wiki/Karatsuba_algorithm if greater than 1000
    /*
        (az+b)*(cz+d)

        Calculate ac, bd, (a+b)(c+d)
        return ac*z^2 + ((a+b)*(c+d) - ac - bd)z + bd
        = a*c*z^2 + bcz + adz + bd

    */

}
Arb arb_recip(Arb one) {
    /*
        To find the reciprocal of one:
        Start with an initial approximation to find x[0]
        Then use this iteration sequence


        When the exponent 1-D*x[n] is less than accu, the result has been reached.

        x[INF] is exactly equal to the reciprocal

    */
    //Generate an approximation (about one digit of accuracy.
    char sign = one.sign;
    one.sign = 0;
    one.accu += 4;
    Arb approx = arbCTR(calloc(1, 1), 1, -one.exp - 1, 0, one.accu);
    approx.mantissa[0] = 255 / one.mantissa[0];
    Arb error;
    error.exp = one.exp;
    //x[n+1] = x[n] + x[n] * (1 - one*x[n])
    while(error.exp > one.exp - one.accu) {
        error = arb_mult(one, approx);
        //If error will equal zero
        if(error.len == 1 && error.exp == 0) {
            if(error.mantissa[0] == 1) {
                freeArb(error);
                break;
            }
        }
        //1-error
        if(error.exp == 0) {
            error.mantissa[0] -= 1;
            error.sign = 1;
        }
        else {
            if(error.exp < -1) arbRightShift(&error, -error.exp - 1);
            int i = 0;
            for(i = 0;i < error.len;i++) error.mantissa[i] = 255 - error.mantissa[i];
            error.mantissa[i - 1] += 1;
            error.sign = 0;
        }
        trimZeroes(&error);
        // Calculate approx + approx*error
        Arb tmp = arb_mult(approx, error);
        Arb tmp2 = arb_add(approx, tmp);
        freeArb(tmp);
        freeArb(approx);
        approx = tmp2;
        freeArb(error);
    }
    one.sign = sign;
    one.accu -= 4;
    approx.accu -= 4;
    approx.sign = sign;
    trimZeroes(&approx);
    return approx;
}
Arb arb_pi(int accu) {
    const unsigned char piStart[] = { 36,67,63,246,106,168,136,136,133,90,163,48,8,141,211,49,19,49,25,152,138,162,46,224,3,55,112,7,115,52,68,74,164,64,9,147,56,130,34,34,41,153,159,243,49,29 };
    Arb out;
    out.accu = accu;
    out.len = accu;
    out.exp = 0;
    out.mantissa = calloc(out.len, 1);
    out.sign = 0;
    out.mantissa[0] = 3;
    int i;
    int maxLen = out.len;
    if(maxLen > 46) maxLen = 46;
    for(i = 1;i < maxLen;i++) {
        out.mantissa[i] = piStart[i - 1];
    }
    if(maxLen != out.len) {
        //https://www.davidhbailey.com/dhbpapers/bbp-alg.pdf
    }
    return out;
}
#pragma endregion
#pragma endregion
#pragma region Numbers
Number newNum(double r, double i, unit_t u) {
    Number out;
    out.r = r;
    out.i = i;
    out.u = u;
    return out;
}
double parseNumber(const char* num, double base) {
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
    if(real == NULL) real = "NULL";
    char* imag = doubleToString(num.i, base);
    if(imag == NULL) imag = "NULL";
    char* unit = toStringUnit(num.u);
    int outLength = (num.u != 0 ? strlen(unit) + 2 : 0) + (num.i == 0 ? 0 : strlen(imag)) + strlen(real) + 3;
    char* out = calloc(outLength, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    memset(out, 0, outLength);
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
char* doubleToString(double num, double base) {
    if(num == 0 || base <= 1 || base > 36) {
        char* out = calloc(2, 1);
        if(out == NULL) error(mallocError);
        out[0] = '0';
        return out;
    }
    if(isnan(num)) {
        int pos = 0;
        char* out = malloc(5);
        if(out == NULL) { error(mallocError);return NULL; }
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
        if(out == NULL) { error(mallocError);return NULL; }
        if(num < 0) out[pos++] = '-';
        out[pos] = 'I';
        out[pos + 1] = 'n';
        out[pos + 2] = 'f';
        out[pos + 3] = '\0';
        return out;
    }
    //Allocate string
    char* out = calloc(24, 1);
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
char* appendToHistory(Value num, double base, bool print) {
    if(historySize - 1 == historyCount) {
        history = realloc(history, (historySize + 25) * sizeof(Value));
        if(history == NULL) { error(mallocError);return NULL; }
        historySize += 25;
    }
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
#pragma region Number Functions
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
    //the builtin atan2 is wrong for this edge case
    if(one.i == 0 && one.r < 0) arg = M_PI;
    if(one.r == 0 && one.i == 0) arg = 0;
    double p1 = exp(two.r * 0.5 * logabs - two.i * arg);
    double cis = two.i * 0.5 * logabs + two.r * arg;
    if(isnan(cis)) cis = 0;
    //If sine or cos of cis is very close to zero, set it to zero
    double sinecis = sin(cis);
    double coscis = cos(cis);
    if(sinecis<1e-15 && sinecis>-1e-15) sinecis = 0;
    if(coscis<1e-15 && coscis>-1e-15) coscis = 0;
    return newNum(p1 * coscis, p1 * sinecis, unitInteract(one.u, two.u, '^', two.r));
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
    double sinhi = sinh(one.i);
    double coshi = cosh(one.i);
    if(one.i == 0) sinhi = 0, coshi = 1;
    return newNum(sin(one.r) * cosh(one.i), cos(one.r) * sinh(one.i), one.u);
}
Number compSqrt(Number one) {
    double abs = sqrt(one.r * one.r + one.i * one.i);
    double sgnI = one.i / fabs(one.i);
    if(one.i == 0) sgnI = 1;
    if(one.r > 0 && one.i == 0) sgnI = 0;
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
    if(type < 21) {
        //Apply degree ratio
        num.r *= degrat;
        num.i *= degrat;
        if(type == op_sin)
            return compSine(num);
        if(type == op_cos)
            return newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), num.u);
        if(type == op_tan)
            return compDivide(compSine(num), newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), 0));
        if(type == op_sinh) {
            //sinh(x+y) = sinh(x)*cosh(y)+cosh(x)*sinh(y)
            //sinh(i*x) = i * sin(x)
            //sinh(a+bi) = sinh(a)*cos(b) + cosh(a) * i * sin(b)
            return newNum(sinh(num.r) * cos(num.i), cosh(num.r) * sin(num.i), num.u);
        }
        if(type == op_cosh) {
            //cosh(x+y) =cosh(x)*cosh(y)+sinh(x)*sinh(y)
            //cosh(x*i) = cos(x)
            //cosh(a+bi) = cosh(a)*cos(b)+sinh(a)*i*sin(b)
            return newNum(cosh(num.r) * cos(num.i), sinh(num.r) * sin(num.i), num.u);
        }
        if(type == op_tanh) {
            //tanh(x+y) = (tanh(x)+tanh(y))/(1+tanh(x)*tanh(y))
            //tanh(y*i) = i * tan(y)
            //tanh(a+bi) = (tanh(a)+i*tan(b))/(1+tanh(a)*i*tan(b))
            Number numer = newNum(tanh(num.r), tan(num.i), num.u);
            Number denom = newNum(1, tanh(num.r) * tan(num.i), 0);
            return compDivide(numer, denom);
        }
        Number out;
        bool reciprocal = false;
        // csc, sec, tan
        if(type == op_csc || type == op_sec || type == op_cot) reciprocal = true;
        if(type == op_csc)
            out = compSine(num);
        if(type == op_sec)
            out = newNum(cos(num.r) * cosh(num.i), sin(num.r) * sinh(num.i), num.u);
        if(type == op_cot)
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
        //acsc, asec, acot: invert and change opID
        if(type == op_acsc || type == op_asec || type == op_acot) {
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
        if(opID == op_atan) {
            out = compDivide(newNum(-num.r, 1 - num.i, 0), newNum(num.r, 1 + num.i, 0));
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
        //Apply degree ratio
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
#pragma endregion
#pragma region Vectors
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
#pragma region Value Constructors
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
        out.tree = malloc(sizeof(Tree));
        if(out.tree == NULL) error(mallocError);
        *out.tree = treeCopy(*val.tree, NULL, false, false, false);
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
#pragma endregion
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
        freeTree(*val.tree);
        free(val.tree);
    }
    if(val.type == value_arb) {
        if(val.numArb != NULL) {
            free(val.numArb->r.mantissa);
            free(val.numArb->i.mantissa);
        }
        free(val.numArb);
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
        outTree.branch = val.tree;
        outTree.argNames = val.argNames;
        outTree.argCount = argListLen(val.argNames);
        outTree.optype = optype_anon;
        outTree.op = 0;
        return treeToString(outTree, false, NULL);
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
    return NULL;
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
#pragma region Value Functions
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
    if(one.type == value_arb) {

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
        if(one.u == two.u) out.u = one.u;
        else if(one.u == 0) out.u = two.u;
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
#pragma endregion
#pragma region Trees
#pragma region Tree Comparison
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
#pragma endregion
#pragma region Tree Constructors
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
    if(out == NULL) { error(mallocError);return NULL; }
    if(copy)
        out[0] = treeCopy(one, NULL, 0, 0, 0);
    else
        out[0] = one;
    return out;
}
#pragma endregion
#pragma region Tree Management
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
Tree treeCopy(Tree tree, const Tree* args, bool unfold, int replaceArgs, bool optimize) {
    Tree out = tree;
    if(tree.optype == optype_anon) {
        out.argNames = argListCopy(tree.argNames);
        out.branch = malloc(sizeof(Tree));
        if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
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
    if(tree.argCount != 0) {
        out.branch = malloc(tree.argCount * sizeof(Tree));
        if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
    }
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
#pragma endregion
#pragma region Tree Parsing
char* inputClean(const char* input) {
    if(input[0] == '\0') {
        error("no equation", NULL);
        return NULL;
    }
    //Allocate out
    char* out = calloc(strlen(input) + 1, 1);
    if(out == NULL) { error(mallocError);return NULL; }
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
int getCharType(char in, int curType, int base, bool useUnits) {
    if((in >= '0' && in <= '9') || in == '.') return 0;
    if(in >= 'a' && in <= 'z') return 1;
    if(in == '_') return 1;
    if(useUnits && ((in >= 'A' && in <= 'Z') || in == '$') && (curType != 0 || (in > 'A' + (int)base - 10))) return 1;
    if((in >= '*' && in <= '/' && in != '.') || in == '%' || in == '^') return 6;
    return -1;
}
Tree generateTree(const char* eq, char** argNames, double base) {
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
        //Closed square brackets with underscore
        if(ch == ']' && eq[i + 1] == '_' && brackets == 1) {
            sectionTypes[sectionCount - 1] = 5;
            curType = getCharType(eq[i + 2], curType, base, useUnits);
            if(eq[i + 2] == '[' || eq[i + 2] == '(' || eq[i + 2] == '<') {
                brackets++;
                i++;
                int type = ch == '(' ? 3 : (ch == '[' ? 4 : 7);
                curType = type;
            }
            brackets--;
            i++;
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
        //Arrow notation
        if(ch == '=' && (curType == 1 || curType == 3) && eq[i + 1] == '>') {
            //Set current type to 8 and fill the section with the rest of the string
            sectionTypes[sectionCount - 1] = 8;
            i++;
            curType = 8;
            while(eq[++i] != '\0');
            break;
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
        int sectionLength = sections[i + 1] - sections[i];
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
            if(useArb) {
                Arb r = parseArb(section + (useBase ? 2 : 0), numBase, globalAccuracy);
                Value out;
                out.type = value_arb;
                out.numArb = calloc(1, sizeof(ArbNum));
                out.numArb->r = r;
                ops[i] = newOpValue(out);
            }
            else {
                double numr = parseNumber(section + (useBase ? 2 : 0), numBase);
                ops[i] = newOpVal(numr, 0, 0);
            }
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
            if(args == NULL) { error(mallocError);return NULLOPERATION; }
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
            if(sectionLength > 1) {
                if(section[1] == '-')
                    nextNegative = true;
                // ** as pow
                else if(first == '*' && section[1] == '*') {
                    if(section[2] == '-') nextNegative = true;
                    ops[i] = newOp(NULL, 0, op_pow, 0);
                    continue;
                }
                //Else invalid
                else
                    error("'%s' not a valid operator", section);
            }
            //If - is the first character and first section
            if(first == '-' && i == 0) {
                //This operation is multplied by the next value, it is a place holder
                ops[i] = newOpVal(1, 0, 0);
                nextNegative = true;
                continue;
            }
            //Standard operation IDs
            else if(first == '^') opID = op_pow;
            else if(first == '%') opID = op_mod;
            else if(first == '*') opID = op_mult;
            else if(first == '/') opID = op_div;
            else if(first == '+') opID = op_add;
            else if(first == '-') opID = op_sub;
            else error("'%s' is not a valid operator", section);
            // Insert operation with the correct id
            ops[i] = newOp(NULL, 0, opID, 0);
            continue;
        }
        else if(sectionTypes[i] == 8) {
            //Get arglist
            char** argList = parseArgumentList(section);
            char** appendedArgList = mergeArgList(argNames, argList);
            //Find '=' position
            int eqPos = 0;
            while(section[++eqPos] != '=');
            //Generate tree
            Tree tree = generateTree(section + eqPos + 2, appendedArgList, base);
            free(appendedArgList);
            //Return tree
            Tree out;
            out.branch = malloc(sizeof(Tree));
            if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
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
            ops[i] = generateTree(section + 1, argNames, base);
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
            if(args == NULL) { error(mallocError);return NULLOPERATION; }
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
        if(nextNegative && !(first > '*' && first < '/' && first != '.') && first != '^' && first != '%') {
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
    for(i = 0; i < 3; i++) {
        offset = 0;
        int j;
        for(j = 0; j < sectionCount + offset; j++) {
            ops[j] = ops[j + offset];
            //Configure order of operations
            int op = ops[j].op;
            if(i == 0) if(op != op_pow) continue;
            if(i == 1) if(op != op_mod && op != op_mult && op != op_div) continue;
            if(i == 2) if(op != op_add && op != op_sub) continue;
            //Confirm that current item is a builtin operation
            if(ops[j].optype != optype_builtin || ops[j].argCount != 0) continue;
            //Check for missing argument
            if(j == 0 || j == sectionCount - 1) {
                error("missing argument in operation", NULL);
                return NULLOPERATION;
            }
            //Combine previous and next, set offset
            ops[j] = newOp(allocArgs(ops[j - 1], ops[j + 1 + offset], 0, 0), 2, op, 0);
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
#pragma endregion
char* treeToString(Tree tree, bool bracket, char** argNames) {
    if(tree.optype == optype_anon) {
        char* argListString = argListToString(tree.argNames);
        char** newArgNames = mergeArgList(argNames, tree.argNames);
        char* treeString = treeToString(tree.branch[0], true, newArgNames);
        int argListLen = strlen(argListString);
        char* out = calloc(argListLen + 2 + strlen(treeString) + 1, 1);
        if(out == NULL) { error(mallocError);return NULL; }
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
            if(out == NULL) { error(mallocError);return NULL; }
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
            values[i] = treeToString(tree.branch[i], false, argNames);
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
        argText[i] = treeToString(tree.branch[i], false, argNames);
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
Value computeTree(Tree tree, const Value* args, int argLen) {
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
                    vec = args[tree.branch[0].op];
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
                    Value out;
                    out.type = value_num;
                    out.r = atan2(one.i, one.r);
                    //The builtin atan2 is wrong for this edge case
                    if(one.i == 0 && one.r < 0) out.r = M_PI;
                    out.i = 0;
                    out.u = one.u;
                    return out;
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
                if(inputs == NULL) { error(mallocError);return NULLVAL; }
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
        if(replaceArgs == NULL) { error(mallocError);return NULLVAL; }
        if(out.tree == NULL || replaceArgs == NULL) { error(mallocError);return NULLVAL; }
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
            if(out.branch == NULL) { error(mallocError);return NULLOPERATION; }
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
Value calculate(const char* eq, double base) {
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
#pragma region Argument Lists
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
    if(out == NULL) { error(mallocError);return NULL; }
    int i;
    for(i = 0;i < len;i++) {
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
#pragma endregion
#pragma region -include Functions
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
#pragma endregion
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
    //Compute tree
    Tree* tree = malloc(sizeof(Tree));
    if(argNames == NULL || tree == NULL) {
        free(name);
        return;
    }
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
        if(customfunctions == NULL) { error(mallocError);return; }
        functionArrayLength += 10;
    }
    customfunctions[numFunctions++] = newFunction(name, tree, argCount, argNames);
}
void deleteCustomFunction(int id) {
    //Free members
    customfunctions[id].nameLen = 0;
    freeTree(*customfunctions[id].tree);
    free(customfunctions[id].tree);
    free(customfunctions[id].name);
    //Free argNames
    int j = -1;
    char** argNames = customfunctions[id].argNames;
    while(argNames[++j] != NULL) free(argNames[j]);
    free(argNames);
    //Set tree to NULL
    customfunctions[id].tree = NULL;
    customfunctions[id].argCount = 0;
}
Tree findFunction(const char* name) {
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
    {1, 3, "sgn"}, {1, 3, "abs"}, {1, 3, "arg"}, {0, 0, " "},
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
    if(history == NULL || customfunctions == NULL) error(mallocError);
}
bool startsWith(char* string, char* sw) {
    int compareLength = strlen(sw);
    return memcmp(string, sw, compareLength) == 0 ? true : false;
}
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
                outLen += 8;
                outLen += customfunctions[i].nameLen;
                functionContents[i] = treeToString(*customfunctions[i].tree, false, customfunctions[i].argNames);
                outLen += strlen(functionContents[i]);
                functionInputs[i] = argListToString(customfunctions[i].argNames);
                outLen += strlen(functionInputs[i]);
            }
            char* out = calloc(outLen, 1);
            int outPos = 0;
            for(int i = 0;i < numFunctions;i++) {
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
            snprintf(out + outPos, outLen - outPos, "There are %d custom functions.", numFunctions);
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
        int nameLen = strlen(input + 5);
        for(int i = 0;i < numFunctions;i++) {
            if(customfunctions[i].nameLen != nameLen) continue;
            if(strcmp(input + 5, customfunctions[i].name) != 0) continue;
            char* out = calloc(nameLen + 33, 1);
            snprintf(out, nameLen + 33, "Function '%s' has been deleted.", customfunctions[i].name);
            deleteCustomFunction(i);
            return out;
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
        char* cleanInput = inputClean(input + 4);
        if(globalError) return NULL;
        //Set x
        char** x = calloc(2, sizeof(char*));
        if(x == NULL) { error(mallocError);return NULL; }
        x[0] = calloc(2, 1);
        if(x[0] == NULL) { error(mallocError);return NULL; }
        x[0][0] = 'x';
        //Get tree
        Tree ops = generateTree(cleanInput, x, 0);
        free(cleanInput);
        //Clean tree
        Tree cleanedOps = treeCopy(ops, NULL, true, false, true);
        freeTree(ops);
        //Get derivative and clean it
        Tree dx = derivative(cleanedOps);
        freeTree(cleanedOps);
        Tree dxClean = treeCopy(dx, NULL, false, false, true);
        freeTree(dx);
        //Print output
        Value ret;
        ret.type = value_func;
        ret.tree = malloc(sizeof(Tree));
        *ret.tree = dxClean;
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
        char* clean = inputClean(input + 7);
        Tree tree = generateTree(clean, NULL, 0);
        free(clean);
        if(globalError) return NULL;
        //Convert to string, free, and return
        char* out = treeToString(tree, false, NULL);
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
                    if(tempLen + outPos >= outSize - 50) {
                        outSize += 200;
                        out = realloc(out, outSize);
                        memset(out + outSize - 200, 0, 200);
                    }
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
                if(ratioLen + outPos > outSize - 10) {
                    outSize += 100;
                    out = realloc(out, outSize);
                    memset(out + outSize - 100, 0, 100);
                }
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
        char* name = input + 9;
        char* nameClean = inputClean(name);
        int includeID = -1;
        for(int i = 0;i < includeFuncsLen;i++) {
            if(strcmp(nameClean, includeFuncs[i].name) == 0) {
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
            error("cannot find function '%s'", nameClean);
            free(nameClean);
            return NULL;
        }
        free(nameClean);
        char* out = calloc(20, 1);
        strcpy(out, "Function included");
        return out;
    }
    else return NULL;
}
#pragma endregion
#pragma region Misc
#pragma region Factors
int* primeFactors(int num) {
    int* out = calloc(11, sizeof(int));
    if(out == NULL) { error(mallocError);return NULL; }
    int outSize = 10;
    int outPos = 0;
    int max = num / 2;
    //Remove factors of 2
    while(num % 2 == 0) {
        out[outPos++] = 2;
        num = num / 2;
        if(outPos == outSize) {
            out = realloc(out, (outSize + 11) * sizeof(int));
            if(out == NULL) { error(mallocError);return NULL; }
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
                if(out == NULL) { error(mallocError);return NULL; }
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
#pragma endregion
#pragma region Pretty Printing
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
char* printRatio(double in, bool forceSign) {
    char sign = in < 0 ? '-' : '+';
    //Get ratio
    int numer = 0;
    int denom = 0;
    getRatio(in, &numer, &denom);
    //Print number if no ratio exists
    if(numer == 0 && denom == 0) {
        char* string = doubleToString(sign == '-' ? -in : in, 10);
        char* out = calloc(strlen(string) + 3, 1);
        if(forceSign || sign == '-') out[0] = sign;
        if(forceSign) out[1] = ' ';
        strcpy(out + strlen(out), string);
        free(string);
        return out;
    }
    else {
        char* out = calloc(25, 1);
        if(sign == '-' || forceSign) out[0] = sign;
        if(forceSign) out[1] = ' ';
        int outlen = strlen(out);
        if(floor(fabs(in)) == 0) snprintf(out + outlen, 24 - outlen, "%d/%d", numer, denom);
        else snprintf(out + outlen, 24 - outlen, "%d%c%d/%d", (int)floor(fabs(in)), sign, numer, denom);
        return out;
    }
}
#pragma endregion
#pragma region Syntax Highlighting
int getVariableType(const char* name, bool useUnits, char** argNames, char** localVars) {
    //Remove spaces
    int len = strlen(name);
    char nameClear[len + 1];
    int i = -1;
    int j = 0;
    while(name[++i] != '\0') if(name[i] != ' ') {
        if(name[i] >= 'A' && name[i] <= 'Z') {
            nameClear[j++] = name[i] + 'a' - 'A';
        }
        else nameClear[j++] = name[i];
    }
    nameClear[j] = '\0';
    int nameLen = strlen(nameClear);
    i = 0;
    //Units
    if(useUnits) {
        char nameCase[len + 1];
        i = -1;
        j = 0;
        while(name[++i] != '\0') if(name[i] != ' ') nameCase[j++] = name[i];
        nameCase[j] = '\0';
        int prefix = -1;
        char first = nameCase[0];
        for(i = 0;i < metricCount;i++) if(first == metricNums[i]) {
            prefix = i;
            break;
        }
        if(prefix != -1) {
            const char* nameNoPrefix = nameCase + 1;
            for(i = 0;i < unitCount;i++) {
                const char* unit = unitList[i].name;
                if(strcmp(nameCase, unit) == 0) return 17;
                if(strcmp(nameNoPrefix, unit) == 0) return 17;
            }
        }
        else for(i = 0;i < unitCount;i++) {
            const char* unit = unitList[i].name;
            if(strcmp(nameCase, unit) == 0) return 17;
        }
    }
    //Argument Names
    if(argNames != NULL) {
        i = -1;
        while(argNames[++i] != NULL) {
            if(strcmp(argNames[i], nameClear) == 0) return 16;
        }
    }
    //Local Variables
    if(localVars != NULL) {
        i = -1;
        while(localVars[++i] != NULL) {
            if(strcmp(localVars[i], nameClear) == 0) return 18;
        }
    }
    //Custom functions
    for(i = 0;i < numFunctions;i++) {
        if(customfunctions[i].nameLen != nameLen) continue;
        if(strcmp(customfunctions[i].name, nameClear) == 0) return 15;
    }
    //Builtin functions
    //Possibly use a sorted list?
    for(i = 0;i < immutableFunctions;i++) {
        if(stdfunctions[i].nameLen != nameLen) continue;
        if(strcmp(stdfunctions[i].name, nameClear) == 0) return 14;
    }
    return 13;
}
const char* syntaxTypes[] = { "Null","Numeral","Variable","Comment","Error","Bracket","Operator","String","Command","Space","Escape","Null11","Invalid Operator","Invalid Variable","Builtin","Custom","Argument","Unit","Local Variable" };
char* highlightSyntax(const char* eq) {
    /*
        0 - Null (white)
        1 - Number
        2 - Variable
        3 - Comment (green)
        4 - Error character (red)
        5 - Bracket (white)
        6 - Operator (bright black)
        7 - String (yellow)
        8 - Command (blue)
        9 - Space (white)
        10 - Escape sequence (for strings)
        Advanced only
        11 -
        12 - Invalid operator
        13 - Invalid varible name
        14 - Builtin variable
        15 - Custom functions
        16 - Arguments
        17 - Unit
        18 - Local Variable
    */
    int eqLen = strlen(eq);
    char* out = calloc(eqLen, 1);
    bool isComment = false;
    int i = -1;
    if(eq[0] == '/' && eq[1] == '/') {
        out[0] = out[1] = 3;
        isComment = true;
        i += 2;
    }
    if(eq[0] == '#') {
        out[0] = 3;
        isComment = true;
        i += 1;
    }
    if(eq[0] == '.') {
        i++;
        out[0] = 8;
    }
    int bracket = 0;
    if(eq[0] == '-') {
        int j = 0;
        while(eq[j] != ' ' && eq[j] != '\0') {
            out[j] = 8;
            j++;
        }
        i = j - 1;
    }
    int state = 0;
    int base = 10;
    while(eq[++i] != '\0') {
        char ch = eq[i];
        if(ch == ' ' || ch == '\t') out[i] = 9;
        if(state == 0) base = 10;
        //Commented characters
        if(bracket == 0 && isComment) {
            out[i] = 3;
            if(eq[i] == '$' && eq[i + 1] == '(') {
                out[i] = 6;
                out[i + 1] = 5;
                i++;
                bracket++;
                continue;
            }
            continue;
        }
        if(ch == '(' || ch == '[' || ch == '<' || ch == ')' || ch == ']' || (ch == '>' && eq[i - 1] != '=')) {
            out[i] = 5;
            state = 0;
            if(ch == '(' || ch == '[' || ch == '<') bracket++;
            else {
                bracket--;
                // Error if no opening bracket
                if(bracket < 0) out[i] = 4;
                // ]_ base syntax
                if(ch == ']') {
                    if(eq[i + 1] == '_') {
                        out[i + 1] = 6;
                        i++;
                    }
                }
            }
            continue;
        }
        if(ch == ';') {
            //Set character as bracket
            state = 0;
            out[i] = 5;
            continue;
        }
        if(ch == ',') {
            state = 0;
            out[i] = 5;
            continue;
        }
        if(ch == '*' || ch == '/' || ch == '+' || ch == '-' || ch == '^' || ch == '%') {
            state = 0;
            out[i] = 6;
            continue;
        }
        if(ch == '=' && eq[i + 1] == '>') {
            out[i] = 6;
            out[i + 1] = 6;
            i++;
            state = 0;
            continue;
        }
        //Numbers
        if(((ch >= '0' && ch <= '9') || ch == '.') && state != 2) {
            if(ch == '0' && state != 1) {
                char b = eq[i + 1];
                base = 10;
                if(b == 'x') base = 16;
                if(b == 'd') base = 10;
                if(b == 'o') base = 8;
                if(b == 't') base = 3;
                if(b == 'b') base = 2;
                if(base != 10) {
                    out[i] = 1;
                    i++;
                }
            }
            out[i] = 1;
            state = 1;
            continue;
        }
        // ABCDEF for hexadecimal
        if(ch >= 'A' && state != 2 && (ch - 'A' < base - 10)) {
            out[i] = 1;
            state = 1;
            continue;
        }
        //Variables
        if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '$' || (state == 2 && ((ch >= '0' && ch <= '9') || ch == '.' || ch == '_'))) {
            out[i] = 2;
            state = 2;
            continue;
        }
        //Spaces
        if(ch == ' ' || ch == '\t') out[i] = 9;
        //Error character
        else {
            out[i] = 4;
            state = 0;
        }
    }
    if(bracket > 0) {
        bracket = 0;
        int i = eqLen - 1;
        //Find unmatched brackets
        for(i = eqLen - 1;i > -1;i--) {
            if(out[i] == 3) continue;
            char ch = eq[i];
            if(ch == '(' || ch == '<' || ch == '[') {
                bracket--;
                if(bracket < 0) {
                    out[i] = 4;
                    bracket = 0;
                }
            }
            if(ch == ')' || (ch == '>' && eq[i - 1] != '=') || ch == ']') {
                bracket++;
            }

        }
    }
    return out;
}
char* advancedHighlight(const char* eq, const char* syntax, bool forceUnits, char** arguments, char** localVariables) {
    int i = -1;
    int prev = 0;
    int prevPos = 0;
    int bracket = 0;
    bool useUnits = forceUnits;
    int unitBracketID = -1;
    int len = strlen(eq);
    char* out = calloc(len, 1);
    memcpy(out, syntax, len);
    char** oldArgs = arguments;
    char** freeArgs = NULL;
    if(eq[0] == '-') {
        char name[strlen(eq)];
        int j = 0;
        int nameI = 0;
        while(eq[++j] != ' ' && eq[j] != '\0') {
            name[nameI++] = eq[j];
        }
        name[nameI] = '\0';
        if(strcmp(name, "f") == 0) {

        }
        if(strcmp(name, "dx") == 0 || strcmp(name, "g") == 0) {
            char x[2] = "x";
            char* xArgs[2];
            xArgs[0] = x;
            xArgs[1] = NULL;
            arguments = mergeArgList(arguments, xArgs);
        }
        if(strcmp(name, "def") == 0) {
            int equal = 4;
            while(eq[++equal] != '=' && eq[equal] != '\0');
            if(eq[equal] == '=') {
                out[equal] = 8;
                //Find ( position
                int br = 4;
                while(eq[++br] != '(' && br < equal);
                if(br != equal) {
                    ignoreError = true;
                    char** args = parseArgumentList(eq + br);
                    ignoreError = false;
                    freeArgs = args;
                    arguments = mergeArgList(arguments, args);
                }
                memset(out + 5, 15, br - 5);
            }
        }
        if(strcmp(name, "degset") == 0) {
            char rad[4] = "rad";
            char deg[4] = "deg";
            char grad[5] = "grad";
            char* args[4];
            args[0] = rad;
            args[1] = deg;
            args[2] = grad;
            args[3] = NULL;
            arguments = mergeArgList(arguments, args);
        }
        if(strcmp(name, "include") == 0) {
            //Add included funcs names to arguments list
            char* names[includeFuncsLen + 1];
            names[includeFuncsLen] = NULL;
            for(int i = 0;i < includeFuncsLen;i++)
                names[i] = includeFuncs[i].name;
            arguments = mergeArgList(arguments, names);
        }
    }
    while(eq[i] != '\0' || i == -1) {
        i++;
        //Spaces
        if(syntax[i] == 9) continue;
        if(prev != syntax[i]) {
            //Operators
            if(prev == 6) {
                if(eq[prevPos] == '=' && eq[prevPos + 1] == '>') {
                    i = prevPos + 2;
                    //Find start of arguments
                    int j = prevPos - 1;
                    //Multiple variables
                    if(eq[prevPos - 1] == ')') {
                        while(j != 0 && eq[--j] != '(') {
                            if(syntax[j] != 2 && syntax[j] != 5) out[j] = 4;
                            if(syntax[j] == 13 || syntax[j] == 2) out[j] = 16;
                        }
                    }
                    //Single variable
                    else {
                        j++;
                        while(true) {
                            j--;
                            if(j == -1) break;
                            if(syntax[j] == 2 || (syntax[j] > 12 && syntax[j] < 19)) {
                                out[j] = 16;
                                continue;
                            }
                            break;
                        }
                        j++;
                    }
                    //Calculate argument list
                    ignoreError = true;
                    char** argList = parseArgumentList(eq + j);
                    char** newArguments = mergeArgList(arguments, argList);
                    //Find function bounding range
                    int startBracket = bracket;
                    int x = i;
                    for(x = i;eq[x] != '\0';x++) {
                        bool isCloseBracket = (eq[x] == ')' || eq[x] == ']' || (eq[x] == '>' && eq[x - 1] != '='));
                        if(bracket == startBracket && (eq[x] == ',' || isCloseBracket)) break;
                        if(isCloseBracket) bracket--;
                        if(eq[x] == '(' || eq[x] == '[' || eq[x] == '<') bracket++;
                    }
                    //Copy strings
                    char expression[x - i + 2];
                    memcpy(expression, eq + i, x - i);
                    expression[x - i] = '\0';
                    char syn[x - i + 2];
                    memcpy(syn, syntax + i, x - i);
                    syn[x - i] = '\0';
                    //Highlight anonymous function contents
                    char* highlighted = advancedHighlight(expression, syn, useUnits, newArguments, localVariables);
                    memcpy(out + i, highlighted, x - i);

                    //Cleanup
                    ignoreError = false;
                    freeArgList(argList);
                    free(newArguments);
                    i = x;
                    prev = 0;
                    prevPos = i;
                    continue;
                }
                else {
                    //This block determines if it is a valid operator
                    bool isValid = false;
                    char op[i - prevPos + 1];
                    int x = prevPos - 1, y = 0;
                    while(++x != i) if(eq[x] != ' ') op[y++] = eq[x];
                    op[y] = '\0';
                    int len = strlen(op);
                    //Possible negative character
                    if(op[len - 1] == '-' && len != 1) {
                        op[len - 1] = '\0';
                        len--;
                    }
                    if(len == 1) {
                        const char* ops = "+-/*^%";
                        char opchar = op[0];
                        if(opchar == '+') isValid = true;
                        if(opchar == '-') isValid = true;
                        if(opchar == '/') isValid = true;
                        if(opchar == '*') isValid = true;
                        if(opchar == '^') isValid = true;
                        if(opchar == '%') isValid = true;
                        if(opchar == '_' && eq[prevPos - 1] == ']') isValid = true;
                        // $() in comments
                        if(opchar == '$' && syntax[prevPos - 1] == 3) isValid = true;
                    }
                    // **
                    if(len == 2 && op[0] == '*' && op[1] == '*') isValid = true;
                    if(!isValid) memset(out + prevPos, 12, i - prevPos);
                }
            }
            //Variables
            if(prev == 2) {
                //Copy name
                char name[i - prevPos + 1];
                memcpy(name, eq + prevPos, i - prevPos);
                name[i - prevPos] = '\0';
                //Get name type
                int type = getVariableType(name, useUnits, arguments, localVariables);
                //Set type to out
                memset(out + prevPos, type, i - prevPos);
            }
            prev = out[i];
            prevPos = i;
        }
        if(syntax[i] == 3) continue;
        //Opening brackets
        if(eq[i] == '(' || eq[i] == '[' || eq[i] == '<') {
            if(eq[i] == '[' && unitBracketID == -1) {
                unitBracketID = bracket;
                useUnits = true;
            }
            bracket++;
        }
        //Closing brackets
        if(eq[i] == ']' || eq[i] == ')' || (eq[i] == '>' && eq[i - 1] != '=')) {
            bracket--;
            if(eq[i] == ']' && unitBracketID == bracket) {
                unitBracketID = -1;
                if(!forceUnits) useUnits = false;
            }
        }
    }
    if(arguments != oldArgs) free(arguments);
    if(freeArgs != NULL) freeArgList(freeArgs);
    return out;
}
#pragma endregion
#pragma endregion
