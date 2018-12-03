ENTRY(_Reset)

SECTIONS
{
    . = 0x10000; /* 1 MB */
    .text : { startup.o (.vector_table) *(.text) } 
    . = ALIGN(8);
}
