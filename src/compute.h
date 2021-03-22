//compute.h contains header information for compute.c
#ifndef COMPUTE_H
#define COMPUTE_H 1
typedef struct NumberStruct Number;
typedef struct VectorStruct Vector;
typedef struct TreeStruct Tree;
typedef struct ValueStruct Value;
#define unit_t unsigned long long
#pragma region Numbers
/**
 * Returns a number made of the three components
 * @param r Real component
 * @param i Imaginary component
 * @param u Unit component
 */
Number newNum(double r, double i, unit_t u);
//Returns one+two
Number compAdd(Number one, Number two);
//Returns one-two
Number compSubtract(Number one, Number two);
//Returns one*two
Number compMultiply(Number one, Number two);
//Returns pow(one,two) or one^two
Number compPower(Number one, Number two);
//Returns one/two
Number compDivide(Number one, Number two);
//Returns sin(one)
Number compSine(Number one);
//Returns one-two
Number compSubtract(Number one, Number two);
//Returns sqrt(one)
Number compSqrt(Number one);
/**
 * Returns gamma function of one
 * The gamma function is equal to factorial(one-1)
 * @param one number to pass to the gamma function
 */
Number compGamma(Number one);
/**
 * Returns the trigonometric result of num, based on type
 * @param type Type of operation to run (ex. op_sin)
 * @param num The input into the function
 */
Number compTrig(int type, Number num);
/**
 * Returns the binary operation of one and two, based on type
 * @param type Type of operation to run (ex. op_and)
 * @param one First input
 * @param two Second input
 */
Number compBinOp(int type, Number one, Number two);
#pragma endregion
#pragma region Vectors
/**
 * Return the determinant of a square matrix
 */
Number determinant(Vector vec);
/**
 * Returns the subsection of a vector with the row and column omitted.
 */
Vector subsection(Vector vec, int row, int column);
/**
 * Transpose the matrix (swap x and y coordinates)
 */
Vector transpose(Vector one);
/**
 * Multiply two matrices (order matters)
 */
Vector matMult(Vector one, Vector two);
/**
 * newVecScalar, but returns a Value
 */
Value newValMatScalar(int type, Number scalar);
/**
 * Creates a one-by-one vector and fills it with num
 */
Vector newVecScalar(Number num);
/**
 * Initializes an empty vector with width and height
 */
Vector newVec(short width,short height);
#pragma endregion
#pragma region Values
Value valMult(Value one, Value two);
Value valAdd(Value one, Value two);
Value valDivide(Value one, Value two);
Value valPower(Value one, Value two);
Value valModulo(Value one, Value two);
Value valNegate(Value one);
Value valLn(Value one);
#pragma endregion
/**
 * Computes the operation tree
 * @param op Tree to compute
 * @param args Arguments (only used for functions)
 */
Value computeTree(Tree op, const Value* args, int argLen, Value* localVars);
#endif