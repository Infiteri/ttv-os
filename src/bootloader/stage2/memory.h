#pragma once

#include "stdio.h"

void far *MemCpy(void far *dst, const void far *src, uint16_t num);
void far *MemSet(void far *ptr, int value, uint16_t num);
int MemCmp(const void far *ptr1, const void far *ptr2, uint16_t num);
