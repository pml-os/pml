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

void
int_pit_tick (void)
{
  pic_8259_eoi (0);
}

void
int_ps2_keyboard (void)
{
  pic_8259_eoi (1);
}

void
int_serial2 (void)
{
  pic_8259_eoi (3);
}

void
int_serial1 (void)
{
  pic_8259_eoi (4);
}

void
int_parallel2 (void)
{
  pic_8259_eoi (5);
}

void
int_floppy_disk (void)
{
  pic_8259_eoi (6);
}

void
int_parallel1 (void)
{
  outb (PIC_8259_READ_ISR, PIC_8259_MASTER_COMMAND);
  if (inb (PIC_8259_MASTER_COMMAND) & 0x80)
    return;
  pic_8259_eoi (7);
}

void
int_acpi_control (void)
{
  pic_8259_eoi (9);
}

void
int_peripheral1 (void)
{
  pic_8259_eoi (10);
}

void
int_peripheral2 (void)
{
  pic_8259_eoi (11);
}

void
int_ps2_mouse (void)
{
  pic_8259_eoi (12);
}

void
int_coprocessor (void)
{
  pic_8259_eoi (13);
}

void
int_ata_primary (void)
{
  pic_8259_eoi (14);
}

void
int_ata_secondary (void)
{
  pic_8259_eoi (15);
}
