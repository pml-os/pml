/* exceptions.c -- This file is part of PML.
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
#include <pml/panic.h>

void
int_div_zero (void)
{
  panic ("CPU exception: division by zero");
}

void
int_debug (void)
{
}

void
int_nmi (void)
{
}

void
int_breakpoint (void)
{
}

void
int_overflow (void)
{
  panic ("CPU exception: overflow");
}

void
int_bound_range (void)
{
  panic ("CPU exception: bound range exceeded");
}

void
int_bad_opcode (void)
{
  panic ("CPU exception: invalid opcode");
}

void
int_no_device (void)
{
  panic ("CPU exception: device not available");
}

void
int_double_fault (unsigned long err)
{
  panic ("CPU exception: double fault");
}

void
int_bad_tss (unsigned long err)
{
  panic ("CPU exception: invalid TSS selector %#02lx", err);
}

void
int_bad_segment (unsigned long err)
{
  panic ("CPU exception: invalid segment selector %#02lx", err);
}

void
int_stack_segment (unsigned long err)
{
  panic ("CPU exception: stack-segment fault on selector %#02lx", err);
}

void
int_gpf (unsigned long err)
{
  panic ("CPU exception: general protection fault (segment %#02lx)", err);
}

void
int_fpu (void)
{
  panic ("CPU exception: x87 FPU exception");
}

void
int_align_check (unsigned long err)
{
  panic ("CPU exception: alignment check");
}

void
int_machine_check (void)
{
  panic ("CPU exception: machine check");
}

void
int_simd_fpu (void)
{
  panic ("CPU exception: SIMD FPU exception");
}

void
int_virtualization (void)
{
  panic ("CPU exception: virtualization exception");
}

void
int_security (unsigned long err)
{
  panic ("CPU exception: security exception");
}
