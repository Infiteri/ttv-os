#include "string.h"
#include "stdio.h"

const char *StrChr(const char *str, char c)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == c)
            return str;

        ++str;
    }

    return NULL;
}

char *StrCpy(char *dst, const char *src)
{
    char *dst0 = dst;
    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        dst++;
        src++;
    }

    *dst = '\0';
    return dst0;
}

unsigned StrLen(const char *str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}

bool IsLower(char chr) { return chr >= 'a' && chr <= 'z'; }

char ToUpper(char chr) { return IsLower(chr) ? (chr - 'a' + 'A') : chr; }
