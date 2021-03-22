//arb.c contains code about the arbitrary precision type
#include "general.h"
#include "arb.h"
#include <math.h>
#include <string.h>
int globalAccuracy=0;
bool useArb=false;
int digitAccuracy=0;
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
#pragma enderegion