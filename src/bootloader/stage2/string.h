#pragma once

#include "stdio.h"

const char *StrChr(const char *str, char c);
char *StrCpy(char *des, const char *src);
unsigned StrLen(const char *str);

bool IsLower(char chr);
char ToUpper(char chr);
