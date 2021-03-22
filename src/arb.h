#ifndef ARB_H
#define ARB_H 1
#include <stdbool.h>
typedef struct ArbStruct Arb;
//Global arbitrary precision accuracy
extern int globalAccuracy;
//Global use arbitrary precision
extern bool useArb;
//Global arbitrary precision accuracy in base 10
extern int digitAccuracy;
///General Functions
//Returns the number of digits required to reach accuracy in base
int getArbDigitCount(int base);
Arb arbCTR(unsigned char* mant, short len, short exp, char sign, short accu);
Arb copyArb(Arb arb);
void freeArb(Arb arb);
//Returns 1 if one>two, -1 if two>one, or 0 if they are equal
int arbCmp(Arb one, Arb two);
//Trim the zeroes and round up if len is greater than accu
void trimZeroes(Arb* arb);
//Arb conversions
Arb parseArb(char* string, int base, int accu);
double arbToDouble(Arb arb);
char* arbToString(Arb arb, int base, int digitAccuracy);
Arb doubleToArb(double val, int accu);
//Math functions
Arb arb_divModInt(Arb one, unsigned char two, int* carryOut);
Arb multByInt(Arb one, int two);
Arb arb_floor(Arb one);
Arb arb_add(Arb one, Arb two);
Arb arb_subtract(Arb one, Arb two);
Arb arb_mult(Arb one, Arb two);
//Calculate the reciprocal of one
Arb arb_recip(Arb one);
//Calculate pi to accu digits
Arb arb_pi(int accu);
//Calculate e to accu digits
Arb arb_e(int accu);
//Calculate the integer factorial of one
Arb arb_intFact(Arb one);
Arb arb_exp(Arb one);
Arb arb_ln(Arb one);
Arb arb_pow(Arb one, Arb two);
Arb arb_sinh(Arb one);
#endif