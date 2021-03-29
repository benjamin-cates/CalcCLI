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
    if(one.type==value_num) {
        return one.r+one.i;
    }
    if(one.type==value_vec) {
        double total=0;
        for(int i=0;i<one.vec.total;i++) {
            total+=one.vec.val[i].r;
            total+=one.vec.val[i].i;
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
                }
                if(tree.op == op_width) return newValNum(width, 0, 0);
                if(tree.op == op_height) return newValNum(height, 0, 0);
                if(tree.op == op_length) return newValNum(width * height, 0, 0);
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
            }
        }
        Value one, two;
        if(tree.argCount > 0) {
            one = computeTree(tree.branch[0], args, argLen, localVars);
            if(one.type == value_func && tree.optype == optype_builtin) {
                if(tree.op != op_run && tree.op != op_sum && tree.op != op_product && tree.op != op_fill) {
                    error("functions cannot be passed to %s", stdfunctions[tree.op].name);
                    freeValue(one);
                    return NULLVAL;
                }
            }
        }
        if(tree.argCount > 1) {
            two = computeTree(tree.branch[1], args, argLen, localVars);
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
        if(tree.op < 63) {
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
            //Comparisons
            if(tree.op < 59) {
                int cmp = valCompare(one, two);
                freeValue(one);
                freeValue(two);
                //Equal
                if(cmp == 0 && (tree.op == op_equal || tree.op == op_lt_equal || tree.op == op_gt_equal)) return newValNum(1, 0, 0);
                //Not equal
                if(cmp != 0 && tree.op == op_not_equal) return newValNum(1, 0, 0);
                //Greater than
                if(cmp == 1 && (tree.op == op_gt || tree.op == op_gt_equal)) return newValNum(1, 0, 0);
                //Less than
                if(cmp == -1 && (tree.op == op_lt || tree.op == op_lt_equal)) return newValNum(1, 0, 0);
                //Else return 0
                return newValNum(0, 0, 0);
            }
            if(one.type != two.type) valueConvert(op_add, &one, &two);
            if(tree.op == op_min || tree.op == op_max) {
                if(one.type == value_num) {
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
                        if((oneNum.r > twoNum.r) ^ (tree.op == op_max)) out.vec.val[i + j * width] = twoNum;
                        else out.vec.val[i + j * width] = oneNum;
                    }
                    freeValue(one);
                    freeValue(two);
                    return out;
                }
            }
            if(tree.op == op_lerp) {
                //(1 - c) * one + c * two;
                Value c = computeTree(tree.branch[2], args, argLen, localVars);
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
                freeValue(one);
                freeValue(two);
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
            if(tree.op == op_not) {
                if(one.type == value_num) {
                    Value out;
                    out.type = value_num;
                    out.num = compBinNot(one.num);
                    return out;
                }
                if(one.type == value_vec) {
                    Value out;
                    out.type = value_vec;
                    out.vec = one.vec;
                    out.vec.val = malloc(one.vec.total * sizeof(Number));
                    for(int i = 0;i < one.vec.total;i++) out.vec.val[i] = compBinNot(one.vec.val[i]);
                    freeValue(one);
                    return out;
                }
                freeValue(one);
                return NULLVAL;
            }
            //Apply the binary operations properly with vectors
            const Number(*funcs[])(Number, Number) = { &compBinAnd,&compBinOr,&compBinXor,&compBinLs,&compBinRs };
            Value out = DivPowMod(funcs[tree.op - op_and], one, two, op_div);
            freeValue(one);
            freeValue(two);
            return out;
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
                    if(i < -historyCount) {
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
                if(tree.argCount > 0) freeValue(one);
                if(tree.argCount > 1) freeValue(two);
                return NULLVAL;
            }
            if(tree.op == op_run) {
                int argCount = tree.argCount - 1;
                int requiredArgs = argListLen(one.argNames);
                if(argCount < requiredArgs) {
                    error("not enough args in run function");
                    freeValue(one);
                    if(tree.argCount > 1) freeValue(two);
                    return NULLVAL;
                }
                Value inputs[tree.argCount];
                memset(inputs, 0, sizeof(Value) * tree.argCount);
                if(inputs == NULL) { error(mallocError);return NULLVAL; }
                inputs[0] = two;
                int i;
                for(i = 1;i < argCount;i++) {
                    inputs[i] = computeTree(tree.branch[i + 1], args, argLen, localVars);
                }
                Value out = runAnonymousFunction(one, inputs);
                for(i = 0;i < argCount;i++) freeValue(inputs[i]);
                freeValue(one);
                return out;
            }
            Value tempArgs[2];
            memset(tempArgs, 0, (argLen + 1) * sizeof(Number));
            if(args != NULL) memcpy(tempArgs, args, sizeof(Number) * argLen);
            Value loopArgValues[3];
            loopArgValues[0] = two;
            loopArgValues[1] = computeTree(tree.branch[2], args, argLen, localVars);
            loopArgValues[2] = computeTree(tree.branch[3], args, argLen, localVars);
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
                    Value current = runAnonymousFunction(one, tempArgs);
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
                    Value current = runAnonymousFunction(one, tempArgs);
                    Value new = valMult(out, current);
                    freeValue(out);
                    freeValue(current);
                    out = new;
                }
            }
            freeValue(one);
            return out;
        }
        //Matrix functions
        if(tree.op < 107) {
            if(tree.op == op_fill) {
                int width = getR(two);
                freeValue(two);
                int height = 1;
                if(tree.argCount > 2) {
                    Value three = computeTree(tree.branch[2], args, argLen, localVars);
                    height = getR(three);
                    freeValue(three);
                }
                if(width * height >= 0x7FFF || width * height < 0) {
                    if(width * height > 0) error("vector size too large");
                    else error("vector size cannot be negative");
                    freeValue(one);
                    return NULLVAL;
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
                        Value cell = runAnonymousFunction(one, funcArgs);
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
                    Value cell = runAnonymousFunction(two, funcArgs);
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