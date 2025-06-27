#pragma once

#include "stdio.h"

void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOut,
                         uint32_t *remainderOut);
void _cdecl x86_Video_WriteCharTeletype(char c, unsigned char page);
bool _cdecl x86_Disk_Reset(uint8_t drive);
bool _cdecl x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t sector, uint16_t head,
                          uint8_t count, void far *out);
bool _cdecl x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveOut, uint16_t *cylinder,
                                    uint16_t *sector, uint16_t *head);
