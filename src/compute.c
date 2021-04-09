#include "compute.h"
#include "general.h"
#include "functions.h"
#include "parser.h"
#include <math.h>
#include <string.h>
#pragma region Numbers
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
            if(opID == op_acos)
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
Number compBinNot(Number one) {
    if(one.r != 0)
        one.r = (double)~((long)one.r);
    if(one.i != 0)
        one.i = (double)~((long)one.i);
    return one;
}
Number compBinAnd(Number one, Number two) {
    one.r = (double)(((long)one.r) & ((long)two.r));
    one.i = (double)(((long)one.i) & ((long)two.i));
    return one;
}
Number compBinOr(Number one, Number two) {
    one.r = (double)(((long)one.r) | ((long)two.r));
    one.i = (double)(((long)one.i) | ((long)two.i));
    return one;
}
Number compBinXor(Number one, Number two) {
    one.r = (double)(((long)one.r) ^ ((long)two.r));
    one.i = (double)(((long)one.i) ^ ((long)two.i));
    return one;
}
Number compBinLs(Number one, Number two) {
    one.r = (double)(((long)one.r) << ((long)two.r));
    one.i = (double)(((long)one.i) << ((long)two.i));
    return one;
}
Number compBinRs(Number one, Number two) {
    one.r = (double)(((long)one.r) >> ((long)two.r));
    one.i = (double)(((long)one.i) >> ((long)two.i));
    return one;
}
#pragma endregion
#pragma region Vectors
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
Value valMult(Value one, Value two) {
    int freeType = 0;
    if(one.type != two.type) freeType = valueConvert(op_mult, &one, &two);
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
            if(freeType & 1) freeValue(one);
            if(freeType & 2) freeValue(two);
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
            if(freeType & 1) freeValue(one);
            if(freeType & 2) freeValue(two);
            return out;
        }
    }
    if(one.type == value_arb) {

    }
    return NULLVAL;
}
Value valAdd(Value one, Value two) {
    int freeType = 0;
    if(one.type != two.type) freeType = valueConvert(op_add, &one, &two);
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
        if(freeType & 1) freeValue(one);
        if(freeType & 2) freeValue(two);
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
    int freeType = 0;
    if(one.type != two.type) freeType = valueConvert(op_div, &one, &two);
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
            if(freeType & 2) freeValue(two);
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
            if(freeType & 1) freeValue(one);
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
        if(freeType & 1) freeValue(one);
        if(freeType & 2) freeValue(two);
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
bool valEqual(Value one, Value two) {
    if(one.type != two.type) return false;
    if(one.type == value_num) {
        if(one.r != two.r) return false;
        if(one.i != two.i) return false;
        if(one.u != two.u) return false;
        return true;
    }
    if(one.type == value_vec) {
        if(one.vec.height != two.vec.height) return false;
        if(one.vec.total != two.vec.total) return false;
        for(int i = 0;i < one.vec.total;i++) {
            Number oneVal = one.vec.val[i];
            Number twoVal = two.vec.val[i];
            if(oneVal.r != twoVal.r) return false;
            if(oneVal.i != twoVal.i) return false;
            if(oneVal.u != twoVal.u) return false;
        }
        return true;
    }
    return false;
}
//Return the sum of all components. 3+4i will return 7, <10,4> will return 14
double flattenVal(Value one) {
    if(one.type == value_num) {
        return one.r + one.i;
    }
    if(one.type == value_vec) {
        double total = 0;
        for(int i = 0;i < one.vec.total;i++) {
            total += one.vec.val[i].r;
            total += one.vec.val[i].i;
        }
        return total;
    }
    return 0;
}
int valCompare(Value one, Value two) {
    double oneAbs = flattenVal(one);
    double twoAbs = flattenVal(two);
    if(oneAbs < twoAbs) return -1;
    if(oneAbs > twoAbs) return 1;
    if(valEqual(one, two)) return 0;
    return 2;
}
#pragma endregion
Value computeTree(Tree tree, const Value* args, int argLen, Value* localVars) {
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
                    Value cell = computeTree(tree.branch[i], args, argLen, localVars);
                    if(cell.type == value_num) vec.val[i] = cell.num;
                    else if(cell.type == value_vec) {
                        vec.val[i] = cell.vec.val[0];
                    }
                    else vec.val[i] = NULLNUM;
                    freeValue(cell);
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
                    vec = computeTree(tree.branch[0], args, argLen, localVars);
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
                    if(freeVec) freeValue(vec);
                }
                if(tree.op == op_width) return newValNum(width, 0, 0);
                if(tree.op == op_height) return newValNum(height, 0, 0);
                if(tree.op == op_length) return newValNum(width * height, 0, 0);
                if(freeVec) freeValue(vec);
            }
            if(tree.op == op_ge) {
                int x = 0, y = 0;
                if(tree.argCount == 3) {
                    Value yVal = computeTree(tree.branch[2], args, argLen, localVars);
                    y = getR(yVal);
                    freeValue(yVal);
                }
                Value xVal = computeTree(tree.branch[1], args, argLen, localVars);
                x = getR(xVal);
                freeValue(xVal);
                if(x < 0 || y < 0) return NULLVAL;
                Value vec;
                bool freeVec = false;
                if(tree.branch[0].optype == optype_builtin && tree.branch[0].op == op_vector) {
                    int width = tree.branch[0].argWidth;
                    if(tree.argCount == 3)
                        if(x >= width || y >= tree.branch[0].argCount / width) return NULLVAL;
                    return computeTree(tree.branch[0].branch[x + y * width], args, argLen, localVars);
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
                    Value index = computeTree(tree.branch[0].branch[0], args, argLen, localVars);
                    int i = getR(index);
                    freeValue(index);
                    if(i < 0) {
                        if(i < -historyCount) {
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
                    vec = computeTree(tree.branch[0], args, argLen, localVars);
                }
                if(vec.type == value_num) {
                    if(x == 0 && y == 0) return vec;
                    else return newValNum(0, 0, 0);
                }
                if(vec.type == value_vec) {
                    int width = vec.vec.width;
                    Value out;
                    out.type = value_num;
                    if(x + y * width >= vec.vec.total) {
                        error("ge out of bounds");
                        if(freeVec) freeValue(vec);
                        return NULLVAL;
                    }
                    out.num = vec.vec.val[x + y * width];
                    if(freeVec) freeValue(vec);
                    if(tree.argCount == 3)
                        if(x >= width || y >= vec.vec.height) return NULLVAL;
                    return out;
                }
                return NULLVAL;
                if(freeVec) freeValue(vec);
            }
        }
        //Prevent name conflict
        const Value* arguments = args;
        Value out = NULLVAL;
        //Compute each branch
        Value args[tree.argCount];
        memset(args, 0, sizeof(args));
        const unsigned char* acceptableArgs = stdfunctions[tree.op].inputs;
        for(int i = 0;i < tree.argCount;i++) {
            const char* type[] = { "number","vector","anonymous function","arbitrary-precision number" };
            args[i] = computeTree(tree.branch[i], arguments, argLen, localVars);
            if(globalError) goto ret;
            if((acceptableArgs[i] & (2 << args[i].type)) == 0) {
                if(tree.op == op_run && i != 0) continue;
                error("Invalid %s in call to %s", type[args[i].type], stdfunctions[tree.op].name);
                goto ret;
            }
        }
        //Basic operators
        if(tree.op < 9) {
            if(tree.op == op_i) out = newValNum(0, 1, 0);
            else if(tree.op == op_neg) out = valNegate(args[0]);
            else if(tree.op == op_pow) out = DivPowMod(&compPower, args[0], args[1], 0);
            else if(tree.op == op_mod) out = DivPowMod(&compModulo, args[0], args[1], 1);
            else if(tree.op == op_div) out = DivPowMod(&compDivide, args[0], args[1], 1);
            else if(tree.op == op_mult) out = valMult(args[0], args[1]);
            else if(tree.op == op_add) out = valAdd(args[0], args[1]);
            else if(tree.op == op_sub) {
                Value negative = valNegate(args[1]);
                out = valAdd(args[0], negative);
                freeValue(negative);
            }
            goto ret;
        }
        //Trigonometric functions
        if(tree.op < 30) {
            if(args[0].type == value_num) {
                out.type = args[0].type;
                out.num = compTrig(tree.op, args[0].num);
            }
            else if(args[0].type == value_vec) {
                out = args[0];
                args[0] = NULLVAL;
                for(int i = 0;i < args[0].vec.total;i++) {
                    out.vec.val[i] = compTrig(tree.op, out.vec.val[i]);
                }
            }
            goto ret;
        }
        //Log, arg, and abs
        if(tree.op < 46) {
            if(tree.op == op_sqrt) out = valPower(args[0], newValNum(1.0 / 2, 0, 0));
            if(tree.op == op_cbrt) out = valPower(args[0], newValNum(1.0 / 3, 0, 0));
            if(tree.op == op_exp) {
                if(args[0].type == value_num) {
                    double expr = exp(args[0].r);
                    out.r = expr * cos(args[0].i);
                    out.i = expr * sin(args[0].i);
                }
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < args[0].vec.total;i++) {
                        double expr = exp(out.vec.val[i].r);
                        out.vec.val[i].r = expr * cos(out.vec.val[i].i);
                        out.vec.val[i].i = expr * sin(out.vec.val[i].i);
                    }
                }
            }
            if(tree.op == op_ln) out = valLn(args[0]);
            if(tree.op == op_logten) out = valMult(valLn(args[0]), newValNum(1 / log(10), 0, 0));
            if(tree.op == op_log) {
                Value LnArg = valLn(args[0]);
                Value LnBase = valLn(args[1]);
                out = valDivide(LnArg, LnBase);
                freeValue(LnArg);
                freeValue(LnBase);
            }
            if(tree.op == op_fact) {
                if(args[0].type == value_num) {
                    args[0].r += 1;
                    out.num = compGamma(args[0].num);
                }
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].r += 1;
                        out.vec.val[i] = compGamma(out.vec.val[i]);
                    }
                }
            }
            if(tree.op == op_sgn) {
                Value abs = valAbs(args[0]);
                out = valDivide(args[0], abs);
                freeValue(abs);
            }
            if(tree.op == op_abs) out = valAbs(args[0]);
            if(tree.op == op_arg) {
                if(args[0].type == value_num) {
                    out.r = atan2(args[0].i, args[0].r);
                    //The builtin atan2 is wrong for this edge case
                    if(args[0].i == 0 && args[0].r < 0) out.r = M_PI;
                    out.i = 0;
                    out.u = args[0].u;
                }
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < out.vec.total;i++) {
                        Number num = out.vec.val[i];
                        out.vec.val[i].r = atan2(num.i, num.r);
                        //The builtin atan2 is wrong for this edge case
                        if(num.i == 0 && num.r < 0) out.vec.val[i].r = M_PI;
                        out.vec.val[i].i = 0;
                        out.vec.val[i].u = num.u;
                    }
                }
            }
            goto ret;
        }
        //Rounding and conditionals
        if(tree.op < 63) {
            if(tree.op >= op_round && tree.op <= op_ceil) {
                double (*roundType)(double);
                if(tree.op == op_round) roundType = &round;
                if(tree.op == op_floor) roundType = &floor;
                if(tree.op == op_ceil) roundType = &ceil;
                out = args[0];
                args[0] = NULLVAL;
                if(out.type == value_num) {
                    out.r = (*roundType)(out.r);
                    out.i = (*roundType)(out.i);
                }
                if(out.type == value_vec) {
                    for(int i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].r = (*roundType)(out.vec.val[i].r);
                        out.vec.val[i].i = (*roundType)(out.vec.val[i].i);
                    }
                }
                goto ret;
            }
            if(tree.op == op_getr) {
                if(args[0].type == value_num) out.r = args[0].r;
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].i = 0;
                        out.vec.val[i].u = 0;
                    }
                }
                goto ret;
            }
            if(tree.op == op_geti) {
                if(args[0].type == value_num) out.r = args[0].i;
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].r = out.vec.val[i].i;
                        out.vec.val[i].i = 0;
                        out.vec.val[i].u = 0;
                    }
                }
                goto ret;
            }
            if(tree.op == op_getu) {
                if(args[0].type == value_num) {
                    out.r = 1;
                    out.u = args[0].u;
                }
                if(args[0].type == value_vec) {
                    int i;
                    out = args[0];
                    args[0] = NULLVAL;
                    for(i = 0;i < out.vec.total;i++) {
                        out.vec.val[i].r = 1;
                        out.vec.val[i].i = 0;
                    }
                }
                goto ret;
            }
            //Comparisons
            if(tree.op < 59) {
                int cmp = valCompare(args[0], args[1]);
                //Equal
                if(cmp == 0 && (tree.op == op_equal || tree.op == op_lt_equal || tree.op == op_gt_equal)) out.r = 1;
                //Not equal
                if(cmp != 0 && tree.op == op_not_equal) out.r = 1;
                //Greater than
                if(cmp == 1 && (tree.op == op_gt || tree.op == op_gt_equal)) out.r = 1;
                //Less than
                if(cmp == -1 && (tree.op == op_lt || tree.op == op_lt_equal)) out.r = 1;
                goto ret;
            }
            if(args[0].type != args[1].type) valueConvert(op_add, args, args + 1);
            if(tree.op == op_min || tree.op == op_max) {
                if(args[0].type == value_num) {
                    bool isGreater = args[0].r > args[1].r;
                    bool expectsGreater = tree.op == op_max;
                    if(isGreater ^ expectsGreater) out = args[1];
                    else out = args[0];
                }
                if(args[0].type == value_vec) {
                    Vector one = args[0].vec;
                    Vector two = args[1].vec;
                    int width = one.width > two.width ? one.width : two.width;
                    int height = one.height > two.height ? one.height : two.height;
                    out.type = value_vec;
                    out.vec = newVec(width, height);
                    for(int i = 0;i < width;i++) for(int j = 0;j < height;j++) {
                        Number oneNum = NULLNUM;
                        Number twoNum = NULLNUM;
                        if(i < one.width && j < one.height) oneNum = one.val[i + j * one.width];
                        if(i < two.width && j < two.height) twoNum = two.val[i + j * two.width];
                        if((oneNum.r > twoNum.r) ^ (tree.op == op_max)) out.vec.val[i + j * width] = twoNum;
                        else out.vec.val[i + j * width] = oneNum;
                    }
                }
            }
            if(tree.op == op_lerp) {
                //(1 - c) * one + c * two;
                Value c = computeTree(tree.branch[2], args, argLen, localVars);
                Value negativeC = valNegate(c);
                Value oneSubC = valAdd(newValNum(1, 0, 0), negativeC);
                Value cTimesTwo = valMult(c, args[1]);
                Value oneSubCTimesTwo = valMult(oneSubC, args[0]);
                Value out = valAdd(cTimesTwo, oneSubCTimesTwo);
                freeValue(c);
                freeValue(negativeC);
                freeValue(oneSubC);
                freeValue(cTimesTwo);
                freeValue(oneSubCTimesTwo);
            }
            if(tree.op == op_dist) {
                if(args[0].type == value_num) out = newValNum(sqrt(pow(fabs(args[0].r - args[1].r), 2) + pow(fabs(args[0].i - args[1].i), 2)), 0, 0);
                if(args[0].type == value_vec) {
                    int i;
                    Vector one = args[0].vec;
                    Vector two = args[1].vec;
                    int length = one.total > two.total ? one.total : two.total;
                    for(i = 0;i < length;i++) {
                        Number oneNum, twoNum;
                        if(i < one.total) oneNum = one.val[i];
                        else oneNum = NULLNUM;
                        if(i < two.total) twoNum = two.val[i];
                        else twoNum = NULLNUM;
                        out.r += pow(oneNum.r - twoNum.r, 2) + pow(oneNum.i - twoNum.i, 2);
                    }
                }
            }
            goto ret;
        }
        //Binary Operations
        if(tree.op < 72) {
            if(tree.op == op_not) {
                if(args[0].type == value_num) out.num = compBinNot(args[0].num);
                if(args[0].type == value_vec) {
                    out = args[0];
                    args[0] = NULLVAL;
                    for(int i = 0;i < out.vec.total;i++) out.vec.val[i] = compBinNot(out.vec.val[i]);
                }
                goto ret;
            }
            //Apply the binary operations properly with vectors
            const Number(*funcs[])(Number, Number) = { &compBinAnd,&compBinOr,&compBinXor,&compBinLs,&compBinRs };
            out = DivPowMod(funcs[tree.op - op_and], args[0], args[1], op_div);
            goto ret;
        }
        //Constants
        if(tree.op < 88) {
            if(tree.op == op_pi) out = newValNum(3.1415926535897932, 0, 0);
            else if(tree.op == op_e) out = newValNum(2.718281828459045, 0, 0);
            else if(tree.op == op_phi) out = newValNum(1.618033988749894, 0, 0);
            else if(tree.op == op_ans) {
                if(historyCount == 0) {
                    error("no previous answer", NULL);
                    goto ret;
                }
                out = copyValue(history[historyCount - 1]);
            }
            else if(tree.op == op_hist) {
                int i = (int)floor(getR(args[0]));
                if(i < 0) {
                    if(i < -historyCount) {
                        error("history too short", NULL);
                        goto ret;
                    }
                    out = copyValue(history[historyCount - i]);
                }
                if(i >= historyCount) {
                    error("history too short", NULL);
                    goto ret;
                }
                out = copyValue(history[i]);
            }
            else if(tree.op == op_histnum) out = newValNum(historyCount, 0, 0);
            else if(tree.op == op_rand) out = newValNum((double)rand() / RAND_MAX, 0, 0);
            goto ret;
        }
        //Run, Sum, and Product
        if(tree.op < 93) {
            if(tree.op == op_run) {
                int argCount = tree.argCount - 1;
                int requiredArgs = argListLen(args[0].argNames);
                if(argCount < requiredArgs) {
                    error("not enough args in run function");
                    goto ret;
                }
                out = runAnonymousFunction(args[0], args + 1);
                goto ret;
            }
            Value tempArgs[2];
            memset(tempArgs, 0, sizeof(tempArgs));
            double loopArgs[3];
            loopArgs[0] = getR(args[1]);
            loopArgs[1] = getR(args[2]);
            loopArgs[2] = getR(args[3]);
            double i;
            int loopCount = 0;
            if(tree.op == op_sum) {
                out = newValNum(0, 0, 0);
                tempArgs[0] = newValNum(0, 0, 0);
                for(i = loopArgs[0];i <= loopArgs[1];i += loopArgs[2]) {
                    tempArgs[0].r = i;
                    tempArgs[1].r = loopCount;
                    Value current = runAnonymousFunction(args[0], tempArgs);
                    Value new = valAdd(out, current);
                    freeValue(out);
                    freeValue(current);
                    if(globalError) { freeValue(new);goto ret; }
                    out = new;
                    loopCount++;
                    if(loopCount > 100000) { error("infinite loop detected");goto ret; }
                }
            }
            if(tree.op == op_product) {
                out = newValNum(1, 0, 0);
                tempArgs[0] = newValNum(0, 0, 0);
                for(i = loopArgs[0]; i <= loopArgs[1];i += loopArgs[2]) {
                    tempArgs[0].r = i;
                    tempArgs[1].r = loopCount;
                    Value current = runAnonymousFunction(args[0], tempArgs);
                    Value new = valMult(out, current);
                    freeValue(out);
                    freeValue(current);
                    if(globalError) { freeValue(new);goto ret; }
                    out = new;
                    loopCount++;
                    if(loopCount > 100000) { error("infinite loop detected");goto ret; }
                }
            }
            goto ret;
        }
        //Matrix functions
        if(tree.op < 107) {
            if(tree.op == op_fill) {
                int width = getR(args[1]);
                int height = 1;
                if(tree.argCount > 2) height = getR(args[2]);
                if(width * height >= 0x7FFF || width * height < 1) {
                    if(width * height > 0) error("vector size too large");
                    else error("vector size cannot be negative or zero");
                    goto ret;
                }
                out;
                out.type = value_vec;
                out.vec = newVec(width, height);
                if(args[0].type == value_func) {
                    Value funcArgs[3];
                    funcArgs[0] = NULLVAL;
                    funcArgs[1] = NULLVAL;
                    funcArgs[2] = NULLVAL;
                    for(int j = 0;j < height;j++) for(int i = 0;i < width;i++) {
                        funcArgs[0].r = i;
                        funcArgs[1].r = j;
                        funcArgs[2].r = i + j * width;
                        Value cell = runAnonymousFunction(args[0], funcArgs);
                        out.vec.val[i + j * width] = getNum(cell);
                        freeValue(cell);
                        if(globalError) goto ret;
                    }
                }
                if(args[0].type == value_vec || args[0].type == value_num) {
                    Number num = getNum(args[0]);
                    for(int j = 0;j < height;j++) for(int i = 0;i < width;i++) {
                        out.vec.val[i + j * width] = num;
                    }
                }
            }
            else if(tree.op == op_map) {
                if(args[0].type == value_num) {
                    args[0] = newValMatScalar(value_vec, args[0].num);
                }
                out.type = value_vec;
                out.vec = newVec(args[0].vec.width, args[0].vec.height);
                Value funcArgs[5];
                funcArgs[0] = NULLVAL;
                funcArgs[1] = NULLVAL;
                funcArgs[2] = NULLVAL;
                funcArgs[3] = NULLVAL;
                funcArgs[4] = args[0];
                int i, j;
                for(int i = 0;i < args[0].vec.total;i++) {
                    funcArgs[0].num = args[0].vec.val[i];
                    funcArgs[1].r = i % args[0].vec.width;
                    funcArgs[2].r = i / args[0].vec.width;
                    funcArgs[3].r = i;
                    Value cell = runAnonymousFunction(args[1], funcArgs);
                    out.vec.val[i] = getNum(cell);
                    freeValue(cell);
                }
            }
            else if(tree.op == op_det) {
                if(args[0].type == value_num) {
                    args[0] = newValMatScalar(value_vec, args[0].num);
                }
                if(args[0].vec.width != args[0].vec.height) {
                    error("Cannot calculate determinant of non-square matrix", NULL);
                    goto ret;
                }
                out.num = determinant(args[0].vec);
            }
            else if(tree.op == op_transpose) {
                if(args[0].type == value_num) args[0] = newValMatScalar(value_vec, args[0].num);
                out.type = value_vec;
                out.vec = transpose(args[0].vec);
            }
            else if(tree.op == op_mat_mult) {
                if(args[0].type == value_num) args[0] = newValMatScalar(value_vec, args[0].num);
                if(args[1].type == value_num) args[1] = newValMatScalar(value_vec, args[1].num);
                if(args[0].vec.width != args[1].vec.height) {
                    error("matrix size error in mat_mult", NULL);
                    goto ret;
                }
                out.type = value_vec;
                out.vec = matMult(args[0].vec, args[1].vec);
            }
            else if(tree.op == op_mat_inv) {
                if(args[0].type == value_num) args[0] = newValMatScalar(value_vec, args[0].num);
                if(args[0].vec.width != args[0].vec.height) {
                    error("cannot take inverse of non-square matrix");
                    goto ret;
                }
                out.type = value_vec;
                out.vec = matInv(args[0].vec);
            }
            goto ret;
        }
    ret:
        for(int i = 0;i < tree.argCount;i++) freeValue(args[i]);
        return out;
    }
    if(tree.optype == optype_argument) {
        if(tree.op >= argLen) error("argument error", NULL);
        return copyValue(args[tree.op]);
    }
    if(tree.optype == optype_localvar) {
        if(localVars == NULL) {
            error("internal: No local variables passed to computeTree");
            return NULLVAL;
        }
        return copyValue(localVars[tree.op]);
    }
    if(tree.optype == optype_custom) {
        if(customfunctions[tree.op].code.list == NULL) {
            error("this uses a nonexistent function", NULL);
            return NULLVAL;
        }
        Value funcArgs[tree.argCount];
        int i;
        //Crunch args
        for(i = 0; i < tree.argCount; i++)
            funcArgs[i] = computeTree(tree.branch[i], args, argLen, localVars);
        //Compute value
        Value out = runFunction(customfunctions[tree.op], funcArgs);
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
        out.code = malloc(sizeof(CodeBlock));
        Tree* replaceArgs = calloc(argLen, sizeof(Tree));
        if(replaceArgs == NULL) { error(mallocError);return NULLVAL; }
        if(out.code == NULL || replaceArgs == NULL) { error(mallocError);return NULLVAL; }
        int i;
        for(i = 0;i < argLen;i++) {
            replaceArgs[i] = newOpValue(args[i]);
        }
        *out.code = copyCodeBlock(*tree.code, replaceArgs, tree.argWidth, false);
        free(replaceArgs);
        return out;
    }
    return NULLVAL;
}