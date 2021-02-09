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
    int i;
    //Loop over regularTests
    for(i = 0;i < sizeof(regularTestResults) / 24;i++) {
        //Get result and expected
        Value result = calculate(regularTestText[i], 0);
        Number expected = regularTestResults[i];
        if(globalError) globalError = false;
        //If wrong
        if(result.r != expected.r || result.i != expected.i || result.u != expected.u) {
            //Print error
            char* resultString = valueToString(result, 10.0);
            char* expectedString = toStringNumber(expected, 10.0);
            failedTest(testIndex, "Regular", regularTestText[i], "expected %s, but got %s", expectedString,resultString);
            free(resultString);
            free(expectedString);
        }
        testIndex++;
    }
    testIndex=0;
    for(i = 0;i < unitCount;i++) {
        for(int j = 0;j < metricCount + 1;j++) {
            if(unitList[i].multiplier>0&&j!=metricCount) continue;
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
                failedTest(testIndex, "Units (base units)", input, "expected 0x%x, but got 0x%x",unitList[i].baseUnits,out.u);
            }
            if((j == metricCount && out.r != fabs(unitList[i].multiplier)) || (j != metricCount && out.r != fabs(unitList[i].multiplier * metricNumValues[j]))) {
                failedTest(testIndex, "Unit values", input, "expected %g, but got %g", fabs(unitList[i].multiplier * metricNumValues[j]), out.r);
            }
            testIndex++;
            globalError=false;
        }
    }
    //Print failed count
    printf("Failed %d %s.\n", failedCount, failedCount == 1 ? "test" : "tests");
    return 0;
}