#include "../Calc.h"
#include <stdarg.h>
void error(const char* format, ...) {
    //Print error
    printf("Error: ");
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    printf("\n");
    globalError = true;
};
//These tests basic parsing
const char* regularTestText[] = {
    "1",
    "i",
    "1+4",
    "(1-4)+4",
    "3*-4",
    "3**5",
    "3^4",
    "2/4",
    "24.5",
    "si n(0)",
    "tan(0)",
    "[m ]",
    "[m*kg]",
    "run (n=>n+1,4)",
    "run(run(n=>(j=>(n+j)),4),3)",
    "sum(n= > n+1,0, 5,1)",
    "[0A1 ]_16",
    "[0A1 ]_([10000]_2)",
    "run((x, y)=>( x + y) , 5, 3 . 4)",
    "0xA1",
    "0  b101.1",
    "0d5 4.3",
    "0o10.1",
    "abs(<10,4;4,3,1,1;1>)",
    "abs(f  ill(n= >n+1,10,3))",
    "10(4)",
    "run(n=>(5)n,4)",
    //"abs(<10,,5>)",
    "1+2i",
    "10.5e2",
    "1e1",
    "[10e3]_4",
    "0e5",
    "run(n=>{returnn+1},2)",
    "run((a,b)=>{out=0;y=5;while(equal(equal(y,10),0)) {y=y+1;out=out+a+b+y};return out},2,3)",
    "run(n=>{if(equal(n,5)) return 10;else return 0},5)",
    "run(n=>{if(equal(n,5)) return 10;else return 0},2)",
    "run(n=>{out=0;while(1) {if(1)break;out=1};return out;},1)",
    "run(n=>{out=0;while(grthan(rand,0.3)) {if(1)continue;out=1};return out;},1)",
};
const Number regularTestResults[] = {
    {1.0,0.0,0},
    {0.0,1.0,0},
    {5.0,0.0,0},
    {1.0,0.0,0},
    {-12.0,0.0,0},
    {243.000000000000170,0.0,0},
    {81.000000000000028,0.0,0},
    {0.5,0.0,0},
    {24.5,0.0,0},
    {0.0,0.0,0},
    {0.0,0.0,0},
    {1.0,0.0,0x1},
    {1.0,0.0,0x101},
    {5.0,0.0,0},
    {7.0,0.0,0},
    {21.0,0.0,0},
    {161.0,0.0,0},
    {161.0,0.0,0},
    {8.4,0.0,0},
    {161.0,0.0,0},
    {5.5,0,0},
    {54.3,0.0,0},
    {8.125,0.0,0},
    {12.0,0.0,0},
    {33.985290935932859,0.0,0},
    {40.0,0.0,0},
    {20.0,0.0,0},
    {1.0,2.0,0},
    {1050.0,0.0,0},
    {10.0,0.0,0},
    {256.0,0.0,0},
    {0.0,0.0,0},
    {3.0,0.0,0},
    {65.0,0.0,0},
    {10.0,0.0,0},
    {0.0,0.0,0},
    {0.0,0.0,0},
    {0.0,0.0,0},
};
const char* zeroTests[] = {
    "i-i", "neg(1)+1", "-1+1", "pow(10,0)-1", "2^0-1", "mod(10,5)", "10%5", "mult(0,10)", "0*10", "div(10,10)-1", "10/10-1", "add(neg(1),1)", "sub(10.5,10.5)", "sin(0)", "cos(0)-1", "floor(tan(1.56))-92", "csc(pi/2)-1", "sec(pi)+1", "floor(1/cot(1.56))-92", "floor(sinh(10.3))-14866", "cosh(0)-1", "tanh(0)", "asin(1)-pi/2", "acos(1)", "atan(1e50)-pi/2", "acsc(-1)+pi/2", "asec(1)", "acot(1)-pi/4", "asinh(sinh(1))-1", "acosh(cosh(1))-1", "round(atanh(tanh(1)))-1", "sqrt(4)-2", "round(cbrt(8))-2", "exp(ln(2))-2", "ln(exp(2))-2", "round(logten(1000))-3", "round(log(1000,10))-3", "round(fact(3))-6", "sgn(i)-i", "abs(-i)-1", "arg(i)-pi/2", "round(0.5)-1", "floor(0.5)", "ceil(0.3)-1", "getr(i)", "geti(10.5[m])", "getu([km])/[m]-1", "grthan(10,4)-1", "equal(10.3,10.3)-1", "min(4,5)-4", "min(5,4)-4", "max(5,4)-5", "max(4,5)-5", "lerp(-1,1,0.5)", "dist(0,3+4i)-5", "not(1)+2", "and(0,5)", "or(0,5)-5", "xor(5,3)-6", "ls(5,1)-10", "rs(5,1)-2", "floor(1/(pi-3.14))-627", "floor(1/(phi-1.6))-55", "floor(1/(e-2.71))-120", "histnum", "floor(rand)", "run(x=>x+1,-1)", "sum(x=>x,0,10,1)-55", "product(x=>x,1,10,1)-3628800", "width(<>)-1", "height(<10;20>)-2", "length(<1,1,1;1>)-6", "ge(<0,1>,0,0)", "ge(<0,1>,0)", "abs(fill(x=>0,4,4))", "abs(map(<0,1,2,3,4,5>,(v,x)=>v-x))", "det(<0,1;0,2>)", "abs(transpose(<0,0;0>))", "run(n=>{while(1) break;return 2;},0)-2","-1*1-(-1)",

};
int failedCount = 0;
int testIndex = 0;
void failedTest(int index, const char* testType, const char* equation, const char* format, ...) {
    printf("Failed test %d (%s)", index, testType);
    if(globalError) printf(" with error");
    printf(", input was \"%s\": ", equation);
    va_list message;
    va_start(message, format);
    vprintf(format, message);
    va_end(message);
    printf("\n");
    failedCount++;
}
int main() {
    startup();
    int i;
    //Loop over regularTests
    for(i = 0;i < sizeof(regularTestResults) / 24;i++) {
        //Get result and expected
        Value result = calculate(regularTestText[i], 0);
        Number expected = regularTestResults[i];
        //If wrong
        if(result.r != expected.r || result.i != expected.i || result.u != expected.u || globalError) {
            //Print error
            char* resultString = valueToString(result, 10.0);
            char* expectedString = toStringNumber(expected, 10.0);
            failedTest(testIndex, "Regular", regularTestText[i], "expected %s, but got %s", expectedString, resultString);
            free(resultString);
            free(expectedString);
            globalError = false;
        }
        testIndex++;
    }
    testIndex = 0;
    //Unit tests
    for(i = 0;i < unitCount;i++) {
        for(int j = 0;j < metricCount + 1;j++) {
            if(unitList[i].multiplier > 0 && j != metricCount) continue;
            char input[10];
            memset(input, 0, 10);
            int inputPos = 0;
            input[inputPos++] = '[';
            if(j != metricCount) input[inputPos++] = metricNums[j];
            int nameLen = strlen(unitList[i].name);
            memcpy(input + inputPos, unitList[i].name, nameLen);
            inputPos += nameLen;
            input[inputPos++] = ']';
            Value out = calculate(input, 0);
            if(out.u != unitList[i].baseUnits) {
                failedTest(testIndex, "Units (base units)", input, "expected 0x%x, but got 0x%x", unitList[i].baseUnits, out.u);
            }
            if((j == metricCount && out.r != fabs(unitList[i].multiplier)) || (j != metricCount && out.r != fabs(unitList[i].multiplier * metricNumValues[j]))) {
                failedTest(testIndex, "Unit values", input, "expected %g, but got %g", fabs(unitList[i].multiplier * metricNumValues[j]), out.r);
            }
            testIndex++;
            globalError = false;
        }
    }
    //Zero tests
    testIndex = 0;
    for(i = 0;i < sizeof(zeroTests) / sizeof(char*);i++) {
        Value out = calculate(zeroTests[i], 0.0);
        if(out.r != 0 || out.i != 0 || out.u != 0 || globalError) {
            char* str = valueToString(out, 10);
            failedTest(testIndex, "zeroes", zeroTests[i], "expected 0, got %s", str);
            free(str);
            globalError = false;
        }
        freeValue(out);
        testIndex++;
    }
    //Print failed count
    printf("Failed %d %s.\n", failedCount, failedCount == 1 ? "test" : "tests");
    cleanup();
    return 0;
}