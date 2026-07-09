ENTRY(_init)

SECTIONS
{
    . = __BOOT__;

    __kernel_start = .;

    .init :
    {
        __init_start = .;
		KEEP(*(.init))
        *(.init.*)
        __init_end = .;
    }

    .text :
    {
        __text_start = .;
        *(.text)
        *(.text.*)
        __text_end = .;
    }

    .rodata :
    {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        __rodata_end = .;
    }

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    }

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

	.payload :
    {
        KEEP(*(.payload))
    }

    __kernel_end = .;
}
