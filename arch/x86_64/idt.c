/* idt.c -- This file is part of PML.
   Copyright (C) 2021 XNSC

   PML is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PML is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PML. If not, see <https://www.gnu.org/licenses/>. */

#include <pml/interrupt.h>
#include <pml/pit.h>

static struct idt_entry idt_table[IDT_SIZE];
static struct idt_ptr idt_ptr;

void
set_int_vector (unsigned char num, void *addr, unsigned char privilege,
		unsigned char type)
{
  uintptr_t offset = (uintptr_t) addr;
  idt_table[num].offset_low = offset & 0xffff;
  idt_table[num].selector = 0x08;
  idt_table[num].ist = 0;
  idt_table[num].type_attr =
    IDT_ATTR_PRESENT | ((privilege & 3) << 5) | (type & 0xf);
  idt_table[num].offset_mid = (offset >> 16) & 0xffff;
  idt_table[num].offset_high = offset >> 32;
  idt_table[num].reserved = 0;
}

void
init_idt (void)
{
  /* Remap the 8259 PIC and setup the PIT timer */
  pic_8259_remap ();
  pit_set_freq (0, 1000);

  /* Fill and load the IDT */
  fill_idt_vectors ();
  idt_ptr.size = sizeof (idt_table) - 1;
  idt_ptr.addr = idt_table;
  load_idt (idt_ptr);
}
