#ifndef MISC_H
#define MISC_H 1
#include "general.h"
struct Preference {
    const char* name;
    const Value defaultVal;
    Value current;
};
//Returns 1 on success, 0 on fail
int setPreference(char* name, Value val,bool save);
Value getPreference(char* name);
void savePreferences();
void loadPreferences();
void updatePreference(int id);
#define preferenceCount 5
extern struct Preference preferences[preferenceCount];
extern const bool allowedPreferences[preferenceCount];
#endif