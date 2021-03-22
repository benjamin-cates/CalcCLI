//units.c contains unit data, unit functions, and unit name recognition
#include "general.h"
//Header info for units.c is in general.h
//IMPORTANT: this array must be in alphabetical order (according to ASCII, so that includes case), otherwise the binary search method will not find it. All units are automatically tested in Test.c
const unitStandard unitList[] = {
    {"$", 1.0, 0x1000000000000},       //Dollar
    {"A", -1.0, 0x1000000},            //Ampere
    {"Ah",-3600.0,0x01010000},
    {"B", -8.0, 0x100000000000000},    //Byte
    {"Bps", -8.0, 0x100000000FF0000}, //Bytes per second
    {"F", -1.0, 0x204FFFE},           //Farad
    {"H", -1.0, 0xFEFE0102},          //Henry
    {"Hz", -1.0, 0xFF0000},           //Hertz
    {"J", -1.0, 0xFE0102},            //Joule
    {"K", -1.0, 0x100000000},          //Kelvin
    {"N", -1.0, 0xFE0101},            //Newton
    {"Pa", -1.0, 0xFE01FF},           //Pascal
    {"S", -1.0, 0x203FFFE},           //Siemens
    {"Sv", -1.0, 0xFE0002},           //Sievert
    {"T", -1.0, 0xFFFE0100},          //Tesla
    {"V", -1.0, 0x01FD0102},          //Volt
    {"W", -1.0, 0xFD0102},            //Watt
    {"Wb", -1.0, 0xFFFE0102},         //Weber
    {"Wh",-3600.0,0xFE0102},
    {"acre",4046.8564224,0x02},
    {"are",-1000.0,0x02},
    {"atm",101352.0,0xFE01FF},
    {"b", -1.0, 0x100000000000000},    //Bit
    {"bar",-100000.0,0xFE01FF},
    {"bps", -1.0, 0x100000000FF0000}, //Bits per second
    {"btu",1054.3503,0xFE0102},
    {"c",299792458.0,0xFF0001},
    {"ct",0.0002,0x0100},
    {"cup",0.0002365882365,0x03},
    {"day",86400.0,0x010000},
    {"eV",-0.0000000000000000001602176620898,0xFE0102},
    {"floz",0.0000295735295625,0x03},
    {"ft",0.3048,0x01},
    {"g", -0.001, 0x100},            //Gram
    {"gallon",0.00454609,0x03},
    {"hr",3600.0,0x010000},
    {"in",0.0254,0x01},
    {"kat", -1.0, 0x10000FF0000},     //Katal
    {"kg", 1.0, 0x100},                //Kilogram
    {"kph",0.277777777777777,0xFF0001},
    {"lb",0.45359237,0x0100},
    {"m", -1.0, 0x1},                  //Meter
    {"mach",340.3,0xFF0001},
    {"mi",1609.344,0x01},
    {"min",60.0,0x010000},
    {"mol", -1.0, 0x10000000000},      //Mole
    {"mph",0.4470388888888888,0xFF0001},
    {"nmi",1852.0,0x01},
    {"ohm", -1.0, 0xFEFD0102},        //Ohm
    {"oz",0.028349523125,0x0100},
    {"pc",-30857000000000000.0,0x01},
    {"psi",6894.75729316836133,0xFE01FF},
    {"s", -1.0, 0x10000},              //Second
    {"st",6.35029318,0x0100},
    {"tbsp",0.00001478676478125,0x03},
    {"tn",1000.0,0x0100},
    {"tsp",0.000000492892159375,0x03},
    {"yd",0.9144,0x01},
};
//The metric prefixes ordered by size are: yocto(1e-24), zepto(1e-21, atto(1e-18), fempto(1e-15), pico(1e-12), nano(1e-9), micro(1e-6), milli(1e-3), centi(1e-2), hecto(1e2), kilo(1e3), Mega(1e6), Giga(1e9), Tera(1e12), Peta(1e15), Exa(1e18), Zetta(1e21), Yotta(1e24)
const char metricNums[] = "yzafpnumchkMGTPEZY";
const double metricNumValues[] = { 1e-24, 1e-21, 1e-18, 1e-15, 1e-12, 1e-9, 1e-6, 1e-3, 0.01, 100, 1000, 1e6, 1e9, 1e12, 1e15, 1e18, 1e21, 1e24 };
//The base units are: meter, kilogram, second, amp, kelvin, mole, dollar, bit
const char* baseUnits[] = { "m", "kg", "s", "A", "K", "mol", "$", "bit" };
//Internal method that uses a binary search to find the unit id, this is not in the header file, so use getUnitName() for other files
int getUnitId(const char* name, int pos, int count) {
    if(count < 3) {
        if(strcmp(unitList[pos].name, name) == 0) return pos;
        if(strcmp(unitList[pos + 1].name, name) == 0) return pos + 1;
        return -1;
    }
    int half = count / 2;
    int compare = strcmp(unitList[pos + half].name, name);
    if(compare == 0) return pos + half;
    else if(compare < 0) return getUnitId(name, pos + half, count - half);
    else return getUnitId(name, pos, half);
}
Number getUnitName(const char* name) {
    int match = getUnitId(name, 0, unitCount);
    if(match != -1) return newNum(fabs(unitList[match].multiplier), match, unitList[match].baseUnits);
    //Try with metric prefixes
    int useMetric = -1;
    int i;
    for(i = 0; i < metricCount; i++) if(name[0] == metricNums[i]) {
        useMetric = i;
        break;
    }
    if(useMetric != -1) {
        match = getUnitId(name + 1, 0, unitCount);
        if(match != -1) return newNum(fabs(unitList[match].multiplier) * metricNumValues[useMetric], match, unitList[match].baseUnits);
    }
    //Else return no result
    return newNum(1, -1, 0);
}
char* toStringUnit(unit_t unit) {
    if(unit == 0)
        return NULL;
    int i;
    //Find applicable metric unit
    for(i = 0; i < unitCount; i++) {
        if(unitList[i].multiplier == -1 && unit == unitList[i].baseUnits) {
            //Copy its name into a dynamic memory address
            char* out = calloc(strlen(unitList[i].name), 1);
            if(out == NULL) { error(mallocError);return NULL; }
            strcat(out, unitList[i].name);
            return out;
        }
    }
    //Else generate a custom string
    char* out = calloc(54, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    bool mult = false;
    for(i = 0; i < 8; i++) {
        char val = (unit >> (i * 8)) & 255;
        if(val != 0) {
            if(mult)
                strcat(out, "*");
            strcat(out, baseUnits[i]);
            if(val != 1) {
                strcat(out, "^");
                char buffer[5];
                snprintf(buffer, 4, "%d", val);
                strcat(out, buffer);
            }
            mult = true;
        }
    }
    return out;
}
unit_t unitInteract(unit_t one, unit_t two, char op, double twor) {
    int i;
    if(op == '^') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(char)((float)o * twor)) << (i * 8);
        }
        return out;
    }
    if(two == 0)
        return one;
    if(op == '/') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            char t = (two >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(o - t)) << (i * 8);
        }
        return out;
    }
    if(one == 0)
        return two;
    if(op == '+') {
        if(one != two) {
            error("Cannot add different units", NULL);
        }
        return one;
    }
    if(op == '*') {
        unit_t out = 0;
        for(i = 0; i < 8; i++) {
            char o = (one >> (i * 8)) & 255;
            char t = (two >> (i * 8)) & 255;
            out |= ((unit_t)(unsigned char)(o + t)) << (i * 8);
        }
        return out;
    }
    return 0;
}