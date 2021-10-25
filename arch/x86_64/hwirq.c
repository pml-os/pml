/* hwirq.c -- This file is part of PML.
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
#include <pml/io.h>
#include <pml/kbd.h>

void
int_ps2_keyboard (void)
{
  kbd_recv_key (inb (PS2KBD_PORT_DATA));
  EOI (1);
}

void
int_serial2 (void)
{
  EOI (3);
}

void
int_serial1 (void)
{
  EOI (4);
}

void
int_parallel2 (void)
{
  EOI (5);
}

void
int_floppy_disk (void)
{
  EOI (6);
}

void
int_parallel1 (void)
{
  outb (PIC_8259_READ_ISR, PIC_8259_MASTER_COMMAND);
  if (inb (PIC_8259_MASTER_COMMAND) & 0x80)
    return;
  EOI (7);
}

void
int_acpi_control (void)
{
  EOI (9);
}

void
int_peripheral1 (void)
{
  EOI (10);
}

void
int_peripheral2 (void)
{
  EOI (11);
}

void
int_ps2_mouse (void)
{
  EOI (12);
}

void
int_coprocessor (void)
{
  EOI (13);
}
