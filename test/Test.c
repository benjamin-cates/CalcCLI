#include "../src/arb.h"
#include "../src/compute.h"
#include "../src/functions.h"
#include "../src/general.h"
#include "../src/parser.h"
#include "../src/help.h"
#include <stdarg.h>
#include <time.h>
#pragma region Commandline arguments
double timer = 0;
double printCommands = 0;
double seed = 0;
double estimatedTime = 3;
double useColors = 1;
const struct CommandArg {
    const char* name;
    double* location;
} commands[] = {
    {"--timer",&timer},
    {"--printcommands",&printCommands},
    {"--seed",&seed},
    {"--time",&estimatedTime},
    {"--color",&useColors},
};
#pragma endregion
#pragma region Global Variables
const char* testType;
bool testExpectsErrors = false;
const char* currentTest = NULL;
int failedCount = 0;
int testIndex = 0;
#pragma endregion
//Use "b exception" to break when an error is printed
void exception() {
    if(useColors) printf("\033[1;31mError: \033[0m");
    else printf("Error: ");
}
//Warning: do not break on calls to "error" in the debugger because error() is called during normal testing. Instead break on exception because it is only called when an error is printed.
void error(const char* format, ...) {
    globalError = true;
    if(testExpectsErrors) return;
    if(ignoreError) return;
    //Print error
    exception();
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    printf("\n");
};
#pragma region Random Generation
char* randomCodeBlock(int nest, char** args, char** localvars, bool isLoop);
char* randomArgList(bool allowNoParenthesis) {
    char* out = calloc(20, 1);
    bool useParenthesis = rand() % 2;
    if(useParenthesis || (!allowNoParenthesis)) {
        const char* acceptableChars = "0123456789..._____,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int acceptableCharsLen = strlen(acceptableChars);
        out[0] = '(';
        //Generate each character
        for(int i = 1;i < 18;i++) {
            out[i] = acceptableChars[rand() % acceptableCharsLen];
            //make sure arguments do not start with 0-9 or .
            if(i == 1) out[1] = 'a';
            else if(out[i - 1] == ',') if((out[i] >= '0' && out[i] <= '9') || out[i] == '.' || out[i] == ',') out[i] = acceptableChars[(rand() % 26) + 99];
        }
        //Confirm there isn't an empty arg at the end
        if(out[17] == ',') out[17] = 'a';
        //Add closing bracket
        out[18] = ')';
    }
    else {
        const char* acceptableChars = "0123456789...______abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int acceptableCharsLen = strlen(acceptableChars);
        for(int i = 0;i < 10;i++)
            out[i] = acceptableChars[rand() % acceptableCharsLen];
        if((out[0] >= '0' && out[0] <= '9') || out[0] == '.') out[0] = 'a';
    }
    return out;
}
char* randomExpression(int nest, char** args, char** localvars, int base, bool useUnits) {
    //Numbers, args, units, or localvars
    if(nest == 0) {
        //Units
        if(useUnits && rand() % 3 == 0) {
            int id = rand() % unitCount;
            char prefix = 0;
            if(unitList[id].multiplier < 0) prefix = metricNums[rand() % metricCount];
            char* out = calloc(strlen(unitList[id].name) + 2, 1);
            if(prefix != 0) out[0] = prefix;
            strcat(out, unitList[id].name);
            return out;
        }
        //Arguments and local variables
        if((args || localvars) && rand() % 3 == 0) {
            char** list;
            if(args && localvars) list = (rand() % 2) ? args : localvars;
            else if(args) list = args;
            else list = localvars;
            int listLen = argListLen(list);
            int id = rand() % listLen;
            char* out = calloc(strlen(list[id]) + 1, 1);
            strcpy(out, list[id]);
            if(useUnits) lowerCase(out);
            return out;
        }
        //Return number
        const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char* out = calloc(11, 1);
        for(int i = 0;i < 10;i++) out[i] = digits[rand() % base];
        out[rand() % 5] = '.';
        int ePos = rand() % 4 + 5;
        out[ePos] = 'e';
        //Prevent letters in exponent
        if(base >= 10) for(int i = ePos;i < 10;i++) out[i] = digits[rand() % 10];
        //Prevent numbers starting with a letter
        if(out[0] >= '9' && out[0] != '.') out[0] = '0';
        return out;
    }
    int type = rand() % 6;
    if(type == 0) {
        //Parenthesis with same length
        char* inside = randomExpression(nest, args, localvars, base, useUnits);
        char* out = calloc(strlen(inside) + 3, 1);
        out[0] = '(';
        memcpy(out + 1, inside, strlen(inside));
        out[strlen(inside) + 1] = ')';
        free(inside);
        return out;
    }
    //Variables or functions
    if(type == 1) {
        //Variable
        int id = sortedBuiltin[rand() % sortedBuiltinLen];
        int argCount = strlen(stdfunctions[id].inputs);
        if(argCount == 0) {
            char* out = calloc(stdfunctions[id].nameLen + 1, 1);
            memcpy(out, stdfunctions[id].name, stdfunctions[id].nameLen);
            if(useUnits) lowerCase(out);
            return out;
        }
        char* arguments[argCount];
        int size = stdfunctions[id].nameLen + 3;
        //Generate arguments
        for(int i = 0;i < argCount;i++) {
            arguments[i] = randomExpression(nest - 1, args, localvars, base, useUnits);
            size += strlen(arguments[i]) + 1;
        }
        //Generate out
        char* out = calloc(size, 1);
        memcpy(out, stdfunctions[id].name, stdfunctions[id].nameLen);
        if(useUnits) lowerCase(out);
        out[strlen(out)] = '(';
        for(int i = 0;i < argCount;i++) {
            strcat(out, arguments[i]);
            free(arguments[i]);
            if(i + 1 != argCount) strcat(out, ",");
        }
        strcat(out, ")");
        return out;
    }
    //Operators
    if(type == 2) {
        //Operator
        const char* operators[] = { "+","-","*","/","^","%","=","==","!=",">","<",">=","<=",
        "+-","--","*-","/-","^-","%-","=-","==-","!=-",">-","<-",">=-","<=-" };
        int op = rand() % (sizeof(operators) / sizeof(char*));
        bool useParenthesis = (op % 13 >= 9 && op % 13 <= 12);
        char* p1 = randomExpression(nest - 1, args, localvars, base, useUnits);
        char* p2 = randomExpression(nest - 1, args, localvars, base, useUnits);
        char* out = calloc(strlen(p1) + strlen(operators[op]) + strlen(p2) + 1 + useParenthesis * 2, 1);
        if(useParenthesis) strcat(out, "(");
        strcat(out, p1);
        strcat(out, operators[op]);
        strcat(out, p2);
        if(useParenthesis) strcat(out, ")");
        free(p1);
        free(p2);
        return out;
    }
    //Square brackets
    if(type == 3) {
        //Unit bracket
        char* baseStr = NULL;
        int newBase = base;
        //Generate base
        if(rand() % 3 == 1) {
            baseStr = randomExpression(1, NULL, NULL, 10, false);
            ignoreError++;
            Value base = calculate(baseStr, 0);
            ignoreError--;
            newBase = getR(base);
            //If error, set base to 10
            if(globalError || newBase <= 1 || newBase >= 36) {
                newBase = 10;
                free(baseStr);
                baseStr = calloc(3, 1);
                memcpy(baseStr, "10", 2);
                globalError = false;
            }
            freeValue(base);
        }
        char* inside = randomExpression(nest - 1, args, localvars, newBase, true);
        //Get length
        int len = strlen(inside) + 6;
        if(baseStr != NULL) len += strlen(baseStr);
        //Compile out
        char* out = calloc(len, 1);
        strcat(out, "[");
        strcat(out, inside);
        strcat(out, "]");
        if(baseStr != NULL) {
            strcat(out, "_");
            bool useBrac = false;
            if(baseStr[0] != '(' && baseStr[0] != '[' && baseStr[0] != '<') {
                strcat(out, "(");
                useBrac = true;
            }
            strcat(out, baseStr);
            if(useBrac) strcat(out, ")");
        }
        free(inside);
        free(baseStr);
        return out;
    }
    //Vectors
    if(type == 4) {
        int count = rand() % 5;
        char* cells[count];
        int semicolonsFlags = rand();
        int len = 3;
        for(int i = 0;i < count;i++) {
            cells[i] = randomExpression(nest - 1, args, localvars, base, useUnits);
            len += strlen(cells[i]) + 1;
        }
        char* out = calloc(len, 1);
        strcat(out, "<");
        for(int i = 0;i < count;i++) {
            strcat(out, cells[i]);
            if(i + 1 != count) strcat(out, (semicolonsFlags >> i) & 1 ? ";" : ",");
            free(cells[i]);
        }
        strcat(out, ">");
        return out;
    }
    //Anonymous functions
    if(type == 5) {
        bool isCodeBlock = rand() % 2;
        char* argList = randomArgList(true);
        char** parsedList = parseArgumentList(argList);
        char** newArgs = mergeArgList(args, parsedList);
        char* expression;
        if(isCodeBlock) {
            expression = randomCodeBlock(nest - 1, newArgs, NULL, false);
        }
        else {
            expression = randomExpression(nest - 1, newArgs, NULL, 10, false);
        }
        char* out = calloc(strlen(argList) + 4 + strlen(expression) + 1, 1);
        strcat(out, argList);
        strcat(out, "=>");
        if(isCodeBlock) strcat(out, "{");
        strcat(out, expression);
        free(expression);
        if(isCodeBlock) strcat(out, "}");
        free(argList);
        freeArgList(parsedList);
        free(newArgs);
        return out;
    }
}
char* randomCodeBlock(int nest, char** args, char** localvars, bool isLoop) {
    char** localvarCopy = argListCopy(localvars);
    int localvarsize = argListLen(localvarCopy);
    int outSize = 100;
    int outLen = 0;
    char* out = calloc(100, 1);
    for(int i = 0;i < 3;i++) {
        int type = rand() % 5;
        char line[1000];
        memset(line, 0, 1000);
        //Local variables
        if(type == 0) {
            //Generate variable name
            char* varName = calloc(6, 1);
            const char* acceptableChars = "0123456789...______abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
            varName[0] = acceptableChars[(rand() % 52) + 19];
            for(int i = 1;i < 5;i++) varName[i] = acceptableChars[rand() % strlen(acceptableChars)];
            char* expression = randomExpression(1, args, localvarCopy, 10, false);
            //Write the line
            strcat(line, varName);
            strcat(line, "=");
            strcat(line, expression);
            free(expression);
            //Append variable name
            argListAppend(&localvarCopy, varName, &localvarsize);
        }
        //Make sure there are no sub blocks when nest is 0
        if(nest == 0 && (type == 1 || type == 2)) type = 3;
        //If and while
        if(type == 1 || type == 2) {
            if(type == 1) strcpy(line, "if(");
            else strcpy(line, "while(");
            //Write conditional
            char* expression = randomExpression(1, args, localvarCopy, 10, false);
            strcat(line, expression);
            free(expression);
            strcat(line, ")");
            //Write code block
            strcat(line, "{");
            char* code = randomCodeBlock(nest - 1, args, localvarCopy, type == 2 ? true : isLoop);
            strcat(line, code);
            free(code);
            strcat(line, "}");
        }
        //Expression
        if(type == 3) {
            char* expression = randomExpression(1, args, localvarCopy, 10, false);
            //Check if line starts with if, while, break...
            if(startsWith(expression, "if") || startsWith(expression, "while") || startsWith(expression, "else") || startsWith(expression, "break") || startsWith(expression, "continue")) {
                type = 4;
            }
            else strcpy(line, expression);
            free(expression);
        }
        //break, continue, and return
        if(type == 4) {
            if(isLoop && rand() % 2) {
                bool isBreak = rand() % 2;
                if(isBreak) strcpy(line, "break");
                else strcpy(line, "continue");
            }
            else {
                strcpy(line, "return");
                char* expression = randomExpression(1, args, localvars, 10, false);
                strcat(line, expression);
                free(expression);
            }
        }
        strcat(line, ";");
        //If length of the line will not fit in the buffer, expand it
        if(strlen(line) + outLen + 4 > outSize) out = recalloc(out, &outSize, strlen(line) + 5, 1);
        strcat(out, line);
        outLen += strlen(line);
        if(type == 4) break;
    }
    freeArgList(localvarCopy);
    return out;
}
#pragma endregion
#pragma region Constants
char validChars[] = "          ()$***+++,,,,--...0123456789;<==>ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^____abcdefghijklmnopqrstuvwxyz{}";
char invalidChars[] = "!\"#%&':=?@\\`|~";
int totalNumberOfTests = 0;
#pragma endregion
void failedTest(int index, const char* equation, const char* format, ...) {
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
#ifdef __linux__
#include <signal.h>
void segFaultHandler(int nSignum, siginfo_t* info, void* p) {
    printf("seg fault on %s", currentTest);
    fflush(stdout);
    exit(0);
}
#endif
#pragma region Tests
void test_standard() {
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
        "run(n=>{if(n==5) return 10;else return 0},5)",
        "run(n=>{if(n=5) return 10;else return 0},2)",
        "run(n=>{out=0;while(1) {if(1)break;out=1};return out;},1)",
        "run(n=>{out=0;while(rand>0.3) {if(1)continue;out=1};return out;},1)",
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
    //Loop over regularTests
    for(int i = 0;i < sizeof(regularTestResults) / 24;i++) {
        //Get result and expected
        currentTest = regularTestText[i];
        Value result = calculate(regularTestText[i], 0);
        Number expected = regularTestResults[i];
        //If wrong
        if(result.r != expected.r || result.i != expected.i || result.u != expected.u || globalError) {
            //Print error
            char* resultString = valueToString(result, 10.0);
            char* expectedString = toStringNumber(expected, 10.0);
            failedTest(i, regularTestText[i], "expected %s, but got %s", expectedString, resultString);
            free(resultString);
            free(expectedString);
            globalError = false;
        }
    }
    totalNumberOfTests += sizeof(regularTestResults) / 24;
}
void test_zeroes() {
    const char* zeroTests[] = {
        "i-i", "neg(1)+1", "-1+1", "pow(10,0)-1", "2^0-1", "mod(10,5)", "10%5", "mult(0,10)", "0*10", "div(10,10)-1", "10/10-1", "add(neg(1),1)", "sub(10.5,10.5)", "sin(0)", "cos(0)-1", "floor(tan(1.56))-92", "csc(pi/2)-1", "sec(pi)+1", "floor(1/cot(1.56))-92", "floor(sinh(10.3))-14866", "cosh(0)-1", "tanh(0)", "asin(1)-pi/2", "acos(1)", "atan(1e50)-pi/2", "acsc(-1)+pi/2", "asec(1)", "acot(1)-pi/4", "asinh(sinh(1))-1", "acosh(cosh(1))-1", "round(atanh(tanh(1)))-1", "sqrt(4)-2", "round(cbrt(8))-2", "exp(ln(2))-2", "ln(exp(2))-2", "round(logten(1000))-3", "round(log(1000,10))-3", "round(fact(3))-6", "sgn(i)-i", "abs(-i)-1", "arg(i)-pi/2", "round(0.5)-1", "floor(0.5)", "ceil(0.3)-1", "getr(i)", "geti(10.5[m])", "getu([km])/[m]-1", "(10>4)-1","(-10>4)", "10!=10","(10=10)-1", "equal(10.3,10.3)-1", "min(4,5)-4", "min(5,4)-4", "max(5,4)-5", "max(4,5)-5", "lerp(-1,1,0.5)", "dist(0,3+4i)-5", "not(1)+2", "and(0,5)", "or(0,5)-5", "xor(5,3)-6", "ls(5,1)-10", "rs(5,1)-2", "floor(1/(pi-3.14))-627", "floor(1/(phi-1.6))-55", "floor(1/(e-2.71))-120", "histnum", "floor(rand)", "run(x=>x+1,-1)", "sum(x=>x,0,10,1)-55", "product(x=>x,1,10,1)-3628800", "width(<>)-1", "height(<10;20>)-2", "length(<1,1,1;1>)-6", "ge(<0,1>,0,0)", "ge(<0,1>,0)", "abs(fill(x=>0,4,4))", "abs(map(<0,1,2,3,4,5>,(v,x)=>v-x))", "det(<0,1;0,2>)", "abs(transpose(<0,0;0>))", "run(n=>{while(1) break;return 2;},0)-2","-1*1-(-1)",
    };
    for(int i = 0;i < sizeof(zeroTests) / sizeof(char*);i++) {
        currentTest = zeroTests[i];
        Value out = calculate(zeroTests[i], 0.0);
        if(out.r != 0 || out.i != 0 || out.u != 0 || globalError) {
            char* str = valueToString(out, 10);
            failedTest(i, zeroTests[i], "expected 0, got %s", str);
            free(str);
            globalError = false;
        }
        freeValue(out);
    }
    totalNumberOfTests += sizeof(zeroTests) / sizeof(char*);
}
void test_units() {
    int testIndex = 0;
    for(int i = 0;i < unitCount;i++) {
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
            currentTest = input;
            Value out = calculate(input, 0);
            if(out.u != unitList[i].baseUnits) {
                testType = "Units (base units)";
                failedTest(testIndex, input, "expected 0x%x, but got 0x%x", unitList[i].baseUnits, out.u);
            }
            if((j == metricCount && out.r != fabs(unitList[i].multiplier)) || (j != metricCount && out.r != fabs(unitList[i].multiplier * metricNumValues[j]))) {
                testType = "Unit values";
                failedTest(testIndex, input, "expected %g, but got %g", fabs(unitList[i].multiplier * metricNumValues[j]), out.r);
            }
            testIndex++;
            globalError = false;
        }
    }
    totalNumberOfTests += testIndex;
}
void test_highlighting() {
    const char* syntax[] = {
        "1+10e2",
        "a=>a^2, a*2",
        "[)]",
        "[1J]_36",
        "[1J]_ 38",
        "<<10;4>,(sqrt(10)+fill(40)>",
        "60*sqrt(10+fill(>)**[Mm+min]/sqrt(10e3)",
        "*/*-",
        "+-",
        "**-",
        "a=>{y=10;if(1) retrun 10;else break;if(1) {j.8=92+a;continue ; while(10+a) {b.2=10;return 15;continue;};return y+b.2+a;}",
    };
    const char* colors[] = {
        "\1\6\1\1\1\1",
        "\20\6\6\20\6\1\13\11\15\6\1",
        "\5\4\5",
        "\5\1\1\5\6\1\1",
        "\5\1\21\5\6\4\4\4",
        "\6\5\1\1\13\1\5\13\4\16\16\16\16\5\1\1\5\6\16\16\16\16\5\1\1\5\6",
        "\1\1\6\16\16\16\16\4\1\1\6\16\16\16\16\5\6\5\6\6\5\21\21\6\21\21\21\5\6\16\16\16\16\5\1\1\1\1\5",
        "\14\14\14\14",
        "\6\6",
        "\6\6\6",
    };
    for(int i = 0;i < sizeof(syntax) / sizeof(char*);i++) {
        //Copy syntax
        char syntaxCopy[strlen(syntax[i])];
        strcpy(syntaxCopy, syntax[i]);
        //Highlight
        char* out = highlightLine(syntaxCopy);
        //Confirm that all characters match
        for(int j = 0;j < strlen(colors[i]);j++) if(colors[i][j] != out[j]) {
            failedTest(i, syntax[i], "expected %s at position %d, but got %s", syntaxTypes[colors[i][j]], j, syntaxTypes[out[j]]);
            break;
        }
        free(out);
    }
}
void test_help() {
    for(int i = 0;i < helpPageCount;i++) {
        struct HelpPage page = pages[i];
        if(page.content == NULL) failedTest(i, pages[i].name, "page.content was NULL");
        int contentLength = strlen(page.content);
        int bracket = 0;
        for(int j = 0;j < contentLength;j++) {
            if(page.content[j] == '<') {
                if(page.content[j + 1] == '/') bracket--;
                else bracket++;
                //ignore <br> tags
                if(memcmp(page.content + j + 1, "br", 2) == 0) bracket--;
            }
        }
        if(bracket != 0) failedTest(i, pages[i].name, "page content has mismatching brackets (%d)", bracket);
    }
}
void test_singleRandomHighlight() {
    char test[50];
    for(int j = 0;j < 49;j++) test[j] = validChars[rand() % (sizeof(validChars) - 1)];
    test[49] = 0;
    char* out = highlightLine(test);
    //Check for an uncaught error
    if(globalError) {
        failedTest(testIndex, test, "recieved error");
        globalError = false;
    }
    //Check for nulls in the output
    for(int j = 0;j < 49;j++) {
        if(out[j] == 0) {
            //Don't print an error for -f command
            if(j == 3 && test[0] == '-' && test[1] == 'f') break;
            failedTest(testIndex, test, "null character at position %d on '%c'", j, test[j]);
            break;
        }
    }
    free(out);
}
void test_singleRandomParse() {
    testExpectsErrors = true;
    char test[11];
    for(int j = 0;j < 10;j++) test[j] = validChars[rand() % (sizeof(validChars) - 1)];
    test[9] = '\0';
    //Test parsing and computing
    inputClean(test);
    Tree tree = generateTree(test, NULL, NULL, 0);
    freeTree(tree);
    globalError = false;
    testExpectsErrors = false;
}
void test_singleRandomExpressionHighlight() {
    //Generate a random expression and highlight it. Fail if any characters are red
    //Generate random expression
    char* test = randomExpression(2, NULL, NULL, 10, false);
    //Color it
    char colors[strlen(test) + 1];
    memset(colors, 0, strlen(test) + 1);
    highlightSyntax(test, colors, NULL, NULL, 10, false);
    //Check whether they are colored as errors
    for(int i = 0;i < strlen(test);i++) {
        //Test if coloring is an error
        if(colors[i] == 12 || colors[i] == 13 || colors[i] == 4 || colors[i] == 0) {
            failedTest(testIndex, test, "%s error at position %d (%c)", syntaxTypes[colors[i]], i, test[i]);
        }
    }
    free(test);
}
void test_singleRandomCompute() {
    char* test = randomExpression(2, NULL, NULL, 10, false);
    currentTest = test;
    //Parse tree
    Tree tree = generateTree(test, NULL, NULL, 0);
    if(globalError) {
        failedTest(testIndex, test, "experienced parsing error");
        globalError = false;
    }
    //Compute tree (only checks for crash)
    testExpectsErrors = true;
    Value out = computeTree(tree, NULL, 0, NULL);
    testExpectsErrors = false;
    globalError = false;
    //Free
    freeTree(tree);
    freeValue(out);
    free(test);
}
enum testType {
    testtype_constant,
    testtype_random,
};
const struct Test {
    void (*test)(void);
    const char* name;
    enum testType type;
} tests[] = {
    {&test_standard,"standard",testtype_constant},
    {&test_zeroes,"zeroes",testtype_constant},
    {&test_units,"units",testtype_constant},
    {&test_highlighting,"highlighting",testtype_constant},
    {&test_help,"help",testtype_constant},
    {&test_singleRandomExpressionHighlight,"random expression highlighting",testtype_random},
    {&test_singleRandomHighlight,"random highlighting",testtype_random},
    {&test_singleRandomParse,"random parsing",testtype_random},
    {&test_singleRandomCompute,"random computation",testtype_random},
};
#pragma endregion
int main(int argc, char** argv) {
#ifdef __linux__
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = segFaultHandler;
    sigaction(SIGSEGV, &action, NULL);
#endif
    //Parse command line arguments
    int commandCount = sizeof(commands) / sizeof(struct CommandArg);
    for(int i = 1;i < argc;i++) {
        for(int j = 0;j < commandCount;j++) {
            if(memcmp(argv[i], commands[j].name, strlen(commands[j].name)) == 0) {
                int eqpos = strlen(commands[j].name);
                if(argv[i][eqpos] != '=') continue;
                *(commands[j].location) = parseNumber(argv[i] + eqpos + 1, 10);
            }
        }
    }
    //Print commands if necessary
    if(printCommands) for(int i = 0;i < sizeof(commands) / sizeof(struct CommandArg);i++) {
        printf("%s set to %g\n", commands[i].name, *commands[i].location);
    }
    startup();
    double remainingTime = estimatedTime;
    //Do tests
    int testCount = sizeof(tests) / sizeof(struct Test);
    for(int i = 0;i < testCount;i++) {
        testType = tests[i].name;
        double start = (double)clock();
        double testTime = remainingTime / (double)(testCount - i);
        if(tests[i].type == testtype_constant) {
            (*(tests[i].test))();
        }
        else if(tests[i].type == testtype_random) {
            srand(seed);
            testIndex = 0;
            clock_t endTime = testTime * CLOCKS_PER_SEC + clock();
            while(clock() < endTime) {
                //Doing 100 will reduce the number of calls to clock()
                for(int j = 0;j < 10;j++) {
                    (*(tests[i].test))();
                    testIndex++;
                }
            }
            totalNumberOfTests += testIndex;
        }
        //Compute time taken
        double total = (double)((double)clock() - start) / (double)CLOCKS_PER_SEC;
        remainingTime -= total;
        if(timer) printf("%s test took %g ms\n", tests[i].name, ((double)total) * 1000);
    }
    printf("Failed %d %s out of %d.\n", failedCount, failedCount == 1 ? "test" : "tests", totalNumberOfTests);
    cleanup();
    return 0;
}