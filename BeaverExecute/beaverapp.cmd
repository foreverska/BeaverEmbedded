
--retain=g_pfnVectors

#define FLASH_SIZE 0x00040000
#define APP_BASE 0x00001000
#define RAM_BASE 0x20000000

app_base = APP_BASE;

MEMORY
{
    FLASH (RX) : origin = APP_BASE, length = FLASH_SIZE - APP_BASE
    SRAM (RWX) : origin = 0x20000000, length = 0x00008000
}

/* Section allocation in memory */

SECTIONS
{
    .intvecs :   > APP_BASE
    .text   :   > FLASH, ALIGN(0x400)
    .const  :   > FLASH, ALIGN(0x400)
    .cinit  :   > FLASH, ALIGN(0x400)
    .pinit  :   > FLASH, ALIGN(0x400)
    .init_array : > FLASH, ALIGN(0x400)

    .vtable :   > 0x20000000
    .data   :   > SRAM
    .bss    :   > SRAM
    .sysmem :   > SRAM
    .stack  :   > SRAM
}

__STACK_TOP = __stack + 512;
