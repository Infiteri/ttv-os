#include "utility.h"

u32 Align(u32 num, u32 align)
{
    if (align == 0)
        return num;

    u32 rem = num % align;
    return (rem > 0) ? (num + align - rem) : num;
}
