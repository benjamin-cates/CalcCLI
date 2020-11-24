#include "Calc.h"
#if defined(__linux__) || defined(unix) || defined(__unix__) || defined(__APPLE__)
#define FANCY_INPUT
#endif
char* readLineRaw() {
    char* out = calloc(10, 1);
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
        }
        out[charPos] = character;
    }
    return out;
}
#ifdef FANCY_INPUT
bool useFancyInput=true;
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
int getTermWidth() {
    struct winsize ws;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);
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
void initializeInput() {
    enableRawMode();
}
char* readLine() {
    char* input = calloc(11, 1);
    int strLenAllocated = 10;
    int strLen = 0;
    int cursorPos = 0;
    int character = 0;
    int i;
    printf("\0337");
    while(1) {
        int width=getTermWidth();
        printf("\0338\033[J\r%s ", input);
        printf("\0338");
        if(cursorPos%width!=0) printf("\033[%dC",cursorPos%width);
        if(cursorPos>width-1) printf("\033[%dB",cursorPos/width);
        character = getchar();
        if(character == 27) {
            int next = getchar();
            if(next == '[') {
                int next2=getchar();
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
                if(next2=='F') {
                    cursorPos=strLen;
                    continue;
                }
                //HOME
                if(next2=='H') {
                    cursorPos=0;
                    continue;
                }
                //DELETE
                if(next2=='3') {
                    getchar();
                    if(cursorPos==strLen) continue;
                    int i;
                    for(i=cursorPos;i<strLen;i++) {
                        input[i]=input[i+1];
                    }
                    strLen--;
                    continue;
                }
            }
        }
        if(character == 127 && cursorPos != 0) {
            cursorPos--;
            input[cursorPos] = '\0';
            for(i = cursorPos;i < strLen;i++) {
                input[i] = input[i + 1];
            }
            strLen--;
            continue;
        }
        else if(character == 127) continue;
        if(character == 10) break;
        if(character<27) {
            for(i=strLen;i>cursorPos-1;i--) {
                input[i+1]=input[i];
            }
            input[cursorPos] = '^';
            cursorPos++;
            character+=64;
        }
        for(i=strLen;i>cursorPos-1;i--) {
            input[i+1]=input[i];
        }
        input[cursorPos] = character;
        strLen++;
        if(strLen == strLenAllocated) {
            strLenAllocated += 10;
            input = realloc(input, strLenAllocated+1);
        }
        cursorPos++;
    }
    printf("\0338\033[J%s\n", input);
    return input;
}
#else
bool useFancyInput=false;
void initializeInput() {

}
char* readLine() {
    return readLineRaw();
};
#endif
void error(const char* format, const char* message) {
    //Print error
    printf("Error: ");
    printf(format, message);
    printf("\n");
    //Set error to true
    globalError = true;
};
void CLI_cleanup() {
    cleanup();
    printf("\b\b-quit\n");
    exit(0);
}
void graphEquation(char* equation, double left, double right, double top, double bottom, int rows, int columns) {
    double columnWidth = (right - left) / columns;
    double rowHeight = (top - bottom) / rows;
    Tree tree = generateTree(equation, "x", 0);
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
int main(int argc, char** argv) {
    //Set cleanup on interupt
    signal(SIGINT, CLI_cleanup);
    startup();
    bool rawInput = false;
    //Parse arguments
    int i;
    for(i = 0; i < argc; i++) {
        if(argv[i][0] == '-') {
            //Verbosity
            if(argv[i][1] == 'v') verbose = true;
            if(argv[i][1] == 'r') rawInput = true;
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
    if(!rawInput) initializeInput();
    //Main loop
    while(true) {
        char* input;
        if(rawInput) input=readLineRaw();
        else input=readLine();
        if(input == NULL) break;
        runLine(input);
    }
    cleanup();
    return 0;
}