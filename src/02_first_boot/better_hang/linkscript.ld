ENTRY(_Reset)

SECTIONS
{
    . = 0x0;
    .text : { startup.o (.vector_table) *(.text) } 
    . = ALIGN(8);
}
