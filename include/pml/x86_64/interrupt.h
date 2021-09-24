/* interrupt.h -- This file is part of PML.
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

#ifndef __PML_INTERRUPT_H
#define __PML_INTERRUPT_H

#include <pml/cdefs.h>

#define PIC_8259_MASTER_COMMAND 0x20
#define PIC_8259_MASTER_DATA    0x21
#define PIC_8259_SLAVE_COMMAND  0xa0
#define PIC_8259_SLAVE_DATA     0xa1

#define PIC_8259_EOI            0x20

#define IDT_GATE_TASK           0x05
#define IDT_GATE_INT            0x0e
#define IDT_GATE_TRAP           0x0f

#define IDT_ATTR_PRESENT        (1 << 7)
#define IDT_SIZE                256

struct idt_entry
{
  uint16_t offset_low;
  uint16_t selector;
  unsigned char ist;
  unsigned char type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t reserved;
};

struct idt_ptr
{
  uint16_t size;
  struct idt_entry *addr;
} __packed;

static inline void
load_idt (const struct idt_ptr *ptr)
{
  __asm__ volatile ("lidt (%0)" :: "r" (ptr));
}

__BEGIN_DECLS

void pic_8259_remap (void);
void pic_8259_eoi (unsigned char irq);
void set_int_vector (unsigned char num, void *addr, unsigned char privilege,
		     unsigned char type);
void fill_idt_vectors (void);
void init_idt (void);

__END_DECLS

#endif
