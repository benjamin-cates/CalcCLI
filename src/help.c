//Help.c is an optional file that includes basic functions to use the help pages, and contains help page metadata at the end.
#include "general.h"
#include "help.h"
enum PageType {
    page_basic = 0,
    page_command = 1,
    page_function = 2,
    page_unit = 3,
    page_error = 4,
    page_guide = 5,
    page_internals = 6,
    page_includable_function = 7,
    page_generated = 8,
};
const char* pageTypes[] = {
    "Basic",
    "Command",
    "Function",
    "Unit",
    "Error",
    "Guide",
    "Internals"
};
char* helpPageToJSON(struct HelpPage page) {
    int len = strlen(page.name) + strlen(page.symbol) + strlen(page.tags) + strlen(page.content) + 200;
    char* out = calloc(len, 1);
    strcat(out, "{name:'");
    strcat(out, page.name);
    strcat(out, "',symbol:'");
    strcat(out, page.symbol);
    strcat(out, "',type:");
    snprintf(out + strlen(out), 10, "%d", page.type);
    strcat(out, ", tags:'");
    strcat(out, page.tags);
    strcat(out, "',content:\"");
    strcat(out, page.content);
    strcat(out, "\"}");
    return out;
}
bool str_startsWith(const char* one, char* two, int twoLen) {
    if(one == NULL) return false;
    //Ignore case of first character of one
    if(one[0] >= 'A' && one[0] <= 'Z') {
        if(one[0] + 32 == two[0] && memcmp(one + 1, two + 1, twoLen - 1) == 0) return true;
        else return false;
    }
    return memcmp(one, two, twoLen) == 0;
}
//Similar to one.includes(two) in Javascript
int str_includes(const char* one, char* two, int twoLen) {
    if(one == NULL) return -1;
    int oneLen = strlen(one);
    for(int i = 0;i <= oneLen - twoLen;i++) {
        if(str_startsWith(one + i, two, twoLen)) return i;
    }
    return -1;
}
//Null terminated list of matching pages sorted by importance
int* searchHelpPages(const char* s) {
    //Clean search
    int searchLen = strlen(s);
    char search[searchLen + 1];
    strcpy(search, s);
    lowerCase(search);
    //Find matches
    int matchIDs[helpPageCount];
    int matchPriority[helpPageCount];
    int matchPos = 0;
    for(int i = 0;i < helpPageCount;i++) {
        int priority = 0;
        //If symbol includes
        int symbolPosition = str_includes(pages[i].symbol, search, searchLen);
        //If page title starts with
        if(str_startsWith(pages[i].name, search, searchLen)) priority = 2;
        else if(str_includes(pages[i].name, search, searchLen) != -1) priority = 3;
        if(symbolPosition != -1) {
            //Set high priority if symbol starts with
            //Command starts with
            if(pages[i].type == page_command && symbolPosition == 1) priority = 1;
            //Unit starts with
            else if(pages[i].type == page_unit && symbolPosition == 1) priority = 1;
            //functions
            else if(symbolPosition == 0) priority = 1;
            //Else lower priority
            else if(priority == 0) priority = 3;
        }
        else {
            //If tags include
            int tagPos = str_includes(pages[i].tags, search, searchLen);
            if(tagPos != -1) {
                //If tag starts with (matching position is one past comma)
                if(tagPos == 0 || pages[i].tags[tagPos - 1] == ',') priority = 4;
                //else tag list includes
                else priority = 4;
            }
        }
        //If a match is found, add it to a list
        if(priority) {
            matchIDs[matchPos] = i;
            matchPriority[matchPos] = priority;
            matchPos++;
        }
    }
    //Sort by priority
    int* out = calloc(matchPos + 1, sizeof(int));
    int outPos = 0;
    for(int priorityType = 1;priorityType < 5;priorityType++) for(int i = 0;i < matchPos;i++) {
        if(matchPriority[i] == priorityType) out[outPos++] = matchIDs[i];
    }
    //Terminate with negative one
    out[outPos] = -1;
    //Return
    return out;
}
void removeHTML(char* in) {
    int offset = 0;
    int i;
    for(i = 0;in[i] != 0;i++) {
        if(in[i] == '<') {
            offset++;
            while(in[i] != '>') {
                i++;
                offset++;
            }
            continue;
        }
        if(in[i] == '&') {
            if(memcmp(in + i, "&nbsp;", 6) == 0) {
                in[i - offset] = ' ';
                i += 5;
                offset += 5;
            }
            else if(memcmp(in + i, "&lt;", 4) == 0) {
                in[i - offset] = '<';
                i += 3;
                offset += 3;
            }
            else if(memcmp(in + i, "&gt;", 4) == 0) {
                in[i - offset] = '>';
                i += 3;
                offset += 3;
            }
            else in[i - offset] = '&';
        }
        in[i - offset] = in[i];
    }
    in[i - offset] = 0;
}
char* getGeneratedPage(struct HelpPage page) {
    int outLen = 1;
    int validPages[helpPageCount];
    int validCount = 0;
    int validType = page_function;
    if(strcmp(page.name, "List of units") == 0) validType = page_unit;
    else if(strcmp(page.name, "List of commands") == 0) validType = page_command;
    for(int i = 0;i < helpPageCount;i++) {
        if(pages[i].type == validType) {
            validPages[validCount++] = i;
            outLen += strlen(pages[i].symbol) + strlen(pages[i].name) + 45;
        }
    }
    char* out = calloc(outLen, 1);
    //Generated pages are unit list, commands, functions, -ls
    if(validType == page_function) {
        for(int i = 0;i < validCount;i++) {
            struct HelpPage page = pages[validPages[i]];
            strcat(out, "<syntax><help>");
            //Append symbol
            int brac = findNext(page.symbol, 0, '(');
            if(brac == -1) brac = strlen(page.symbol);
            //Append name
            char name[brac + 1];
            memcpy(name, page.symbol, brac);
            name[brac] = 0;
            strcat(out, name);
            strcat(out, "</help>");
            //Append arguments
            strcat(out, page.symbol + brac);
            strcat(out, "</syntax> - ");
            strcat(out, page.name);
            strcat(out, "<br>");
        }
    }
    if(validType == page_unit) {
        for(int i = 0;i < validCount;i++) {
            struct HelpPage page = pages[validPages[i]];
            strcat(out, "<syntax>[<help>");
            memcpy(out + strlen(out), page.symbol + 1, strlen(page.symbol) - 2);
            strcat(out, "</help>]</syntax> - ");
            strcat(out, page.name);
            strcat(out, "<br>");
        }
    }
    if(validType == page_command) {
        for(int i = 0;i < validCount;i++) {
            struct HelpPage page = pages[validPages[i]];
            strcat(out, "<syntax><help>");
            strcat(out, page.symbol);
            strcat(out, "</help></syntax> - ");
            strcat(out, page.name);
            strcat(out, "<br>");
        }
    }
    return out;
}
const struct HelpPage pages[helpPageCount] = {
    #pragma region Basic Guides
    {"Getting started", NULL, page_guide,NULL,"This guide aims to explain the basic functions and features of the program. <h2>Interface</h2>Type expressions into the box and press enter to evaluate them. Answers will appear below. In the bottom right corner there are two buttons: one for settings, and one for the help pages. The setting/help pages will appear in the panel on the right.<h2>Expressions</h2>Expressions are mathematical statements represented in text. These are parsed into function trees by the program, and then evaluated. Expressions are made of: numbers, operators, function calls, and parenthesis. An example expression might be: <syntax>sqrt(10)+3</syntax>. When <help>syntax highlighting</help> is enabled, these are colored in.<h3>Numbers</h3>Numbers are one of the many types of values this program can represent. Each number is a combination of one floating point real number, one floating point imaginary number, and a <help title='units'>unit</help>. Numbers support the 'e' suffix for exponentials. Ex: <syntax>1.543e50</syntax><h3>Operators</h3>Together with functions, operators are the 'glue' that make an expression, as in they describe a way that values interact. See a list of supported operators <help title='operator'>here</help>. This program supports, addition(+), subtraction(-), multiplication(*), division(/), exponentiation(^), and mod(%) as operators. <h3>Functions</h3>Functions are relationships between inputs and outputs. Most functions are called using the name-parenthesis syntax, for example, <syntax>mod(10,3)</syntax> is being passed two arguments, 10 and 3. Some functions are constant and require no inputs, like <help><syntax>pi</syntax></help>, and others accept no arguments, but are not constant. Like <help><syntax>ans</syntax></help> and <help><syntax>rand</syntax></help>. The statements within the parenthesis are evaluated first. See the list of over 50 builtin functions <help title='functions'>here</help>.<h2>Basic Features</h2><h3>Commands</h3>Commands are statements that cannot normally be written as an expression. They serve a special purpose to allow many features. All commands start with the '-' character. One command is the '<help><syntax>-ratio</syntax></help>' command, which will print the result as a ratio. A list of commands can be seen <help title='command'>here</help>.<h3>Comments</h3>Comments are statements that aren't evaluated, they can start with either '//' or '#'. Expressions can also be written <help title='comments'>within comments</help>.<h3>Custom functions</h3> Define custom functions with the <syntax>-def</syntax> command. The syntax is <syntax>-def a(x)=x^2</syntax>. See more <help title='-def'>here</help><h2>Other features</h2><ul><li><help title='anonymous functions'>Anonymous Functions</help></li><li><help title='vectors'>Vectors and Matrices</help></li><li><help title='base conversion'>Base Conversion</help></li></ul><h3>Search</h3>Click <a href='javascript:openPanelPage(2)'>here</a> to search the help pages.<h3>Bugs</h3>Occasionally, this program has minor or fatal bugs, if you'd like to report them, submit an issue at <a href='https://github.com/Unfit-Donkey/CalcCLI/issues'>github.com/Unfit-Donkey/CalcCLI/issues</a><h3>Source Code</h3>The source code is available at <a href='https://github.com/Unfit-Donkey/CalcCLI>github.com/Unfit-Donkey/CalcCLI</a>"},
    {"Info", NULL,page_guide,NULL,"This is a personal project of <a href='https://github.com/Unfit-Donkey'>Benjamin Cates</a>. <a href='https://github.com/Unfit-Donkey/CalcForJS'>CalcForJS</a> is a web port of <a href='https://github.com/Unfit-Donkey/CalcCLI'>CalcCLI</a>. The webport is compiled using the <a href='https://emscripten.org/'>emscripten</a> libraries. CalcCLI is a command line calculator written in C."},
    {"Numbers", NULL,page_basic,NULL,"Numbers are mathematical objects used to represent quantities. Numbers are written with arabic numerals in base-10 (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10). For non-integer quantities, use the period (.), for example, five and a half is written as \"5.5\". In some places, commas are used to separate groups of digits, for example, one thousand can be written as 1,000. This type of writting numbers is not supported, as commas are used to separate items in a list. If you prefer to separate digits for readability, use spaces instead because they are ignored.<br> <h4>Bases</h4> See <a href='javascript:openHelp(\"base conversion\")'>base conversion</a> for more info.<br> <h4>Imaginary numbers and units</h4>In this calculator, numbers are internally stored in three components: real, <a href='javascript:openHelp(\"i\")'>imaginary</a>, and <help>units</help>."},
    {"Operators", NULL,page_basic,NULL,"Operators separate expressions with a function; it is a shorthand notation for the actual function notation. For example, <syntax>a+b</syntax> is shorthand for <syntax>add(a,b)</syntax><table class='tableborder'><thead><tr><td>Name    </td><td>Func</td><td>Op</td><td>Order</td></tr></thead><tbody><tr><td>Add     </td><td><help>add</help></td><td>+</td><td>3rd</td></tr><tr><td>Subtract</td><td><help>sub</help></td><td>-</td><td>3rd</td></tr><tr><td>Multiply</td><td><help>mult</help></td><td>*</td><td>2nd</td></tr><tr><td>Divide  </td><td><help>div</help></td><td>/</td><td>2nd</td></tr><tr><td>Modulo  </td><td><help>mod</help></td><td>%</td><td>2nd</td></tr><tr><td>Exponentiate</td><td><help>pow</help></td><td>^ or **</td><td>1st</td></tr></tbody></table>All operators support the negative sign after it, <syntax>4*-3</syntax>, for example."},
    {"Units", NULL,page_basic,NULL, "This is an automatically generated list of units. Some units support <help>metric prefixes</help>. All units are case-sensitive and are only recognized between the unit brackets, \"[\" and \"]\". This is to avoid potential name overlap between units and custom variables. All units interact via multiplication and division, but adding numbers with different units is not allowed. Internally, units are stored as an array of 8 <help>base units</help>.<br>Examples:<ul><li><syntax>400[W]*10[s]</syntax> = <syntax>4000[J]</syntax></li></ul>", },
    {"List of units", NULL,page_generated,NULL,""},
    {"Base units", NULL,page_basic,"metric","Each unit in this calculator is stored as an array of base units. In the SI system, there are seven base units, but in this program, the canela is ommitted and replaced with the <help>bit</help>. Additionally, the unit of <help>currency</help> is also added in. The exponent of each base unit is represented as an integer from -128 to 127. This allows 8 base units to be fit into a 64-bit integer. If a calculation goes past these ranges, it will overflow (wrap around). Here is a list of base units:<table class='tableborder'><thead><tr><td>Name    </td><td>Symbol</td><td>Type</td></tr></thead><tbody><tr><td>Meter   </td><td><help>m</help></td><td>Length</td></tr><tr><td>Kilogram</td><td><help>kg</help></td><td>Mass</td></tr><tr><td>Second  </td><td><help>s</help></td><td>Time</td></tr><tr><td>Ampere  </td><td><help>A</help></td><td>Electric Current</td></tr><tr><td>Kelvin  </td><td><help>K</help></td><td>Temperature</td></tr><tr><td>Mole    </td><td><help>mol</help></td><td>Substance</td></tr><tr><td>Dollar  </td><td><help>$</help></td><td>Currency</td></tr><tr><td>Bit     </td><td><help>b</help></td><td>Information</td></tr></tbody></table>"},
    {"Metric prefixes",NULL,page_basic,NULL,"Units that are equal to their metric base units support metric prefixes. To do this, the unit brackets (\"[\" and \"]\") are case-sensitive. Here is a list of prefixes:<table class='tableborder'><thead><tr><td>Prefix</td><td>Name</td><td>Value</td></tr></thead><tbody><tr><td>Y</td><td>Yotta</td><td>10e24</td></tr><tr><td>Z</td><td>Zeta</td><td>10e21</td></tr><tr><td>E</td><td>Exa</td><td>10e18</td></tr><tr><td>P</td><td>Peta</td><td>10e15</td></tr><tr><td>T</td><td>Tera</td><td>10e12</td></tr><tr><td>G</td><td>Giga</td><td>10e9</td></tr><tr><td>M</td><td>Mega</td><td>1 000 000</td></tr><tr><td>k</td><td>Kilo</td><td>1 000</td></tr><tr><td>Z</td><td>Hecto</td><td>100</td></tr><tr><td>c</td><td>Centi</td><td>0.01</td></tr><tr><td>m</td><td>Milli</td><td>0.001</td></tr><tr><td>u</td><td>Micro</td><td>0.000 001</td></tr><tr><td>n</td><td>Nano</td><td>10e-9</td></tr><tr><td>p</td><td>Pico</td><td>10e-12</td></tr><tr><td>f</td><td>Fempto</td><td>10e-15</td></tr><tr><td>a</td><td>Atto</td><td>10e-18</td></tr><tr><td>z</td><td>Zepto</td><td>10e-21</td></tr><tr><td>y</td><td>Yocto</td><td>10e-24</td></tr></tbody></table>"},
    {"Base conversion",NULL,page_basic,NULL,"This calculator supports 5 base prefixes, they are listed here: <table class='tableborder'><thead><tr><td>Prefix</td><td>Base</td><td>Name</td></tr></thead><tbody><tr><td>0x</td><td>16</td><td>Hexadecimal</td></tr><tr><td>0d</td><td>10</td><td>Decimal</td></tr><tr><td>0o</td><td>8</td><td>Octal</td></tr><tr><td>0t</td><td>3</td><td>Ternary</td></tr><tr><td>0b</td><td>2</td><td>Binary</td></tr></tbody></table> Output base with the <help><syntax>-base</syntax></help> command.<h3>Arbitrary Base</h3>The syntax for parsing a number in an arbitrary base is: <em>[{exp}]_{base}</em>. The base can be any positive, real number. Inside the bracket, the digits go 0-9, A-Z (capitalized), this means there is a maximum useful base of 36. If a number starts with a character, it is parsed as a variable or unit, so precede them with a zero. Example: <syntax>[0A1]_16</syntax> = <syntax>161</syntax>. The base can also be an expression inside parenthesis, but it must be a constant, so <syntax>-def a(x)=[10]_x</syntax> will not work.<br>Examples:<br><ul><li><syntax>[10.1]_2.5</syntax> = <syntax>2.9</syntax></li><li><syntax>[1000]_8</syntax> = <syntax>512</syntax></li><li><syntax>[10]_ans</syntx></li><li><syntax>[1011]_(-1)</syntax> = <syntax>3</syntax></li><li><syntax>[0J]_36</syntax> = <syntax>19</syntax></li><li>Note: <syntax>[J]_36</syntax> will be parsed as 1 joule.</li></ul>"},
    {"List of commands",NULL,page_generated,NULL,""},
    {"Comments",NULL,page_basic,NULL,"Comments are notes that you can place in the history. All comments either start with '//' or '#'. Expressions can also be evaluated within comments using the '$()' synatx. Example: \"<syntax>// Energy required: $(500[kW]*3[hr])</syntax>\" will put \"<syntax>// Energy required: 5400000000[J]</syntax>\" into the history."},
    {"Syntax highlighting",NULL,page_basic,NULL,"Calculator automatically highlights the input syntax. This can be disabled in the settings. When a character or variable is highlighted red, it is invalid; this also applies to unmatched brackets or parenthesis. Syntax highlighting closely matches parsing, but may not be exact."},
    {"List of functions", NULL,page_generated,"list",""},
    {"Custom functions",NULL,page_basic,"variables", "Define with <help><syntax>-def</syntax></help>, Delete with <help><syntax>-del</syntax></help>, List with <help><syntax>-ls</syntax></help>"},
    {"Anoynmous functions", NULL,page_basic,"lambda,arrow notation,=>","<strong>Anonymous functions</strong>, also known as lambda funcitons, are created with arrow notation ('=&gt;'). Anonymous functions are written as <em>n=&gt;exp</em>. <em>n</em> can be any valid variable name. For multiple inputs, wrap them in parenthesis and separate by commas, ex: <syntax>(x,y)=&gt;x+y</syntax>. Anonymous functions are only accepted in the <syntax>run</syntax>, <syntax>sum</syntax>, <syntax>product</syntax>, <syntax>fill</syntax>, and <syntax>map</syntax>; passing them to any other builtin-function will return an error. Examples:<br><ul><li><syntax>run((x,y)=&gt;x+y,10,2)</syntax> = <syntax>12</syntax></li><li><syntax>fill(n=&gt;2n,5,1)</syntax> = <syntax>&lt;0,2,4,6,8&gt;</syntax></li><li><syntax>map(&lt;1,2;4,3&gt;,n=&gt;n+1)</syntax> = <syntax>&lt;2,3;5,4&gt;</syntax></li></ul>"},
    {"Vectors", NULL,page_basic,"matrix,matrices","A vector is a 2D list of <help title='number'>numbers</help> because they each contain a real component, an imaginary component and a unit. The syntax for vectors is to wrap them in angle brackets, they start with '&lt;' and end with '&gt;', commas ',' separate elements, and ';' separate rows. Like most programming languages, the first element has an index of zero; make sure to keep this in mind. One useful situation for a vector is to return multiple numbers from a function. For example, the quadratic formula can be written as <syntax>-def solvequad(a,b,c)=(&lt;-b,-b&gt;+&lt;1,-1&gt;*sqrt(b^2-4a*c))/2a</syntax>. Vectors can also be treated as matrices using the <help><syntax>mat_mult</syntax></help>, <help><syntax>mat_inv</syntax></help>, and <help><syntax>det</syntax></help> functions.<br>Examples:<ul><li><syntax>&lt;1,2&gt;</syntax> is a list with 1 and 2.</li><li>&lt;1,2;4,3&gt; is a 2 by 2 matrix with 1 and 2 in the first row and 4 and 3 in the second row. The 1 is in position (0,0) with index 0, and the 4 is in position (0,1) with index 3. Basically, it is stored as &lt;1,2,4,3&gt; with a width of 2.</li><li>&lt;1;2,3,4;0,5&gt; is stored as &lt1,0,0;2,3,4;0,5,0&gt; with a width of 3 becuase each row is filled with zeroes to achieve an even row width.</li></ul> Special functions:<ul><li><help><syntax>length</syntax></help> returns the total number of elements.</li><li><help><syntax>width</syntax></help> returns the width of the vector.</li><li><help><syntax>height</syntax></help> returns the height of the vector.</li><li><help><syntax>ge</syntax></help> returns a cell at specific coordinates.</li><li><help><syntax>fill</syntax></help> will fill a vector with a constant or an expression.</li><li><help><syntax>map</syntax></help> will map a vector's values using a function.</li><li><help><syntax>det</syntax></help> returns the determinant of a square matrix</li><li><help><syntax>transpose</syntax></help> will transpose the elements across the diagonal</li><li><help><syntax>mat_mult</syntax></help> returns the prouct of two matrices.</li><li><help><syntax>mat_inv</syntax></help> returns the inverse of a matrix.</li></ul>"},
    {"Local variables", NULL,page_basic,"variables","Local variables are temporary variables used to store values. As opposed to custom functions with no inputs, local variables store a value, not an expression. Local variables are defined with an name, an equal sign, and an expression. The name must be at the start of the line, they cannot be set within functions. If the local variable name is already taken, the program will overwrite the old one. To view local variables, run the <help title='-ls'><syntax>-ls local</syntax></help> function. <br>Examples:<ul><li><syntax>x=sqrt(4-4*2*3)</syntax></li><li><syntax>gamma=x=>fact(x-1)</syntax></li></ul>"},
    #pragma endregion
    #pragma region Commands
    {"Define", "-def",page_command,"define,custom function","The <syntax>-def</syntax> command is used to define custom functions.<br>Syntax:<br><syntax>-def</syntax> {name}{argList}={expression}<br><br>If no argument list is provided, the function will still remain dynamic. Warning: <syntax>-def a=ans-1</syntax> will not behave as you expect. To delete a function, use the <help><syntax>-del</syntax></help> command.<br><br>Examples:<ul><li><syntax>-def m=45</syntax></li><li><syntax>-def npr(n,r)=fact(n)/fact(n-r)</syntax></li><li><syntax>-def getrow(vec,row)=fill(x=>ge(vec,x,row),width(vec),1)</syntax></li></ul>"},
    {"Delete", "-del",page_command,"undefine,custom function","Delete custom functions with <syntax>-del</syntax>.<br>Syntax:<br>-del {name}<br><br>To redefine a function, you have to delete it first."},
    {"List", "-ls",page_command,"custom function","The <syntax>-ls</syntax> command lists all currently defined custom functions. Custom functions are defined with <a href='javascript:openHelp(\"-def\")'>-def</a> and deleted with <a href='javascript:openHelp(\"-del\")'>-del</a>."},
    {"Degree ratio","-degset",page_command,"degrees,radians,gradians,trigonometry","You can convert degrees using the degset command. <syntax>-degset</syntax> accepts four possible inputs:<ul><li>deg (= pi/180)</li><li>rad (= 1)</li><li>grad (= pi/200)</li><li>Custom value (evaluates an expression)</li></ul>The degree ratio changes the outputs of all <a href='javascript:helpSearch(\"trigonometry\")'>trigonometric functions</a>. Examples: <br><syntax>-degset pi/100</syntax> will set it to 200 degrees per circle. <syntax>sin(100)</syntax> will return 0."},
    {"Derivative","-dx",page_command,"slope,calculus","<sytnax>-dx</syntax> returns the derivative of the input regarding x"},
    {"Base","-base",page_command,NULL,"<syntax>-base</syntax> returns a number converte to a different base.<br>Syntax:<br>-base{ret} {exp}<br><br>Examples:<br><ul><li><syntax>-base2 100</syntax> = <syntax>1100100</syntax></li><li><syntax>-base3 10</syntax> = <syntax>101</syntax></li></ul>The return type can be any integer from 2 to 36."},
    {"Unit convert","-unit", page_command,NULL,"Return the second argument converted to the first input.<br>Syntax:<br>-unit{dest} {exp}.<br><br>Examples:<br><ul><li><syntax>-unit[psi] [atm]</syntax> = <syntax>14.69... [psi]</syntax></li><li><syntax>-unit[mi/s] [c]</syntax> = <syntax>186282.397... [mi/s]</syntax></li></ul>For the first argument, the square brackets are not required, but it looks messy."},
    {"Graph","-g", page_command,NULL,"<syntax>-g</syntax> is in experimental mode, do not use it"},
    {"Parse","-parse", page_command,NULL,"<syntax>-ratio</syntax> command appends a number or vector to history and prints it as a ratio. It uses continued fractions to estimate a ratio. If the contnued fraction does not terminate before 20 digits, it will print the number as a decimal. If the numerator is greated than the denominator, it will print it as a mixed number."},
    {"Factorize","-factor",page_command,"-factors,prime","The <syntax>-factors</syntax> command (also known as <syntax>-factor</syntax>) reports the prime factors of a number. The command will only run for 32-bit integers, meaning numbers greater than 2 000 000 000 are not supported. The command will either report that the number is prime, or the list of factors that make it. <br>Examples:<ul><li><syntax>-factors 1504</syntax> = <syntax>2^5 * 47</syntax></li></ul>"},
    {"Help","-help", page_command,"search,f1","<strong>-help {name}</strong> opens the help page that most closely resembles name. If no name is provided, the main help menu is opened."},
    #pragma endregion
    #pragma region Builtin Functions
    {"i","i", page_function,"imaginary number","<syntax>i</syntax> is the imaginary number, equal to the square root of <syntax>-1</syntax>."},
    #pragma region Basic Operators
    {"Negate","neg(a)",page_function,"negative,-","<syntax>neg</syntax>(a) returns the negative of a. It is identical to -a."},
    {"Power","pow(a,b)",page_function,"exponent,power,^","<syntax>pow</syntax>(a,b) returns a to the power of b. <em>pow(a,b)</em> is equivalent to a^b."},
    {"Modulus", "mod(a,b)",page_function,"modulo,%","<strong>mod(a,b)</strong> returns the remainder of a/b. It is equivalent to a%b."},
    {"Multiply","mult(a,b)",page_function,"multiplication,product,*","<strong>mult(a,b)</strong> returns a multiplied by b, equivalent to a*b.<br>Examples<ul><li><syntax>3*4</syntax> = 12</li><li><syntax>&lt;1,2,3&gt;*3</syntax> = <syntax>&lt;2,4,6&gt;</syntax></li><li><syntax>&lt;1,2;4,3&gt;*&lt;4,3,2&gt;</syntax> = <syntax>&lt;4,6&gt;</syntax></li></ul> For matrix multiplication (opposed to vector multiplication), use the <help><syntax>mat_mult</syntax></help> function."},
    {"Divide","div(a,b)",page_function,"division,quotient,/","<strong>div(a,b)</strong> returns a divided by b. Can also be written as a/b."},
    {"Add","add(a,b)",page_function,"addition,plus,sum,+","<strong>add(a,b)</strong> returns the sum of <em>a</em> and <em>b</em>. <strong>add</strong> is identical to the <em>+</em> operator"},
    {"Subtract","sub(a,b)",page_function,"subtraction,minus,difference,-","<strong>sub(a,b)</strong> returns the difference of a and b. It can also be written as a-b."},
    #pragma endregion
    #pragma region Trigonometry
    {"Sine", "sin(x)",page_function,"trigonometry","<strong>sin(x)</strong> returns the sine of <em>x</em>. Note: The output of <em>sin</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Cosine","cos(x)",page_function,"trigonometry","<strong>cos(x)</strong> returns the cosine of <em>x</em>. <em>cos(x)</em> is equivalent to <em><help>sin</help>(<help>pi</help>-x)</em>. Note: The output of <em>cos</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Tangent","tan(x)",page_function,"trigonometry","<strong>tan(x)</strong> returns the tangent of <em>x</em>. <em>tan(x)</em> is equivalent to <em><help>sin</help>(x)/<help>cos</help>(x)</em>. Note: The output of <em>tan</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Secant", "sec(x)",page_function,"trigonometry","<strong>sec(x)</strong> returns the secant of <em>x</em>. This is equivalent to <em>1/<help>cos</help>(x)</em>. Note: The output of <em>sec</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Cosecant","csc(x)",page_function,"trigonometry","<strong>csc(x)</strong> returns the cosecant of <em>x</em>. <em>csc(x)</em> is equivalent to <em>1/<help>sin</help>(x)</em>. Note: The output of <em>csc</em> is affected by the <help>degree ratio</help>."},
    {"Cotangent", "cot(x)",page_function,"trigonometry","<strong>cot(x)</strong> returns the cotangent of <em>x</em>. <em>cot(x)</em> is equivalent to <em><help>cos</help>(x)/<help>sin</help>(x)</em>. Note: The output of <em>cot</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Hyperbolic sine","sinh(x)",page_function,"trigonometry","<strong>sinh(x)</strong> returns the hyperbolic sine of <em>x</em>. <em>sinh(x)</em> is equivalent to <em>-i * <help>sin</help>(i*x)</em>, and <em>(<help>exp</help>(x)-exp(-x))/2</em>. Note: The output of <em>sinh</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Hyperbolic cosine","cosh(x)",page_function,"trigonometry","<strong>cosh(x)</strong> returns the hyperbolic cosine of <em>x</em>. <em>cosh(x)</em> is equivalent to <em><help>cos</help>(i*x)</em>, and <em>(<help>exp</help>(x)+exp(-x))/2</em>. Note: The output of <em>cosh</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Hyperbolic tangent","tanh(x)",page_function,"trigonometry","<strong>tanh(x)</strong> returns the hyperbolic tangent of <em>x</em>. <em>tanh(x)</em> is equivalent to <em><help>sinh</help>(x)/<help>cosh</help>(x)</em>. Note: The output of <em>tanh</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse sine","asin(x)",page_function,"arcsine,trigonometry","<strong>asin(x)</strong> returns the inverse sine of <em>x</em>. Note: The output of <em>asin</em> is affected by the <help tile='degset'>degree ratio</help>."},
    {"Inverse cosine","acos(x)",page_function,"arccosine,trigonometry","<strong>acos(x)</strong> returns the inverse cosine of <em>x</em>. Note: The output of <em>acos</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse tangent","atan(x)",page_function,"arctangent,trigonometry","<strong>atan(x)</strong> returns the inverse tangent of <em>x</em>. Note: The output of <em>atan</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse secant","asec(x)",page_function,"arcsecant,trigonometry","<strong>asec(x)</strong> returns the inverse secant of <em>x</em>. Note: The output of <em>sec</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse cosecant","acsc(x)",page_function,"arccosecant,trigonometry","<strong>acsc(x)</strong> returns the inverse cosecant of <em>x</em>. Note: The output of <em>acsc</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse cotangent","acot(x)",page_function,"arccotangent,trigonometry","<strong>acot(x)</strong> returns the inverse cotangent of <em>x</em>. Note: The output of <em>cot</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse hyperbolic sine","asinh(x)",page_function,"hyperbolic arcsine,trigonometry","<strong>asinh(x)</strong> returns the inverse hyperbolic sine of <em>x</em>. Note: The output of <em>sinh</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse hyperbolic cosine","acosh(x)",page_function,"hyperbolic arccosine,trigonometry","<strong>acosh(x)</strong> returns the inverse hyperbolic cosine of <em>x</em>. Note: The output of <em>cosh</em> is affected by the <help title='degset'>degree ratio</help>."},
    {"Inverse hyperbolic tangent","atanh(x)",page_function,"hyperbolic arctangent,trigonometry","<strong>atanh(x)</strong> returns the inverse hyperbolic tangent of <em>x</em>. Note: The output of <em>tanh</em> is affected by the <help title='degset'>degree ratio</help>."},
    #pragma endregion
    #pragma region Common functions (sqrt, exp, log)
    {"Square root","sqrt(x)",page_function,NULL,"<strong>sqrt(x)</strong> returns the square root of <em>x</em>. It is equivalent to <em>x^0.5</em>."},
    {"Cube root","cbrt(x)",page_function,NULL,"<strong>cbrt(x)</strong> returns the cube root of <em>x</em>. It is equivalent to <em>x^(1/3)</em>."},
    {"Exponent","exp(x)",page_function,NULL,"<strong>exp(x)</strong> returns <em>e^x</em>. It is the inverse of <help>ln</help>"},
    {"Natural log","ln(x)",page_function,"logarithm","<strong>ln(x)</strong> returns the natural logarithm of <em>x</em>. It is equivalent to <em>log(x,e)</em>. <strong>ln</strong> is the inverse function of <help>exp</help>"},
    {"Logarithm 10","logten(x)",page_function,NULL,"<strong>logten(x)</strong> returns the log—base 10 of x, It is equivalent to <em>log(x,10)</em>"},
    {"Logarithm","log(x,b)",page_function,NULL,"<strong>log(x,b)</strong> returns the log of <em>x</em>, base b. It is equivalent to <em>ln(x)/ln(b)</em>."},
    {"Factorial","fact(x)",page_function,NULL,"<strong>fact(x)</strong> returns the factorial of <em>x</em>. The notation <em>x!</em> is not supported. The factorial of negative integers is not defined."},
    {"Sign","sgn(x)",page_function,"step","<strong>sgn(x)</strong> returns the sign of <em>x</em>. More specifically, <em>sgn(x)</em> is equivalent to <em>x/abs(x)</em>."},
    {"Absolute value","abs(x)",page_function,NULL,"<strong>abs(x)</strong> returns the absolute value of <em>x</em>. This is defined as the distance of the value from zero. Vectors and complex numbers are accepted."},
    {"Argument","arg(x)",page_function,"angle,imaginary","<strong>arg(x)</strong> returns the argument of <em>x</em>. The argument of a complex number is the angle from the positive real axis.<br><h3>Examples:</h3><br><syntax>arg(1)</syntax> = <syntax>0</syntax><br><syntax>arg(i)</syntax> = <syntax>pi/2</syntax><br><syntax>arg(-2)</syntax> = <syntax>pi</syntax>"},
    #pragma endregion
    #pragma region Rounding and Components
    {"Round","round(x)",page_function,NULL,"<strong>round(x)</strong> returns x rounded to the nearest integer. If x is an integer plus .5, x is rounded up, in that case with a negative number, it is rounded down. For imaginary numbers and vectors, each component is treated separately."},
    {"Floor","floor(x)",page_function,"round down","<strong>floor(x)</strong> returns <em>x</em> rounded down to the nearest integer. <em>floor</em> rounds towards negative infinity, so negative numbers are different. For imaginary numbers and vectors, each component is treated separately."},
    {"Ceiling","ceil(x)",page_function,"round up","<strong>ceil(x)</strong> returns <em>x</em> rounded up to the nearest integer. <em>ceil</em> rounds towards positive infinity, so negative numbers are treated differently. For imaginary numbers and vectors, each component is treated separately."},
    {"Get real","getr(z)",page_function,"component","<strong>getr(x)</strong> returns <em>x</em> stripped of the imaginary and unit components. For vectors, a vector is returned with <em>getr</em> called on each element."},
    {"Get imaginary","geti(z)",page_function,"component","<strong>geti(x)</strong> returns <em>x</em> stripped of the real and unit components. For vectors, a vector is returned with <em>geti</em> called on each element."},
    {"Get unit","getu(z)",page_function,"component","<strong>getu(x)</strong> returns <em>x</em> stripped of the numeric components. That means it returns <em>1</em> multiplied by <em>x</em>'s <help>unit</help>. For vectors, a vector is returned with <em>getu</em> called on each element."},
    #pragma endregion
    #pragma region Comparison
    {"Greater than","gt(a,b)",page_function,"comparison,>","<strong>grthan(a,b)</strong> returns <em>1</em> if the real component of <em>a</em> is greater than the real component of <em>b</em>. For vectors, a vector is returned with the result of each comparison.<br>Example:<br><syntax>grthan(&lt;5,10&gt;,&lt;10,5&gt;)</syntax> = <syntax>&lt;0,1&gt;</syntax>"},
    {"Equal","equal(a,b)",page_function,"comparison,=,==","<strong>equal(a,b)</strong> returns <em>1</em> only if both are exactly equal. Otherwise <em>equal</em> returns 0."},
    #pragma endregion
    {"Minimum","min(a,b)",page_function,"comparison","<strong>min(a,b)</strong> returns the smaller of the two inputs. For vectors, a new vector is returned with <em>min</em> called on each element."},
    {"Maximum","max(a,b)",page_function,"comparison","<strong>max(a,b)</strong> returns the larger of the two inputs. For vectors, a new vector is returned with <em>max</em> called on each element."},
    {"Linear interpolation","lerp(a,b,d)",page_function,NULL,"<strong>lerp(a,b,d)</strong> returns <em>a</em> if <em>d</em> equals 0 and <em>b</em> if <em>d</em> equals 1. The rest of the values of <em>d</em> are <em>interpolated</em>.<br>Examples:<br><syntax>lerp(0,1,0.5)</syntax> = <syntax>0.5</syntax><br><syntax>lerp(&lt;10,5&gt;,&lt;1,2&gt;,1/3)</syntax> = <syntax>&lt;7,4&gt;</syntax>"},
    {"Distance","dist(a,b)",page_function,NULL,"<strong>dist(a,b)</strong> returns the distance between <em>a</em> and <em>b</em>. Complex numbers are treated as an extra dimension."},
    #pragma region Binary Operations
    {"Binary not","not(a)",page_function,NULL,"<strong>not(a)</strong> returns the binary not of <em>a</em>. First, <em>a</em> is rounded down towards zero, then the binary not operation is applied. This operation is equivalent to <em>-a-1</em> for integer <em>a</em>. The complex component is ignored."},
    {"Binary and","and(a,b)",page_function,NULL,"<strong>and(a,b)</strong> returns the binary and of <em>a</em> and <em>b</em>. The integers are in two's complement, so <em>and(-1,x)</em> returns <em>x</em>. The complex component is calculated separately, and vectors are not supported."},
    {"Binary or","or(a,b)",page_function,NULL,"<strong>or(a,b)</strong> returns the binary or of <em>a</em> and <em>b</em>. The integers are in two's complement, so <em>or(-1,x)</em> returns <em>-1</em>. The complex component is calculated separately, and vectors are not supported."},
    {"Binary xor","xor(a,b)",page_function,NULL,"<strong>xor(a,b)</strong> returns the binary <strong>exclusive or</strong> of <em>a</em> and <em>b</em>. The integers are in two's complement, so <em>xor(-1,x)</em> returns <em>not(x)</em>. <br>Examples:<br><em>xor(0,0) = 0<br>xor(1,0) = 1<br>xor(0,1) = 1<br>xor(1,1) = 0<br></em>The complex component is calculated separately, and vectors are not supported."},
    {"Binary left shift","ls(x,n)",page_function,NULL,"<strong>ls(x,n)</strong> returns <em>x</em> left-shifted <em>n</em> times. It is equivalent to <syntax>floor(x)*2^floor(n)</syntax>. For negative numbers, the sign is retained."},
    {"Binary right shift","rs(x,n)",page_function,NULL,"<strong>rs(x,n)</strong> returns <em>x</em> right-shifted <em>n</em> times. It is equivalent to <syntax>floor(floor(x)/2^floor(n))</syntax>. For negative numbers, the sign is retained."},
    #pragma endregion
    #pragma region Constants
    {"Pi","pi",page_function,NULL,"<strong>Pi</strong> is the ratio between the circumference of a circle and its diameter. It is equal to approximately 3.141592653589."},
    {"Golden ratio","phi",page_function,"ϕ,φ","<strong>Phi</strong> (has the symbol φ or ϕ) is the golden ratio. It is equal to <syntax>(1+sqrt(5))/2</syntax>. Phi is the only constant where (a+b)/a = a/b. More aptly: <syntax>phi</syntax> = <syntax>1+1/phi</syntax>. The value of phi is approximately 1.618033988749."},
    {"Euler's number","e",page_function,NULL,"<strong>e</strong> is Euler's number. It is the base in <help>exp</help>, and the base in <help>ln</help>. <em>e</em> has many applications in the real world, and its value is approximately 2.718281828459."},
    {"Previous answer","ans",page_function,NULL,"<strong>ans</strong> returns the previous value that was calculated. it is equivalent to <syntax><help>hist</help>(-1)</syntax>."},
    {"History","hist(n)",page_function,"previous answer","<strong>hist(n)</strong> returns the <em>n</em>th value in the history. For negative <em>n</em>, <em>hist</em> returns <syntax>hist(<help>histnum</help>+n)</syntax>. For example, <syntax>hist(-1)</syntax> returns the previous value. Anything outside of the range of the calculation history will give an error."},
    {"History count", "histnum",page_function,NULL,"<strong>histnum</strong> returns the number of items in the history. <em>histnum</em> is also equal to the history index of the current calculation, so calling <em>histnum</em> as the first calculation will return zero."},
    {"Random","rand",page_function,NULL,"<strong>rand</strong> returns a random number between <em>0</em> and <em>1</em>"},
    #pragma endregion
    #pragma region Vector Functions
    {"Run function","run(func,...)",page_function,"anonymous,evaluate","<strong>run(func,...)</strong> returns the result of func run with the next inputs. For example <syntax>run(n=>(n+1),3)</syntax> returns 4. The first input of run must be an anonymous function."},
    {"Sum","sum(func,start,end,step)",page_function,"summation,addition","<strong>sum(func,start,end,step)</strong> returns the summation of a series. It is exactly identical to this script:<br><strong> out = 0<br>for(i=start;i&lt;end;i+=step) out+=run(func,i);<br>return out;<br></strong>This function supports vectors as outputs."},
    {"Product","product(func,start,end,step)",page_function,"multiplication","<strong>product(func,start,end,step)</strong> returns the product of a series. It is exactly identical to this script:<br><strong> out = 1<br>for(i=start;i&lt;end;i+=step) out*=run(func,i);<br>return out;<br></strong>This function supports vectors as outputs."},
    {"Width", "width(vec)",page_function,"vector,matrix","<strong>width(vec)</strong> returns the width of the vector <em>vec</em>. <em>width</em> returns 1 if <em>vec</em> is not a vector."},
    {"Height", "height(vec)",page_function,"vector,matrix","<strong>height(vec)</strong> returns the height of the vector <em>vec</em>. <em>height</em> returns 1 if <em>vec</em> is not a vector."},
    {"Length", "length(vec)",page_function,"element count,vector,matrix","<strong>length(vec)</strong> returns the total number of elements in <em>vec</em>. It is equivalent to <em><help>height</help>(vec)*<help>length</help>(vec)</em>. For non-vector values, <em>length</em> returns 1."},
    {"Get element","ge(vec,x,y)",page_function,"index,vector,matrix","<strong>ge(vec,x,<em>y</em>)</strong> returns the element in vec at position <em>x,y</em>. If no <em>y</em> is supplied, it compresses <em>vec</em> into a one-dimensional list. The arguments <em>x</em> and <em>y</em> start at zero, so <syntax>ge(vec,0,0)</syntax> will return the top left corner of <em>vec</em>."},
    {"Vector fill","fill(func,width,height)",page_function,NULL,"<strong>fill(func,width,<em>height</em>)</strong> returns a vector filled with the expression or constant <em>func</em>. <em>func</em> can be either a constant, or an <help>anonymous function</help> with two inputs. <em>height</em> is optional and will default to 1.<br>Examples:<br><syntax>fill(1,2,3)</syntax> = <syntax>&lt;1,1;1,1;1,1&gt;</syntax><br><syntax>fill((x,y)=>(x*y),3,3)</syntax> = <syntax>&lt;0,0,0;0,1,2;0,2,4&gt;</syntax>"},
    {"Vector map","map(vec,func)",page_function,NULL,"<strong>map(vec,func)</strong> will return a new vector where each element of <em>vec</em> has passed through the <help>anonymous function</help> <em>func</em>. <em>func</em> can have up to five inputs, but only one is required.<br>Func inputs:<br><ol><li>v - the value of the cell</li><li>x - the x coordinate of the cell</li><li>y - the y coordinate</li><li>i - the index (v=ge(vec,i))</li><li>vec - the entire vector</li></ol><br>Examples:<br><syntax>map(&lt;1,4,2&gt;,n=>(n+1))</syntax> = <syntax>&lt;2,5,3&gt;</syntax>."},
    {"Determinant","det(mat)",page_function,"matrix","<strong>det(mat)</strong> returns the determinant of <em>mat</em> as if it was a <help>matrix</help>. Only square matrices (where width and height are equal) are accepted."},
    {"Transpose","transpose(mat)",page_function,"matrix","<strong>transpose(at)</strong> will return <em>mat</em> with the cells transposed across the x=y axis. More aptly, this returns a <help>vector</help> with the x and y axis swaped.<br>Examples:<br><syntax>transpose(&lt;1,2&gt;)</syntax> = <syntax>&lt;1;2&gt;</syntax><br><syntax>transpose(&lt;1,2;3,4&gt;)</syntax> = <syntax>&lt;1,3;2,4&gt;</syntax><br>Notice how any values on the diagonal axis do not move."},
    {"Matrix multiplication","mat_mult(a,b)",page_function,"multiply","<strong>mat_mult(a,b)</strong> returns the <help>matrix</help> multiplication of <em>a</em> and <em>b</em>. The width of <em>a</em> must equal the height of <em>b</em>. The result with have the height of <em>a</em> and the width of <em>b</em>. Matrix multiplication is not commutative."},
    {"Matix inverse", "mat_inv(mat)",page_function,"divide","<strong>mat_inv(mat)</strong> returns the inverse of <em>mat</em> as a matrix."},
    #pragma endregion
    #pragma endregion
    #pragma region Units
    {"Meter","[m]",page_unit,"metre,length,distance","<syntax>[m]</syntax> is the metric unit of length known as the meter, or metre. The meter supports metric prefixes for things like <em>km</em> or <em>cm</em>."},
    {"Kilogram","[kg]",page_unit,"mass,weight,kilogramme","<syntax>[kg]</syntax>, or kilogram, is the metric unit of mass. Despite having the kilo prefix, the kilogram is the <help>base unit</help>. The gram, or <syntax>[g]</syntax> is translated to <syntax>0.001[kg]</syntax>. The gram supports all <help>metric prefixes</help>."},
    {"Second","[s]",page_unit,"time,duration","<syntax>[s]</syntax>, or second, is the metric unit of time. Originally, the second was defined as a fraction of a day. Now, it is defined based on atomic clocks. The second supports all <help>metric prefixes</help>."},
    {"Ampere","[A]",page_unit,"electrical flow","<syntax>[A]</syntax>, or Amp, is the metric unit of electrical flow. It is one of the seven <help>base units</help>. The amp is defined as approximately 6.24 quintillion elementary charges per second. The Ampere supports all <help>metric prefixes</help>."},
    {"Kelvin","[K]",page_unit,"temperature","<syntax>[K]</syntax>, or Kelvin, is the metric unit of temperature. The Kelvin scale is equal in spacing to the Celsius scale, except 0° K is equal to absolute zero. The Kelvin unit supports all <help>metric prefixes</help>."},
    {"Mole","[mol]",page_unit,"substance","<syntax>[mol]</syntax>, or mole, is the metric unit of substance. It is defined as 6.02214076×10<sup>23</sup> particles. The mole supports all <help>metric prefixes</help>."},
    {"Currency","[$]",page_unit,"dollar,pound,euro","<syntax>[$]</syntax>, or dollar, is the non-standard <help>base unit</help> unit of currency. The dollar unit is not a specific type of currency, so it does not specify an exact quantity."},
    {"Bit","[b]",page_unit,"information","<syntax>[b]</syntax>, or bit, is an extended <help>base unit</help>. It is defined as the information required to describe a situation with 2 possiblities. The bit supports all <help>metric prefixes</help>."},
    {"Byte","[B]",page_unit,"information","<syntax>[B]</syntax>, or byte, is equal to 8 <a href='javascript:openHelp(\"b\")'>bits</a>. It supports all metric prefixes."},
    {"Bits per second","[bps]",page_unit,"information flow","<syntax>[bps]</syntax>, or <help title='b'>bits</help> per <help title='s'>second</help>, is a unit of information flow. <syntax>[bps]</syntax> supports all <help>metric prefixes</help>, allowing for: kbps (kilobits per second), mbps (megabits per second), and others."},
    {"Bytes per second","[Bps]",page_unit,"information flow","<syntax>[Bps]</syntax>, or <help title='B'>bytes</help> per <help title='s'>second</help> (not to be confused with <help title='bps'>bits per second</help>, is a unit of information flow equal to 8 bits per second. Bytes per second supports all <help>metric prefixes</help>."},
    {"Joule","[J]",page_unit,"energy","<syntax>[J]</syntax>, or joule, is the metric unit of energy. It is equal to the force of one <help title='N'>Newton</help> applied over one <help title='m'>meter</help>. Its base units are m<sup>2</sup>*kg*s<sup>-2</sup>. Joule supports all <help>metric prefixes</help>."},
    {"Watt","[W]",page_unit,"power","<syntax>[W]</syntax>, or watt, is the metric unit of power. It is equal to one <help title='J'>joule</help> per <help title='s'>second</help>. The base units of the watt are: m<sup>2</sup>*kg*s<sup>-3</sup>. The watt supports all <help>metric prefixes</help>."},
    {"Volt","[V]",page_unit,"electric potential","<syntax>[V]</syntax>, or volt, is the metric unit of electric potential. It is equal to both a <help title='J'>joule</help> per <help title='C'>coloumb</help>, and a <help title='W'>watt</help> per <help title='A'>amp</help>. The base units of the volt are: m<sup>2</sup>*kg*s<sup>-3</sup>*A<sup>-1</sup>. The volt supports all <help>metric prefixes</help>."},
    {"Ohm","[ohm]",page_unit,"electric resistance","<syntax>[ohm]</syntax> is the unit of electric resistance. The symbol for the ohm is Ω, but it is impractical to type. The ohm is equal to one <help title='V'>volt</help> per <help title='A'>amp</help>. The base units of the ohm are: m<sup>2</sup>*kg*s<sup>-3</sup>*A<sup>-2</sup>. The ohm supports all <help>metric prefixes</help>."},
    {"Henry","[H]",page_unit,"electric inductance","<syntax>[H]</syntax>, or henry, is the metric unit of inductance. It is equal to one <help title='Wb'>Weber</help> per <help title='A'>Amp</help>, and an Ohm-second. The base units of the henry are: m<sup>2</sup>*kg*s<sup>-2</sup>*A<sup>-2</sup>. The henry supports all <help>metric prefixes</help>."},
    {"Weber","[Wb]",page_unit,"magnetic flux","<syntax>[Wb]</syntax>, or weber, is the metric unit of magnetic flux. The weber is equal to one <help title='J'>joule</help> per <help title='A'>Amp</help>, or <help title='V'>Volt</help>-<help title='s'>second</help>. The base units of the weber are: m<sup>2</sup>*kg*s<sup>-2</sup>*A<sup>-1</sup>. Weber supports all <help>metric prefixes</help>."},
    {"Hertz","[Hz]",page_unit,"frequency","<syntax>[Hz]</syntax>, or hertz, is the metric unit of frequency equal to one per <help title='s'>second</help>. Its base units are: s<sup>-1</sup>. Hertz supports all <help>metric prefixes</help>."},
    {"Siemens","[S]",page_unit,"electric conductance","<syntax>[S]</syntax>, or siemens, is the metric unit of electric conductance. It is equal to the inverse of the <help>ohm</help>, or one <help title='A'>Amp</help> per <help title='V'>volt</help>. Another name for siemens is the mho, which is ohm spelled backwards. The base unit of siemens are: m<sup>-2</sup>*kg<sup>-1</sup>*s<sup>3</sup>*A<sup>2</sup>."},
    {"Farad","[F]",page_unit,"electrical capacitance","<syntax>[F]</syntax>, or farad, is the metric unit of electric capacitance. It is equal to one <help title='C'>coulomb</help> per <help title='V'>volt</help>. Its base units are: m<sup>-2</sup>*kg<sup>-1</sup>*s<sup>4</sup>*A<sup>2</sup>. Farad supports all <help>metric prefixes</help>."},
    {"Tesla","[T]",page_unit,"magnetic induction","<syntax>[T]</syntax>, or Tesla, is the metric unit of magnetic induction. It is equal to one <help title='Wb'>weber</help> per square <help title='m'>meter</help>. Its base units are: kg*s<sup>-2</sup>*A<sup>-1</sup>. Tesla supports all <help>metric prefixes</help>."},
    {"Pascal","[Pa]",page_unit,"pressure","<syntax>[Pa]</syntax>, or pascal, is the metric unit of pressure. It is equal to one <help title='N'>newton</help> per square <help title='m'>meter</help>. Its base units are: m<sup>-1</sup>*kg*s<sup>-2</sup>. Pascal supports all <help>metric prefixes</help>."},
    {"Newton","[N]",page_unit,"force","<syntax>[N]</syntax>, or Newton, is the metric unit of force. It is the force required to accelerate one <help title='kg'>kilogram</help> at one <help title='m'>meter</help> per <help title='s'>second</help> squared. A newton applied over one meter takes a <help title='J'>joule</help> of energy. The base units for the newton are: m*kg*s<sup>-2</sup>. It supports all <help>metric prefixes</help>."},
    {"Sievert","[Sv]",page_unit,"radiation,gray","<syntax>[Sv]</syntax>, or sievert, is metric unit of ionizing radiation. The sievert has identacal base units as the gray (Gy), so the gray is not supported. It is equal to one <help title='J'>joule</help> per <help title='kg'>kilogram</help>. Its base units are: m<sup>2</sup>*s<sup>-2</sup>. The sievert supports all <help>metric prefixes</help>."},
    {"Katal","[kat]",page_unit,"catalytic activity","<syntax>[kat]</syntax>, or katal, is the metric unit of catalytic activity. It is equal to one <help title='mol'>mole</help> per <help title='s'>second</help>. Its base units are: mol*s<sup>-1</sup>."},
    {"Minute","[min]",page_unit,"time","<syntax>[min]</syntax> is a unit of time equal to 60 <help title='s'>seconds</help>."},
    {"Hour","[hr]",page_unit,"time","<syntax>[hr]</syntax> is a unit of time equal to 60 <help title='min'>minutes</help>, or 3600 <help title='s'>seconds</help>."},
    {"Day","[day]",page_unit,"time","<syntax>[day]</syntax> is a unit of time equal to 24 <help title='hr'>hours</help>, or 86400 <help title='s'>seconds</help>."},
    {"Kilometers per hour","[kph]",page_unit,"speed","<syntax>[kph]</syntax>, or <help title='m'>kilometers</help> per <help title='hr'>hour</help>, is a unit of speed equal to 5/18 meters per second."},
    {"Miles per hour","[mph]",page_unit,"speed","<syntax>[mph]</syntax>, or <help title='mi'>miles</help> per <help title='hr'>hour</help>, is a unit of speed equal to 0.447038888... <help title='m'>meters</help> per <help title='s'>second</help>."},
    {"Speed of sound", "[mach]",page_unit,"speed","<syntax>[mach]</syntax> is the unit of speed equal to 343 <help title='m'>meters</help> per <help title='s'>second</help>. This is approximately the speed of sound in 20° C air."},
    {"Speed of light","[c]",page_unit,"speed","<syntax>[c]</syntax> is the unit of speed equal to the speed of light in a vacuum. It is equal to 299,492,458 <help title='m'>meters</help> per <help title='s'>second</help> exactly because the meter is defined by it. The official symbol is capital C, but that was already taken by the <help title='C'>coulomb</help>."},
    {"Foot","[ft]",page_unit,"length,feet","<syntax>[ft]</syntax>, or foot (plural feet), is the imperial unit of length. In 1959, it was defined as exactly equal to 0.3048 <help title='m'>meters</help>. It is equal to a third of a <help title='yd'>yard</help>, or twelve <help title='in'>inches</help>."},
    {"Mile","[mi]",page_unit,"length,distance","<syntax>[mi]</syntax>, or mile, is the imperial unit of a long distance. In 1959, it was agreed to be defined as exactly 1609.344 <help title='m'>meters</help>. It is also equal to 5280 <help title='ft'>feet</help>, or 1760 <help title='yd'>yards</help>."},
    {"Yard","[yd]",page_unit,"length,distance","<syntax>[yd]</syntax>, or yard, is an imperial unit of length. In 1959, it was agreed to be defined as exactly 0.9144 <help title='m'>meters</help>. The yard is also equal to 3 <help title='ft'>feet</help>."},
    {"Inch","[in]",page_unit,"length,distance","<syntax>[in]</syntax>, or inch, is an imperial unit of length originally defined as 1/12 <help title='ft'>feet</help>. In 1959, it was agreed to be defined as 0.0254 <help title='m'>meters</help>."},
    {"Nautical mile","[nmi]",page_unit,"length,distance","<syntax>[nmi]</syntax>, or nautical mile is defined as exactly 1852 <help title='m'>meters</help>."},
    {"Parsec","[pc]",page_unit,"length,distance","<strong>[pc]</strong>, or parsec, is defined as exactly 3.0857 * 10<sup>16</sup> <help title='m'>meters</help>. It was originally defined as the distance between the sun and a star if that star had a parallax of one arc second. Therefore, it is derived from the arc second and the astronomical unit. The parsec is equal to about 3.3 light years. The parsec supports all metric prefixes."},
    {"Acre","[acre]",page_unit,"area","<syntax>[acre]</syntax> is an imperial unit of area. It was originally defined as 66 by 660 <help title='ft'>feet</help> (43560 square feet), now it is defined as exactly 4046.8564224 square meters."},
    {"Are","[are]",page_unit,"area,hectare","<syntax>[are]</syntax> is a metric unit of area equal to 100 square meters. A more common unit is the hectare ([hare]), which is equal to 10000 m<sup>2</sup> <syntax>(100 [m] * 100 [m])</syntax>. The are supports all <help>metric prefixes</help>."},
    {"Carat","[ct]",page_unit,"mass,weight","<syntax>[ct]</syntax>, or carat, is a metric unit of mass used for measuring precious metals. It is equal to exactly 0.2 <help title='kg'>gram</help>."},
    {"Stone","[st]",page_unit,"mass,weight","<syntax>[st]</syntax>, or stone, is an imperial unit of mass equal to 6.35029318 <help title='kg'>kilograms</help>. The exact meaning of 'one stone' varies wildly across culture, so it is not a recommended unit."},
    {"Pound","[lb]",page_unit,"mass,weight,libra","<syntax>[lb]</syntax>, or pound, is an imperial unit of mass. It is defined as 0.45359237 <help title='kg'>kilogram</help>."},
    {"Ounce","[oz]",page_unit,"mass,weight","<syntax>[oz]</syntax>, or ounce, is an imperial unit of mass. Historically, the definition of ounce was varied, but today it is commonly accepted to be 28.3 <help title='kg'>grams</help>, or 1/16 <help title='lb'>pounds</help>."},
    {"Tonne","[tn]",page_unit,"mass,weight","<syntax>[tn]</syntax>, or tonne, is a metric unit used for large masses. It is exactly equal to 1000 <help title='kg'>kilograms</help>, this makes it equal to the Mg (megagram)."},
    {"Gallon","[gallon]",page_unit,"volume","<syntax>[gallon]</syntax> is an imperial unit of volume. It is equal to 0.00454609 cubic meters, or 128 <help title='floz'>fluid ounces</help>."},
    {"Cup","[cup]",page_unit,"volume","<syntax>[cup]</syntax> is an imperial unit of volume. It is equal to 0.0002365882365 cubic meters, or 8 <help title='floz'>fluid ounces</help>."},
    {"Fluid ounce","[floz]",page_unit,"volume","<syntax>[floz]</syntax>, or fluid ounce is an imperial unit of volume. It is equal to 0.0000295735295625 cubic meters, or 29.5 mL."},
    {"Tablespoon","[tbsp]",page_unit,"volume","<syntax>[tbsp]</syntax>, or tablespoon is an imperial unit of volume. It is equal to 0.00001478676478125 cubic meters, 14.7 mL, 0.5 <help title='floz'>fluid ounces</help>, or 3 <help title='tsp'>teaspoons</help>."},
    {"Teaspoon","[tsp]",page_unit,"volume","<syntax>[tsp]</syntax> is an imperial unit of volume. It is equal to 0.000000492892159375 cubic meters,  0.492 mL, 1/6 <help title='floz'>fluid ounces</help>, or 1/3 <help title='tbsp'>tablespoon</help>."},
    {"Amp hour","[Ah]",page_unit,"electric charge","<syntax>[Ah]</syntax>, or Amp hour, is a metric unit of electric charge equal to 3600 <help title='C'>Coulombs</help>. A coulomb is equal to an Amp second. Amp hour supports all <help>metric prefixes</help>."},
    {"Watt hour","[Wh]",page_unit,"energy","<syntax>[Wh]</syntax>, or watt hour, is a metric unit of energy equal to 3600 <help title='J'>joules</help>. A joule is a watt second. Watt hour supports all <help>metric prefixes</help>."},
    {"Electron volt","[eV]",page_unit,"electric charge","<syntax>[eV]</syntax>, or electronvolt, is the unit of charge in an electron. It is equal to 1.602 176 634 * 10<sup>-19</sup> <help title='C'>Coulombs</help>. Electronvolt supports all <help>metric prefixes</help>."},
    {"Atmosphere","[atm]",page_unit,"pressure","<syntax>[atm]</syntax>, or atmosphere, is the approximate pressure of the Earth's atmosphere at sea level. It is equal to 101352 <help title='Pa'>Pascal</help>."},
    {"Bar","[bar]",page_unit,"pressure","<syntax>[bar]</syntax> is a metric unit of pressure equal to 100000 <help title='pa'>Pascal</help>. Bar supports all metric prefixes."},
    {"Pounds per square inch","[psi]",page_unit,"pressure","<syntax>[psi]</syntax>, or pounds per square inch, is an imperial unit of pressure equal to 6894.75729316836133 <help title='Pa'>Pascal</help>."},
    {"British thermal unit","[btu]",page_unit,"energy","<syntax>[btu]</syntax>, or British thermal unit, is an imperial unit of energy equal to 1054.3503 <help title='J'>joules</help>. The original meaning was the energy required to heat a pound of maximum density water by one degree Fahrenheit."},
    #pragma endregion
};
