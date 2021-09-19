OUTPUT_FORMAT (elf64-x86-64)
ENTRY (_start)

SECTIONS
{
  . = 1M;

  __kernel_vma = 0xfffffe0000000000;
  __kernel_start = .;

  .multiboot.data :
  {
    *(.multiboot.data)
  }

  .multiboot.bss :
  {
    *(.multiboot.bss)
  }

  .multiboot.text :
  {
    *(.multiboot.text)
  }

  .multiboot.rodata :
  {
    *(.multiboot.rodata)
  }

  . += __kernel_vma;

  .text ALIGN (4K) : AT (ADDR (.text) - __kernel_vma)
  {
    *(.text)
  }

  .rodata ALIGN (4K) : AT (ADDR (.rodata) - __kernel_vma)
  {
    *(.rodata)
  }

  .data ALIGN (4K) : AT (ADDR (.data) - __kernel_vma)
  {
    *(.data)
  }

  .bss ALIGN (4K) : AT (ADDR (.bss) - __kernel_vma)
  {
    *(COMMON)
    *(.bss)
  }

  __kernel_end = .;

  /DISCARD/ :
  {
    *(.comment)
  }
}
