#include "Calc.h"
#include <stdarg.h>
#if defined __linux__ || defined unix || defined __unix__ || defined __APPLE__
#define USE_TERMIOS_H
#endif
#if defined __WIN32 || defined _WIN64 || defined _WIN32
#define USE_CONIO_H
#endif
bool useColors = true;
void CLI_cleanup() {
    cleanup();
    if(useColors) printf("\b\b\33[1;34m-\33[0mquit\n");
    else printf("\b\b-quit\n");
    exit(0);
}
char* readLineRaw() {
    char* out = calloc(10, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    int outLen = 10;
    int charPos = 0;
    while(true) {
        int character = getchar();
        if(character == -1) {
            free(out);
            return NULL;
        }
        if(character == '\n') return out;
        if(charPos == outLen) {
            out = realloc(out, (outLen += 10));
            if(out == NULL) { error(mallocError);return NULL; }
        }
        out[charPos++] = character;
    }
    return out;
}
int readCharacter();
#ifdef USE_TERMIOS_H
#define USE_FANCY_INPUT
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
int getTermWidth() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}
struct termios orig_termios;
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enableRawMode() {
    atexit(disableRawMode);
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
int readCharacter() {
    return getchar();
}
#elif defined USE_CONIO_H
#define USE_FANCY_INPUT
#include <conio.h>
#include <windows.h>
int getTermWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}
void enableRawMode() {

}
void disableRawMode() {
}
int readCharacter() {
    int out = getch();
    if(out == 3) CLI_cleanup();
    return out;
}
#else
int readCharacter() {
    return getchar();
}
void enableRawMode() {

}
void disableRawMode() {

}
#endif
#ifdef USE_FANCY_INPUT
bool useFancyInput = true;
void printInput(char* string, int cursorPos) {
    //Clear old input
    printf("\0338\033[J\r");
    bool isComment = false;
    //Print input
    if(useColors) {
        if(string[0] == '-' || string[0] == '.') {
            printf("\33[1;34m%c\33[0m", string[0]);
            string++;
        }
        if(string[0] == '#' || (string[0] == '/' && string[1] == '/')) {
            isComment = true;
            printf("\33[1;32m");
        }
    }
    printf("%s ", string);
    if(isComment) printf("\33[0m");
    //Set cursor position
    if(cursorPos == -1) return;
    printf("\0338");
    int width = getTermWidth();
    if(cursorPos % width != 0) printf("\033[%dC", cursorPos % width);
    if(cursorPos > width - 1) printf("\033[%dB", cursorPos / width);
}
char* readLine(bool erasePrevious) {
    char* input = calloc(11, 1);
    if(input == NULL) { error(mallocError);return NULL; }
    int strLenAllocated = 10;
    int strLen = 0;
    int cursorPos = 0;
    int character = 0;
    int i;
    if(!erasePrevious) printf("\0337");
    while(1) {
        if(!erasePrevious) printInput(input, cursorPos);
        else {
            printf("\338");
            erasePrevious = false;
        }
        character = readCharacter();
        //Unix escape sequences
        if(character == 27) {
            int next = readCharacter();
            if(next == '[') {
                int next2 = readCharacter();
                //Left Arrow
                if(next2 == 'D') {
                    cursorPos = cursorPos - 1;
                    if(cursorPos == -1) cursorPos = 0;
                    continue;
                }
                //Right Arrow
                if(next2 == 'C') {
                    cursorPos++;
                    if(cursorPos > strLen) cursorPos--;
                    continue;
                }
                //END
                if(next2 == 'F') {
                    cursorPos = strLen;
                    continue;
                }
                //HOME
                if(next2 == 'H') {
                    cursorPos = 0;
                    continue;
                }
                //DELETE
                if(next2 == '3') {
                    readCharacter();
                    if(cursorPos == strLen) continue;
                    int i;
                    for(i = cursorPos;i < strLen;i++) {
                        input[i] = input[i + 1];
                    }
                    strLen--;
                    continue;
                }
            }
        }
        //Windows escape sequences
        if(character == 0) {
            int next = readCharacter();
            //Left arrow
            if(next == 'K') {
                if(cursorPos != 0)
                    cursorPos--;
                continue;
            }
            //Right arrow
            if(next == 'M') {
                cursorPos++;
                if(cursorPos > strLen)
                    cursorPos--;
                continue;
            }
            //Up arrow is 'H'
            //Down arrow is 'P'
            //HOME
            if(next == 'G') {
                cursorPos = 0;
                continue;
            }
            //END
            if(next == 'O') {
                cursorPos = strLen;
                continue;
            }
            //DELETE
            if(next == 'S') {
                if(cursorPos == strLen) continue;
                int i;
                for(i = cursorPos;i < strLen;i++) {
                    input[i] = input[i + 1];
                }
                strLen--;
                continue;
            }
            continue;
        }
        if(character == 127 || character == 8) {
            if(cursorPos == 0)
                continue;
            cursorPos--;
            input[cursorPos] = '\0';
            for(i = cursorPos;i < strLen;i++) {
                input[i] = input[i + 1];
            }
            strLen--;
            continue;
        }
        else if(character == 127) continue;
        if(character == 10 || character == 13) break;
        if(character == 3) CLI_cleanup();
        if(character == 12) {
            printf("\33[2J\033[f\0337");
            continue;
        }
        if(character < 27) {
            for(i = strLen;i > cursorPos - 1;i--) {
                input[i + 1] = input[i];
            }
            input[cursorPos] = '^';
            strLen++;
            cursorPos++;
            character += 64;
        }
        for(i = strLen;i > cursorPos - 1;i--) {
            input[i + 1] = input[i];
        }
        input[cursorPos] = character;
        strLen++;
        if(strLen == strLenAllocated) {
            strLenAllocated += 10;
            input = realloc(input, strLenAllocated + 1);
            if(input == NULL) { error(mallocError);return NULL; }
        }
        cursorPos++;
    }
    printInput(input, -1);
    printf("\n");
    return input;
}
#else
bool useFancyInput = false;
char* readLine(bool erasePrevious) {
    return readLineRaw();
};
#endif
void error(const char* format, ...) {
    //Print error
    if(useColors) printf("\033[1;31m");
    printf("Error: ");
    if(useColors) printf("\033[0m");
    char* dest[256];
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    printf("\n");
    //Set error to true
    globalError = true;
};
void graphEquation(char* equation, double left, double right, double top, double bottom, int rows, int columns) {
    double columnWidth = (right - left) / columns;
    double rowHeight = (top - bottom) / rows;
    char** xArgName = calloc(2, sizeof(char*));
    if(xArgName == NULL) { error(mallocError);return; }
    xArgName[0] = calloc(2, 1);
    if(xArgName[0] == NULL) { error(mallocError);return; }
    xArgName[0][0] = 'x';
    Tree tree = generateTree(equation, xArgName, 0);
    int i;
    Value x = NULLVAL;
    //Compute columns number of values
    double yvalues[columns + 1];
    for(i = 0; i <= columns; i++) {
        x.r = left + columnWidth * i;
        double out = computeTree(tree, &x, 1).r;
        if(verbose)
            printf("Graph(%g)\t= %g\n", x.r, out);
        yvalues[i] = (out - bottom) / rowHeight;
    }
    //Fill text columns
    char text[columns][rows + 1];
    for(i = 0; i < columns; i++) {
        //set column to spaces
        memset(text[i], ' ', rows);
        double start = yvalues[i];
        double end = yvalues[i + 1];
        //how high start and end are within the box
        double startMod = fmod(yvalues[i], 1);
        double endMod = fmod(yvalues[i + 1], 1);
        // -, ", or _
        if(fabs(start - end) < 0.6) {
            if(start < rows && start > 0 && end < rows && end > 0) {
                double avg = (start + end) / 2;
                double avgMod = fmod(avg, 1);
                if(avgMod < 0.7 && avgMod > 0.3) {
                    text[i][(int)avg] = '-';
                    continue;
                }
                if(avgMod > 0.7)
                    text[i][(int)(avg)] = '"';
                else
                    text[i][(int)(avg)] = '_';
            }
            continue;
        }
        char slash = '\\';
        //if(end>start) swap end and start
        if(end > start) {
            double temp = end;
            end = start;
            start = temp;
            slash = '/';
            temp = startMod;
            startMod = endMod;
            endMod = temp;
        }
        //continue if out of range
        if(end > rows - 0.3 || start < 0.3) continue;
        //if dist is close, set character to \ or /
        double dist = start - end;
        if(dist < 1.2) {
            text[i][(int)(start - 0.3)] = slash;
            continue;
        }
        //else have two slashes filled with pipes in between
        int startBox = (int)(start - 0.3);
        if(startBox > rows - 1) startBox = rows - 1;
        int endBox = (int)(end + 0.3);
        if(endBox < 0) endBox = 0;
        int j;
        for(j = endBox; j <= startBox; j++) text[i][j] = '|';
        if(start < rows - 0.6)
            text[i][startBox] = startMod < 0.5 ? ',' : slash;
        if(end > -0.4)
            text[i][endBox] = endMod > 0.5 ? '\'' : slash;
    }

    printf("Graphing: %s %dx%d x: %g to %g y: %g to %g\n", equation, columns, rows, left, right, bottom, top);
    //Print text columns row by row
    for(i = rows - 1; i >= 0; i--) {
        int j;
        for(j = 0; j < columns; j++)
            putchar(text[j][i]);
        putchar('\n');
    }
    freeTree(tree);
}
bool startsWith(char* string, char* sw) {
    int compareLength = strlen(sw);
    return memcmp(string, sw, compareLength) == 0 ? true : false;
}
void printRatio(double out, bool forceSign) {
    char sign = out < 0 ? '-' : '+';
    //Get ratio
    int numer = 0;
    int denom = 0;
    getRatio(out, &numer, &denom);
    //Print number if no ratio exists
    if(numer == 0 && denom == 0) {
        char* string = doubleToString(sign == '-' ? -out : out, 10);
        printf("%s", string);
        if(string != NULL) free(string);
    }
    else {
        if(sign == '-' || forceSign) printf("%c", sign);
        if(forceSign) printf(" ");
        if(floor(fabs(out)) == 0) printf("%d/%d", numer, denom);
        else printf("%d%c%d/%d", (int)floor(fabs(out)), sign, numer, denom);
    }
}
void compPrintRatio(Number num) {
    //Print ratio for R
    if(num.r != 0) printRatio(num.r, false);
    //Print ratio for I
    if(num.i != 0) {
        if(num.r != 0) printf(" ");
        printRatio(num.i, num.r != 0);
        printf(" i");
    }
    //Print unit
    if(num.u != 0) {
        char* string = toStringUnit(num.u);
        printf(" [%s]", string);
        free(string);
    }
}
void runLine(char* input) {
    int i;
    //If command
    if(input[0] == '-') {
        if(startsWith(input, "-def ")) {
            //Define function
            generateFunction(input + 5);
        }
        else if(startsWith(input, "-del ")) {
            int strLen = strlen(input);
            //Delete function or variable
            for(i = 0; i < numFunctions; i++) {
                //Continue if name does not match
                if(customfunctions[i].nameLen != strLen - 5) continue;
                if(strcmp(input + 5, customfunctions[i].name) != 0) continue;
                //Print message
                printf("Function '%s' has been deleted.\n", customfunctions[i].name);
                //Free members
                customfunctions[i].nameLen = 0;
                freeTree(*customfunctions[i].tree);
                free(customfunctions[i].tree);
                free(customfunctions[i].name);
                //Free argNames
                int j = -1;
                char** argNames = customfunctions[i].argNames;
                while(argNames[++j] != NULL) free(argNames[j]);
                free(argNames);
                //Set tree to NULL
                customfunctions[i].tree = NULL;
                customfunctions[i].argCount = 0;
                return;
            }
            //Error if name not found
            error("Function '%s' not found", input + 5);
        }
        else if(startsWith(input, "-g")) {
            //Graph
            char* cleanInput = inputClean(input + 3);
            if(globalError) return;
            graphEquation(cleanInput, -10, 10, 10, -10, 20, 50);
            free(cleanInput);
        }
        else if(startsWith(input, "-f ")) {
            int strLen = strlen(input);
            //Read lines from a file
            FILE* file = fopen(input + 3, "r");
            //Initialize lines
            unsigned long lineSize = 300;
            char* line = malloc(300);
            if(line == NULL) { error("malloc error");return; }
            //Loop through lines
            while(fgets(line, lineSize, file)) {
                int i;
                //Remove endline character
                while(line[i++] != '\0') if(line[i] == '\n')
                    line[i] = '\0';
                //Print and run line
                printf("%s", line);
                runLine(line);
                globalError = false;
            }
            //Close and free line
            free(line);
            fclose(file);
        }
        else if(startsWith(input, "-quit")) {
            //Exit the program
            cleanup();
            exit(0);
        }
        else if(startsWith(input, "-ls")) {
            //ls lists all user-defined functions
            int num = 0;
            for(i = 0; i < numFunctions; i++) {
                if(customfunctions[i].nameLen == 0) continue;
                num++;
                //Get equation
                char* equation = treeToString(*(customfunctions[i].tree), false, customfunctions[i].argNames);
                //Print name
                printf("%s", customfunctions[i].name);
                //Print arguments (if it has them)
                if(customfunctions[i].argNames != NULL) {
                    printf("(");
                    int j;
                    for(j = 0;j < customfunctions[i].argCount;j++) {
                        if(j != 0) printf(",");
                        printf("%s", customfunctions[i].argNames[j]);
                    }
                    printf(")");
                }
                //Print equation
                printf(" = %s\n", equation);
                free(equation);
            }
            printf("There %s %d user-defined function%s.\n", num == 1 ? "is" : "are", num, num == 1 ? "" : "s");
        }
        else if(startsWith(input, "-dx")) {
            //Clean input
            char* cleanInput = inputClean(input + 4);
            if(globalError) return;
            //Set x
            char** x = calloc(2, sizeof(char*));
            if(x == NULL) { error(mallocError);return; }
            x[0] = calloc(2, 1);
            if(x[0] == NULL) { error(mallocError);return; }
            x[0][0] = 'x';
            //Get tree
            Tree ops = generateTree(cleanInput, x, 0);
            free(cleanInput);
            //Clean tree
            Tree cleanedOps = treeCopy(ops, NULL, true, false, true);
            //Get derivative and clean it
            Tree dx = derivative(cleanedOps);
            Tree dxClean = treeCopy(dx, NULL, false, false, true);
            //Print output
            char* out = treeToString(dxClean, false, x);
            printf("=%s\n", out);
            free(out);
            freeTree(cleanedOps);
            freeTree(ops);
            freeTree(dxClean);
            freeTree(dx);
        }
        else if(startsWith(input, "-base")) {
            //format: -base(16) 46 will return 2E
            int i, expStart = 0;
            //Find end of base
            for(i = 5;i < strlen(input);i++) if(input[i] == ' ') {
                expStart = i + 1;
                input[i] = '\0';
                break;
            }
            //Calculate base
            Value baseVal = calculate(input + 5, 0);
            int base = getR(baseVal);
            if(base > 36 || base < 1) {
                error("base out of bounds", NULL);
                return;
            }
            //Calculate and append to history
            Value out = calculate(input + expStart, 0);
            appendToHistory(out, base, true);
        }
        else if(startsWith(input, "-degset")) {
            //If "rad"
            if(input[8] == 'r' && input[9] == 'a' && input[10] == 'd') degrat = 1;
            //If "deg"
            else if(input[8] == 'd' && input[9] == 'e' && input[10] == 'g') degrat = M_PI / 180;
            //If "grad"
            else if(input[8] == 'g' && input[9] == 'r' && input[10] == 'a' && input[11] == 'd') degrat = M_PI / 200;
            //Else custom value
            else {
                Value deg = calculate(input + 7, 0);
                degrat = getR(deg);
                freeValue(deg);
            }
            printf("Degree ratio set to %g\n", degrat);
        }
        else if(startsWith(input, "-unit")) {
            int i, unitStart = 0;
            //Find end of unit
            for(i = 5;i < strlen(input);i++) if(input[i] == ' ') {
                unitStart = i + 1;
                input[i] = '\0';
                break;
            }
            //Calculate unit and value
            Value unit = calculate(input + 5, 10);
            Value value = calculate(input + unitStart, 0);
            //Check if they are compatible
            if(unit.u != value.u) {
                char* unitOne = toStringUnit(unit.u);
                char* unitTwo = toStringUnit(value.u);
                error("units %s and %s are not compatible", unitOne, unitTwo);
                free(unitOne);
                free(unitTwo);
                return;
            }
            //Divide them
            Value out = valDivide(value, unit);
            //Get string
            char* numString = valueToString(out, 10);
            //Free values
            //Print output
            printf("= %s %s\n", numString, input + 5);
            freeValue(unit);
            freeValue(value);
            freeValue(out);
            if(numString != NULL) free(numString);
        }
        else if(startsWith(input, "-parse")) {
            //Clean and generate tree
            char* clean = inputClean(input + 7);
            Tree tree = generateTree(clean, NULL, 0);
            free(clean);
            if(globalError) return;
            //Convert to string, print, and free
            char* out = treeToString(tree, false, NULL);
            printf("Parsed as: %s\n", out);
            freeTree(tree);
            free(out);
        }
        else if(startsWith(input, "-factors")) {
            int num = parseNumber(input + 9, 10);
            int* factors = primeFactors(num);
            //If num is prime
            if(factors[0] == 0) {
                printf("%d is prime\n", num);
            }
            //Else list factors
            else {
                printf("Factors of %d:", num);
                int i = -1;
                int prev = factors[0];
                int count = 0;
                //List through each factor
                while(factors[++i] != 0) {
                    if(factors[i] != prev) {
                        if(count != 1) printf(" %d^%d *", prev, count);
                        else printf(" %d *", prev);
                        count = 1;
                    }
                    else count++;
                    prev = factors[i];
                }
                if(count == 1)printf(" %d\n", prev);
                else printf(" %d^%d\n", prev, count);
            }
            free(factors);
        }
        else if(startsWith(input, "-ratio")) {
            Value out = calculate(input + 7, 0);
            //If out is a number
            if(out.type == value_num) {
                compPrintRatio(out.num);
                printf("\n");
            }
            //If it is a vector
            if(out.type == value_vec) {
                int i, j;
                printf("<");
                int width = out.vec.width;
                //Loop through all numbers
                for(j = 0;j < out.vec.height;j++) for(i = 0;i < width;i++) {
                    Number num = out.vec.val[i + j * width];
                    if(i == 0 && j != 0) printf(";");
                    if(i != 0) printf(",");
                    compPrintRatio(num);
                }
                printf(">\n");
            }
            freeValue(out);
        }
        else {
            error("command '%s' not recognized.", input + 1);
            return;
        }
    }
    //If comment
    else if(input[0] == '#' || (input[0] == '/' && input[0] == '/')) {
        //Replace $() instances
        if(useFancyInput) {
            printf("\0338\33[J\r");
            if(useColors) printf("\33[1;32m");
            int i = -1;
            //Print each character
            while(input[++i] != '\0') {
                //If is $()
                if(input[i] == '$' && input[i + 1] == '(') {
                    int j = i, endBracket = 0, bracketCount = 0;
                    //Find end bracket
                    while(input[++j]) {
                        if(input[j] == '(') bracketCount++;
                        else if(input[j] == ')') if(--bracketCount == 0) {
                            endBracket = j;
                            break;
                        }
                    }
                    //If no end bracket
                    if(endBracket == 0) {
                        error("no ending bracket");
                        return;
                    }
                    //Copy expression to stringToParse
                    char stringToParse[endBracket - i - 1];
                    memcpy(stringToParse, input + i + 2, endBracket - i - 2);
                    stringToParse[endBracket - i - 2] = '\0';
                    //Calculate value
                    Value out = calculate(stringToParse, 0);
                    char* outString = valueToString(out, 10);
                    freeValue(out);
                    //Print value
                    printf("%s", outString);
                    free(outString);
                    i = endBracket + 1;
                }
                //Print character
                putchar(input[i]);
            }
            //Print endline and color reset
            putchar('\n');
            printf("\33[0m");
        }
        //If it is a comment, ignore
        return;
    }
    //If dot command
    else if(input[0] == '.') {
        //If in raw mode
        if(!useFancyInput) {
            error("dot command is not supported in raw mode");
            return;
        }
        //Delete input
        printf("\0338\033[J\r");
        //Calculate value
        Value out = calculate(input + 1, 0);
        char* outString = valueToString(out, 10);
        freeValue(out);
        //Print if no error
        if(!globalError) {
            if(useColors) printf("\33[1;34m= \033[0m%s", outString);
            else printf("= %s", outString);
        }
        free(outString);
        //Read character before being erased by next readLine() call
        readCharacter();
        printf("\0338");
        return;
    }
    //Else compute it as a value
    else {
        if(input[0] == '\0') {
            error("no input", NULL);
            return;
        }
        Value out = calculate(input, 0);
        if(!globalError) appendToHistory(out, 10, true);
    }
}
int main(int argc, char** argv) {
    //Set cleanup on interupt
    signal(SIGINT, CLI_cleanup);
    startup();
    //Parse arguments
    int i;
    for(i = 0; i < argc; i++) {
        if(argv[i][0] == '-') {
            //Verbosity
            if(argv[i][1] == 'v') verbose = true;
            if(argv[i][1] == 'r') useFancyInput = false;
            //Help
            if(argv[i][1] == 'h') {
                if(argc == i + 1) {
                    printf("Calc is a command line calculator written in c\nTry running:\n\tcalc -h commands\n\tcalc -h syntax\n\tcalc -h functions\n\tcalc -h features\n\tor calc -h info\n");
                    return 0;
                }
                if(strcmp("commands", argv[i + 1]) == 0) {
                    printf("Commands:\n\t-def name(a,b)=[exp]\tDefine a function with arguments a and b\n\t-del name\tDelete the function named name\n\t-g [exp]\tGraph the expression when it comes to x\n\t-dx [exp]\tFind the derivative of the function along x\n\t-f [FILE]\tOpen the file and run every line\n\t-ls\tList all user-defined functions\n\t-quit\tExit the program (Ctrl-C also works)\n");
                }
                if(strcmp("syntax", argv[i + 1]) == 0) {
                }
                if(strcmp("functions", argv[i + 1]) == 0) {
                    printf("Built-in functions:\ni            \tThe imaginary number\nadd(a,b)    \tAddition (also +)\nmult(a,b)   \tMultiplication (also *)\ndiv(a,b)    \tDivision (also /)\nmod(a,b)    \tModulo (also %%)\npow(a,b)    \tPower (also ^)\nneg(a)      \tNegate (also -)\nsubtract(a,b)\tSubtraction (also -)\nmultneg(a,b)\tNegative multiplication (also *-)\ndivneg(a,b) \tNegative divide (also /-)\nmodneg(a,b) \tNegative modulo (also %%-)\npowneg(a,b) \tNegative power (also ^-)\n\nsin(x)      \tSine\ncos(x)      \tCosine\ntan(x)      \tTangent\ncsc(x)      \tCosecant (reciprocal sine)\nsec(x)      \tSecant (reciprocal cosine)\ncot(x)      \tCotangent (reciprocal tangent)\nasin(x)     \tInverse sine\nacos(x)     \tInverse cosine(x)\natan(x)     \tInverse tangent\nacsc(x)\tInverse cosecant\nasec(x)      \tInverse secant\nacot(x)     \tInverse cotangent\n\nln(x)        \tNatural log\nlogten(x)   \tLog base 10\nlog(x,b)    \tShorthand for ln(x)/ln(b)\nround(x)    \tNearest integer\nfloor(x)    \tRound down\nceil(x)     \tRound up\nsgn(x)      \tSign\nabs(x)      \tAbsolute value\narg(x)      \tComplex argument\ngetr(x)     \tReal component\ngeti(x)     \tImaginary component\n\nnot(x)      \tBinary not\nand(a,b)    \tBinary and\nor(a,b)     \tBinary or\nxor(a,b)    \tBinary exclusive-or\nls(a,b)     \tBinary leftshift\nrs(a,b)     \tBinary rightshift\n\npi          \tThe circle constant\nphi         \tThe golden ratio\ne           \tEuler's constant\nans         \tPrevious answer\nhist(x)     \tGet xth result from history\nhistnum     \tNumber of items in history\n");
                }
                if(strcmp("features", argv[i + 1]) == 0) {
                    printf("Calc currently has these features:\n\tFunctions\n\tCustom functions\n\tHistory\n\tCLI graphing\n\tComplex number support\n\tBinary operation support\n\tDerivatives\n\tBase convertion\n");
                }
                if(strcmp("info", argv[i + 1]) == 0) {
                    printf("Calc is a command line program witten by Benjamin Cates in C\nCalc is currently in beta\n");
                }
                return 0;
            }
        }
    }
    if(useFancyInput) enableRawMode();
    //Test VT-100 compatability
    printf("\033[c");
    int firstChar = readCharacter();
    if(firstChar != 27) {
        useFancyInput = false;
        printf("\r    \r");
        printf("Reverting to raw mode\n");
        disableRawMode();
        printf("%c", firstChar);
#ifdef USE_CONIO_H
        ungetch(firstChar);
#else
        ungetc(firstChar, stdin);
#endif
    }
    else {
        while(readCharacter() != 'c');
        printf("\r           \r");
    }
    //Main loop
    while(true) {
        char* input;
        if(useFancyInput) input = readLine(globalError);
        else input = readLineRaw();
        if(input == NULL) break;
        globalError = false;
        runLine(input);
        free(input);
    }
    cleanup();
    return 0;
}