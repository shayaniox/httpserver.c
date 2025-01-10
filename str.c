#include "str.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// It's assumed that *s* is null-terminated char string
string *newstr(const char *s)
{
    string *str = malloc(sizeof(string));
    str->len = strlen(s);
    str->cap = str->len;
    str->data = malloc(str->len + 1);
    memcpy(str->data, s, str->len);
    str->data[str->len] = '\0';
    return str;
}

int extend(string *s, size_t size)
{
    size_t n = s->cap + size;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
#if (UINT_MAX == 0xffffffff)
    n |= n >> 16;
#elif (UINT_MAX > 0xffffffffUL)
    n |= n >> 32;
#endif
    n++;
    s->cap = n;
    s->data = realloc(s->data, s->cap);
    if (s->data == NULL) {
        return -1;
    }
    return 0;
}

int append(string *s, const char *cs)
{
    if (s == NULL) return -1;
    size_t l = strlen(cs);
    if (s->len + l >= s->cap)
        if (extend(s, l) < 0) return -1;
    memcpy(s->data + s->len, cs, l);
    s->len += l;
    s->data[s->len] = '\0';
    return 0;
}

void freestr(string *str)
{
    if (str == NULL) return;
    if (str->data) free(str->data);
    free(str);
}

char *strtocs(string *str)
{
    char *cs = malloc(str->len + 1);
    memcpy(cs, str->data, str->len);
    return cs;
}

void printstr(string *str)
{
    if (str == NULL) return;
    printf("{\n  data: '%.*s'\n  len: %d\n  cap: %d\n}\n", str->len, str->data, str->len, str->cap);
}
