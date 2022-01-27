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
int_div_zero (uintptr_t addr)
{
  panic ("CPU exception: division by zero");
}

void
int_debug (uintptr_t addr)
{
}

void
int_nmi (uintptr_t addr)
{
}

void
int_breakpoint (uintptr_t addr)
{
}

void
int_overflow (uintptr_t addr)
{
  panic ("CPU exception: overflow");
}

void
int_bound_range (uintptr_t addr)
{
  panic ("CPU exception: bound range exceeded");
}

void
int_bad_opcode (uintptr_t addr)
{
  panic ("CPU exception: invalid opcode");
}

void
int_no_device (uintptr_t addr)
{
  panic ("CPU exception: device not available");
}

void
int_double_fault (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: double fault");
}

void
int_bad_tss (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: invalid TSS selector %#02lx", err);
}

void
int_bad_segment (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: invalid segment selector %#02lx", err);
}

void
int_stack_segment (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: stack-segment fault on selector %#02lx", err);
}

void
int_gpf (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: general protection fault (segment %#02lx)", err);
}

void
int_fpu (uintptr_t addr)
{
  panic ("CPU exception: x87 FPU exception");
}

void
int_align_check (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: alignment check");
}

void
int_machine_check (uintptr_t addr)
{
  panic ("CPU exception: machine check");
}

void
int_simd_fpu (uintptr_t addr)
{
  panic ("CPU exception: SIMD FPU exception");
}

void
int_virtualization (uintptr_t addr)
{
  panic ("CPU exception: virtualization exception");
}

void
int_security (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: security exception");
}
