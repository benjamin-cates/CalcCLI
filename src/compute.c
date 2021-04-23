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
Number compLn(Number one) {
    Number out = one;
    out.r = 0.5 * log(one.r * one.r + one.i * one.i);
    out.i = atan2f(one.i, one.r);
    return out;
}
Number compExp(Number one) {
    Number out = one;
    double expr = exp(one.r);
    out.r = expr * cos(one.i);
    out.i = expr * sin(one.i);
    return out;
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
void applyUnaryToVector(Value* one, Number func(Number)) {
    if(one->type == value_num) {
        one->num = (*func)(one->num);
    }
    if(one->type == value_vec) {
        for(int i = 0;i < one->vec.total;i++)
            one->vec.val[i] = (*func)(one->vec.val[i]);
    }
}
Value applyBinaryToVector(Value one, Value two, Number func(Number, Number), bool useMax, bool forceVectorConversion) {
    int freeType = 0;
    //Convert both to vectors if forced
    if(one.type != two.type && forceVectorConversion) {
        if(one.type == value_num) {
            one.vec = newVecScalar(one.num);
            one.type = value_vec;
            freeType |= 1;
        }
        if(two.type == value_num) {
            two.vec = newVecScalar(two.num);
            two.type = value_vec;
            freeType |= 2;
        }
    }
    Value out = NULLVAL;
    //num*num
    if(one.type == value_num && two.type == value_num) {
        out.num = (*func)(one.num, two.num);
        return out;
    }
    out.type = value_vec;
    //vec*num
    if(one.type == value_vec && two.type == value_num) {
        out.vec = newVec(one.vec.width, one.vec.height);
        for(int i = 0;i < out.vec.total;i++)
            out.vec.val[i] = (*func)(one.vec.val[i], two.num);
        if(freeType & 1) freeValue(one);
        return out;
    }
    //num*vec
    if(one.type == value_num && two.type == value_vec) {
        out.vec = newVec(two.vec.width, two.vec.height);
        for(int i = 0;i < two.vec.total;i++)
            out.vec.val[i] = (*func)(one.num, two.vec.val[i]);
        if(freeType & 2) freeValue(two);
        return out;
    }
    //vec*vec
    int width = (one.vec.width < two.vec.width) ^ useMax ? one.vec.width : two.vec.width;
    int height = (one.vec.height < two.vec.height) ^ useMax ? one.vec.height : two.vec.height;
    int total = width * height;
    if(total >= 0xEFFF) {
        if(freeType & 1) freeValue(one);
        if(freeType & 2) freeValue(two);
        error("vector size overflow");
        return NULLVAL;
    }
    out.vec = newVec(width, height);
    for(int x = 0;x < width;x++) for(int y = 0;y < height;y++) {
        Number oneNum = NULLNUM;
        Number twoNum = NULLNUM;
        if(x < one.vec.width && y < one.vec.height) oneNum = one.vec.val[x + y * one.vec.width];
        if(x < two.vec.width && y < two.vec.height) twoNum = two.vec.val[x + y * two.vec.width];
        out.vec.val[x + y * width] = (*func)(oneNum, twoNum);
    }
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
Value valMult(Value one, Value two) {
    return applyBinaryToVector(one, two, &compMultiply, false, false);
}
Value valAdd(Value one, Value two) {
    if(one.type == value_string || two.type == value_string) {
        int freeType = 0;
        if(one.type != value_string) {
            one.string = valueToString(one, 10);
            one.type = value_string;
            freeType |= 1;
        }
        if(two.type != value_string) {
            two.string = valueToString(two, 10);
            two.type = value_string;
            freeType |= 2;
        }
        int len = strlen(one.string) + strlen(two.string) + 1;
        Value out;
        out.string = calloc(len, 1);
        out.type = value_string;
        strcpy(out.string, one.string);
        strcat(out.string, two.string);
        if(freeType & 1) freeValue(one);
        if(freeType & 2) freeValue(two);
        return out;
    }
    return applyBinaryToVector(one, two, &compAdd, true, true);
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
Value valDivide(Value one, Value two) {
    return applyBinaryToVector(one, two, &compDivide, false, false);
}
Value valPower(Value one, Value two) {
    return applyBinaryToVector(one, two, &compPower, true, false);
}
Value valModulo(Value one, Value two) {
    return applyBinaryToVector(one, two, &compModulo, false, false);
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
    if(one.type == value_string) {
        if(two.type == value_string) return strcmp(one.string, two.string);
        else return 2;
    }
    else if(two.type == value_string) return 2;
    double oneAbs = flattenVal(one);
    double twoAbs = flattenVal(two);
    if(oneAbs < twoAbs) return -1;
    if(oneAbs > twoAbs) return 1;
    if(valEqual(one, two)) return 0;
    return 2;
}
#pragma endregion
Value computeTreeMicro(Tree tree, const Value* arguments, int argLen, Value* localVars, int* isFree);
Value computeTree(Tree tree, const Value* args, int argLen, Value* localVars) {
    int isFree = 0;
    Value out = computeTreeMicro(tree, args, argLen, localVars, &isFree);
    if(!isFree) return copyValue(out);
    return out;
}
Value computeTreeMicro(Tree tree, const Value* arguments, int argLen, Value* localVars, int* isFree) {
    if(tree.optype == optype_builtin) {
        if(tree.op == op_val) return tree.value;
#define setOutToArgs(id) if(needsFree[id]){out=args[id];needsFree[id]=0;}else out=copyValue(args[id]);*isFree=1;args[id]=NULLVAL
        //Vector Functions
        Value out = NULLVAL;
        int needsFree[tree.argCount];
        memset(needsFree, 0, sizeof(needsFree));
        //Compute each branch
        Value args[tree.argCount];
        memset(args, 0, sizeof(args));
        const unsigned char* acceptableArgs = stdfunctions[tree.op].inputs;
        for(int i = 0;i < tree.argCount;i++) {
            const char* type[] = { "number","vector","anonymous function","arbitrary-precision number","string" };
            args[i] = computeTreeMicro(tree.branch[i], arguments, argLen, localVars, needsFree + i);
            if(globalError) goto ret;
            if(args[i].type<0 || args[i].type>value_string) {
                error("Invalid error value in call to %s", stdfunctions[tree.op].name);
                goto ret;
            }
            if((acceptableArgs[i] & (2 << args[i].type)) == 0) {
                if(tree.op == op_run && i != 0) continue;
                if(tree.op == op_vector) continue;
                error("Invalid %s in call to %s", type[args[i].type], stdfunctions[tree.op].name);
                goto ret;
            }
        }
        //Basic operators
        if(tree.op < 9) {
            if(tree.op == op_i) out = newValNum(0, 1, 0);
            else if(tree.op == op_neg) out = valNegate(args[0]);
            else if(tree.op == op_pow) out = valPower(args[0], args[1]);
            else if(tree.op == op_mod) out = valModulo(args[0], args[1]);
            else if(tree.op == op_div) out = valDivide(args[0], args[1]);
            else if(tree.op == op_mult) out = valMult(args[0], args[1]);
            else if(tree.op == op_add) out = valAdd(args[0], args[1]);
            else if(tree.op == op_sub) {
                Value negative = valNegate(args[1]);
                out = valAdd(args[0], negative);
                freeValue(negative);
            }
            *isFree = 1;
            goto ret;
        }
        //Trigonometric functions
        if(tree.op < 30) {
            setOutToArgs(0);
            if(out.type == value_num) {
                out.num = compTrig(tree.op, out.num);
            }
            else if(out.type == value_vec) {
                for(int i = 0;i < out.vec.total;i++)
                    out.vec.val[i] = compTrig(tree.op, out.vec.val[i]);
            }
            goto ret;
        }
        //Log, arg, and abs
        if(tree.op < 46) {
            if(tree.op == op_sqrt) {
                out = valPower(args[0], newValNum(1.0 / 2, 0, 0));
                *isFree = 1;
            }
            if(tree.op == op_cbrt) {
                out = valPower(args[0], newValNum(1.0 / 3, 0, 0));
                *isFree = 1;
            }
            if(tree.op == op_exp) {
                setOutToArgs(0);
                applyUnaryToVector(&out, &compExp);
            }
            if(tree.op == op_ln) {
                setOutToArgs(0);
                applyUnaryToVector(&out, &compLn);
            }
            if(tree.op == op_logten) {
                setOutToArgs(0);
                applyUnaryToVector(&out, &compLn);
                Value ln = out;
                out = valMult(ln, newValNum(1 / log(10), 0, 0));
                freeValue(ln);
                *isFree = 1;
            }
            if(tree.op == op_log) {
                setOutToArgs(0);
                Value LnArg = out;
                applyUnaryToVector(&LnArg, &compLn);
                setOutToArgs(1);
                Value LnBase = out;
                applyUnaryToVector(&LnBase, &compLn);
                out = valDivide(LnArg, LnBase);
                freeValue(LnArg);
                freeValue(LnBase);
                *isFree = 1;
            }
            if(tree.op == op_fact) {
                if(args[0].type == value_num) {
                    args[0].r += 1;
                    out.num = compGamma(args[0].num);
                }
                if(args[0].type == value_vec) {
                    setOutToArgs(0);
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
                *isFree = 1;
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
                    setOutToArgs(0);
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
                setOutToArgs(0);
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
                    setOutToArgs(0);
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
                    setOutToArgs(0);
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
                    setOutToArgs(0);
                    for(int i = 0;i < out.vec.total;i++) {
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
            if(args[0].type != args[1].type) {
                int frees = convertToSameType(needsFree[0] + 2 * needsFree[1], args, args + 1);
                needsFree[0] = frees & 1;
                needsFree[1] = frees & 2;
            }
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
                    *isFree = 1;
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
                out = valAdd(cTimesTwo, oneSubCTimesTwo);
                *isFree = 1;
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
                    setOutToArgs(0);
                    for(int i = 0;i < out.vec.total;i++) out.vec.val[i] = compBinNot(out.vec.val[i]);
                }
                goto ret;
            }
            //Apply the binary operations properly with vectors
            const Number(*funcs[])(Number, Number) = { &compBinAnd,&compBinOr,&compBinXor,&compBinLs,&compBinRs };
            const bool useMax[] = { false,true,true,true,true };
            int op = tree.op - op_and;
            out = applyBinaryToVector(args[0], args[1], funcs[op], useMax[op], false);
            *isFree = 1;
            goto ret;
        }
        //Constants
        if(tree.op < 88) {
            if(tree.op == op_pi) out = newValNum(3.1415926535897932, 0, 0);
            else if(tree.op == op_e) out = newValNum(2.718281828459045, 0, 0);
            else if(tree.op == op_phi) out = newValNum(1.618033988749894, 0, 0);
            else if(tree.op == op_typeof) out.r = args[0].type;
            else if(tree.op == op_ans) {
                if(historyCount == 0) {
                    error("no previous answer", NULL);
                    goto ret;
                }
                out = history[historyCount - 1];
            }
            else if(tree.op == op_hist) {
                int i = (int)floor(getR(args[0]));
                if(i < 0) {
                    if(i < -historyCount) {
                        error("history too short", NULL);
                        goto ret;
                    }
                    out = history[historyCount + i];
                    goto ret;
                }
                if(i >= historyCount) {
                    error("history too short", NULL);
                    goto ret;
                }
                out = history[i];
            }
            else if(tree.op == op_histnum) out = newValNum(historyCount, 0, 0);
            else if(tree.op == op_rand) out = newValNum((double)rand() / RAND_MAX, 0, 0);
            goto ret;
        }
        //Run, Sum, and Product
        if(tree.op < 93) {
            if(tree.op == op_run) {
                if(args[0].type == value_string) {
                    char name[strlen(args[0].string) + 1];
                    strcpy(name, args[0].string);
                    lowerCase(name);
                    Tree op = findFunction(name, false, NULL, NULL);
                    if(op.optype == optype_builtin) {
                        //TODO: this unecessarily recomputes args[1], args[2]...
                        Tree toCompute = NULLOPERATION;
                        toCompute.op = op.op;
                        toCompute.argCount = tree.argCount - 1;
                        toCompute.branch = tree.branch + 1;
                        out = computeTree(toCompute, arguments, argLen, localVars);
                    }
                    else if(op.optype == optype_custom) {
                        if(customfunctions[op.op].argCount > tree.argCount - 1) error("not enough args in run function");
                        else out = runFunction(customfunctions[op.op], args + 1);
                    }
                    else error("function '%s' not found", args[0].string);
                    *isFree = 1;
                    goto ret;
                }
                int argCount = tree.argCount - 1;
                int requiredArgs = argListLen(args[0].argNames);
                if(argCount < requiredArgs) {
                    error("not enough args in run function");
                    goto ret;
                }
                out = runAnonymousFunction(args[0], args + 1);
                *isFree = 1;
                goto ret;
            }
            int argCount = argListLen(args[0].argNames);
            if(argCount < 2) argCount = 2;
            Value tempArgs[argCount];
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
            *isFree = 1;
            goto ret;
        }
        //Matrix functions
        if(tree.op < 107) {
            if(tree.op == op_vector) {
                int width = tree.argWidth;
                int height = tree.argCount / tree.argWidth;
                int i;
                Vector vec = newVec(width, height);
                for(i = 0;i < vec.total;i++) vec.val[i] = getNum(args[i]);
                *isFree = 1;
                out.type = value_vec;
                out.vec = vec;
                goto ret;
            }
            else if(tree.op == op_width || tree.op == op_height || tree.op == op_length) {
                int ret = 1;
                if(args[0].type == value_vec) {
                    if(tree.op == op_width) ret = args[0].vec.width;
                    if(tree.op == op_height) ret = args[0].vec.height;
                    if(tree.op == op_length) ret = args[0].vec.total;
                }
                else if(args[0].type == value_string) {
                    if(tree.op == op_length || tree.op == op_width) ret = strlen(args[0].string);
                    if(tree.op == op_height) ret = 1;
                }
                out = newValNum(ret, 0, 0);
                goto ret;
            }
            else if(tree.op == op_ge) {
                int x = 0, y = 0;
                x = getR(args[1]);
                if(tree.argCount == 3) y = getR(args[2]);
                if(x < 0 || y < 0) return NULLVAL;
                if(args[0].type == value_num) {
                    if(x == 0 && y == 0) out = args[0];
                    else return newValNum(0, 0, 0);
                }
                else if(args[0].type == value_vec) {
                    int width = args[0].vec.width;
                    out.type = value_num;
                    if(x + y * width >= args[0].vec.total) {
                        error("ge out of bounds");
                        goto ret;
                    }
                    if(x >= width || y >= args[0].vec.height) out = NULLVAL;
                    else out.num = args[0].vec.val[x + y * width];
                }
                else if(args[0].type == value_string) {
                    char ch = 0;
                    if(y != 0);
                    else if(x<0 || x>strlen(args[0].string));
                    else ch = args[0].string[x];
                    out = newValNum(ch, 0, 0);
                }
                goto ret;
            }
            else if(tree.op == op_fill) {
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
                *isFree = 1;
                if(args[0].type == value_func) {
                    int argCount = argListLen(args[0].argNames);
                    if(argCount < 3) argCount = 3;
                    Value funcArgs[argCount];
                    memset(funcArgs, 0, sizeof(funcArgs));
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
                setOutToArgs(0);
                if(out.type == value_num) {
                    out = newValMatScalar(value_vec, out.num);
                    *isFree = 1;
                }
                bool isString = args[0].type == value_string;
                Value funcArgs[4];
                memset(funcArgs, 0, sizeof(funcArgs));
                int i, j;
                int length = 0, width = 0;
                if(isString) length = strlen(out.string), width = length;
                else length = out.vec.total, width = out.vec.width;
                for(int i = 0;i < length;i++) {
                    if(isString) funcArgs[0].r = out.string[i];
                    else funcArgs[0].num = out.vec.val[i];
                    funcArgs[1].r = i % width;
                    funcArgs[2].r = i / width;
                    funcArgs[3].r = i;
                    Value cell = runAnonymousFunction(args[1], funcArgs);
                    if(isString) out.string[i] = getR(cell);
                    else out.vec.val[i] = getNum(cell);
                    freeValue(cell);
                }
            }
            else if(tree.op == op_det) {
                if(args[0].type == value_num) {
                    out = args[0];
                }
                else if(args[0].vec.width != args[0].vec.height) {
                    error("Cannot calculate determinant of non-square matrix", NULL);
                    goto ret;
                }
                else out.num = determinant(args[0].vec);
            }
            else if(tree.op == op_transpose) {
                if(args[0].type == value_num) { args[0] = newValMatScalar(value_vec, args[0].num);needsFree[0] = 1; }
                out.type = value_vec;
                out.vec = transpose(args[0].vec);
                *isFree = 1;
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
                *isFree = 1;
            }
            else if(tree.op == op_mat_inv) {
                if(args[0].type == value_num) args[0] = newValMatScalar(value_vec, args[0].num);
                if(args[0].vec.width != args[0].vec.height) {
                    error("cannot take inverse of non-square matrix");
                    goto ret;
                }
                out.type = value_vec;
                out.vec = matInv(args[0].vec);
                *isFree = 1;
            }
            goto ret;
        }
        //String functions
        if(tree.op < 120) {
            if(tree.op == op_string) {
                if(args[0].type == value_string) {
                    setOutToArgs(0);
                }
                else {
                    //Get base
                    int base = 10;
                    if(tree.argCount > 1) base = getR(args[1]);
                    if(base < 2 || base > 36) {
                        error("base out of bounds");
                        goto ret;
                    }
                    //Convert to string
                    out.type = value_string;
                    out.string = valueToString(args[0], base);
                    *isFree = 1;
                }
            }
            else if(tree.op == op_eval) {
                int base = 0;
                if(tree.argCount > 1) base = getR(args[0]);
                out = calculate(args[0].string, base);
                *isFree = 1;
            }
            else if(tree.op == op_print) {
                if(args[0].type == value_string) printString(args[0]);
                else {
                    Value toPrint = NULLVAL;
                    toPrint.type = value_string;
                    toPrint.string = valueToString(args[0], 10);
                    printString(toPrint);
                    free(toPrint.string);
                }
            }
            else if(tree.op == op_error) {
                if(args[0].type == value_string) error("%s", args[0].string);
                else {
                    char* message = valueToString(args[0], 10);
                    error("%s", message);
                    free(message);
                }
                goto ret;
            }
            else if(tree.op == op_replace) {
                int stringLen = strlen(args[0].string);
                int searchLen = strlen(args[1].string);
                int replaceLen = strlen(args[2].string);
                int matches[stringLen + 1];
                int matchCount = 0;
                //Find matches
                for(int i = 0;i < stringLen - searchLen;i++)
                    if(memcmp(args[0].string + i, args[1].string, searchLen) == 0)
                        matches[++matchCount] = i;
                //Set maximum replace count
                if(tree.argCount > 3) if(matchCount > getR(args[3])) matchCount = getR(args[3]);
                matches[0] = -searchLen;
                matches[matchCount + 1] = stringLen;
                //Create return buffer
                out.string = calloc(stringLen + matchCount * (replaceLen - searchLen) + 1, 1);
                int outPos = 0;
                int readPos = 0;
                for(int i = 0;i <= matchCount;i++) {
                    //Copy unreplaced section
                    int len = matches[i + 1] - matches[i] - searchLen;
                    memcpy(out.string + outPos, args[0].string + readPos, len);
                    outPos += len;
                    readPos += len;
                    //Copy section to replace
                    if(i != matchCount) {
                        strcpy(out.string + outPos, args[2].string);
                        outPos += replaceLen;
                        readPos += searchLen;
                    }
                }
                out.type = value_string;
                *isFree = 1;
            }
            else if(tree.op == op_indexof) {
                int searchLen = strlen(args[0].string);
                int matchLen = strlen(args[1].string);
                //Find start (if argument is present)
                int start = 0;
                if(tree.argCount > 2) start = getR(args[2]);
                if(start < 0) start += searchLen;
                if(start < 0) { error("start index out of bounds");goto ret; }
                //Main loop
                out.r = -1;
                for(int i = start;i < searchLen - matchLen + 1;i++) {
                    if(memcmp(args[0].string + i, args[1].string, matchLen) == 0) { out.r = i;break; }
                }
            }
            else if(tree.op == op_substr) {
                int stringLen = strlen(args[0].string);
                //Get Range
                int start = getR(args[1]);
                int end = stringLen;
                if(tree.argCount > 2) end = getR(args[2]);
                //Negative indices
                if(start < 0) start += stringLen;
                if(end < 0) start += stringLen;
                //If indices out of range
                if(end < 0 || start < 0 || start >= stringLen || end > stringLen) {
                    error("substring range out of bounds");
                    goto ret;
                }
                //If start is before end
                if(end < start) {
                    int temp = start;
                    start = end;
                    end = temp;
                }
                //Copy string
                int len = end - start;
                if(needsFree[0]) {
                    out.string = args[0].string + start;
                    args[0].string[end] = 0;
                    needsFree[0] = 0;
                }
                else {
                    out.string = calloc(len + 1, 1);
                    memcpy(out.string, args[0].string + start, len);
                }
                out.type = value_string;
                *isFree = 1;
            }
            else if(tree.op == op_lowercase) {
                setOutToArgs(0);
                //Replace characters
                for(int i = 0;out.string[i] != 0;i++)
                    if(out.string[i] >= 'A' && out.string[i] <= 'Z') out.string[i] += 32;
            }
            else if(tree.op == op_uppercase) {
                setOutToArgs(0);
                //Replace characters
                for(int i = 0;out.string[i] != 0;i++)
                    if(out.string[i] >= 'a' && out.string[i] <= 'z') out.string[i] -= 32;
            }
            goto ret;
        }
    ret:
        for(int i = 0;i < tree.argCount;i++) if(needsFree[i]) freeValue(args[i]);
        return out;
    }
    if(tree.optype == optype_argument) {
        if(tree.op >= argLen) error("argument error", NULL);
        return arguments[tree.op];
    }
    if(tree.optype == optype_localvar) {
        if(localVars == NULL) {
            error("internal: No local variables passed to computeTree");
            return NULLVAL;
        }
        return localVars[tree.op];
    }
    if(tree.optype == optype_custom) {
        if(customfunctions[tree.op].code.list == NULL) {
            error("this uses a nonexistent function", NULL);
            return NULLVAL;
        }
        Value funcArgs[tree.argCount];
        int needsFree[tree.argCount];
        memset(needsFree, 0, sizeof(needsFree));
        int i;
        //Crunch args
        for(i = 0; i < tree.argCount; i++)
            funcArgs[i] = computeTreeMicro(tree.branch[i], arguments, argLen, localVars, needsFree + i);
        //Compute value
        Value out = runFunction(customfunctions[tree.op], funcArgs);
        //Free args
        for(i = 0;i < tree.argCount;i++) if(needsFree[i]) freeValue(funcArgs[i]);
        if(globalError)
            return NULLVAL;
        *isFree = 1;
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
            replaceArgs[i] = newOpValue(arguments[i]);
        }
        *out.code = copyCodeBlock(*tree.code, replaceArgs, tree.argWidth, false);
        free(replaceArgs);
        *isFree = 1;
        return out;
    }
    return NULLVAL;
}