
/* linker_i686.ld */
OUTPUT_FORMAT("binary")
ENTRY(entry)

SECTIONS
{
    /* Start at address 0x0000 */
    . = 0x0000;

    /* Code section */
    .text :
    {
        *(.entry)   /* Entry point section, if you have one */
        *(.text)    /* All code sections */
    }

    /* Data section */
    .data :
    {
        *(.data)
    }

    /* BSS section (uninitialized data) */
    .bss :
    {
        *(.bss)
        *(COMMON)
    }

    /* Optionally reserve stack space */
    . = ALIGN(4);
    /* Reserve 0x200 bytes for stack */
    . += 0x200;
}
