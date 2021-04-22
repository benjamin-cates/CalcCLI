#ifndef HELP_H
#define HELP_H 1
struct HelpPage {
    //Name like "Watt" or "Add" or "List"
    const char* name;
    //Symbol like "[W]" or "add(a,b)" or "-ls", guides and basic information have symbol NULL
    const char* symbol;
    //see pageTypes[]
    int type;
    //tags (also known as aliases). For example, the add page might have the tags: addition, summation, and plus
    const char* tags;
    //Page content
    const char* content;
};
#define helpPageCount 171
extern const struct HelpPage pages[helpPageCount];
//Returns a JSON parsable string of the help page. Return value must be freed
char* helpPageToJSON(struct HelpPage page);
//Returns a negative one terminated list of help pages, sorted by
int* searchHelpPages(const char* s);
//Removes all HTML tags from in and replaces ampersand escapes with their proper characters (only &nbsp; &gt; and &lt;)
void removeHTML(char* in);
//List of page type names
extern const char* pageTypes[];
//Returns generated page
char* getGeneratedPage(struct HelpPage page);
#endif