
#include "general.h"
#include <string.h>
#include <math.h>
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
        if(outPos == outSize - 1) out = recalloc(out, &outSize, 10, sizeof(int));
        if(globalError) return NULL;
    }
    int i;
    for(i = 3;i <= max;i += 2) {
        while(num % i == 0) {
            out[outPos++] = i;
            num = num / i;
            if(outPos == outSize - 1) out = recalloc(out, &outSize, 10, sizeof(int));
            if(globalError) return NULL;
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
Tree derivative(Tree tree) {
    if(tree.optype == optype_anon) {
        error("anonymous functions are not supported in dx");
    }
    if(tree.optype == optype_localvar) return NULLOPERATION;
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