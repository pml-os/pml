/* pic8259.c -- This file is part of PML.
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

/*! @file */

#include <pml/interrupt.h>
#include <pml/io.h>

/*!
 * Remaps the 8259 PIC to serve interrupts 32-47.
 */

void
pic_8259_remap (void)
{
  outb_p (0x11, PIC_8259_MASTER_COMMAND);
  outb_p (0x11, PIC_8259_SLAVE_COMMAND);
  outb_p (0x20, PIC_8259_MASTER_DATA);
  outb_p (0x28, PIC_8259_SLAVE_DATA);
  outb_p (4, PIC_8259_MASTER_DATA);
  outb_p (2, PIC_8259_SLAVE_DATA);
  outb_p (1, PIC_8259_MASTER_DATA);
  outb_p (1, PIC_8259_SLAVE_DATA);
  outb_p (0, PIC_8259_MASTER_DATA);
  outb_p (0, PIC_8259_SLAVE_DATA);
}

/*!
 * Disables the 8259 PIC by masking all interrupts.
 */

void
pic_8259_disable (void)
{
  outb_p (0xff, PIC_8259_SLAVE_DATA);
  outb_p (0xff, PIC_8259_MASTER_DATA);
}

/*!
 * Signals the 8259 PIC to resume generating interrupts.
 *
 * @param irq the IRQ number of the interrupt
 */

void
pic_8259_eoi (unsigned char irq)
{
  if (irq >= 8)
    outb (PIC_8259_EOI, PIC_8259_SLAVE_COMMAND);
  outb (PIC_8259_EOI, PIC_8259_MASTER_COMMAND);
}

#ifndef USE_APIC

void
int_start (void)
{
}

#endif
