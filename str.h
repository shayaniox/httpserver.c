#ifndef _str_h
#define _str_h

typedef struct string {
    int len;
    int cap;
    unsigned char *data;
} string;

string *newstr(const char *s);
int append(string *s, const char *cs);
void freestr(string *str);
char *strtocs(string *str);

#endif /* ifndef _str_h */
