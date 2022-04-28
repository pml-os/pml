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
#include <pml/process.h>

void
int_div_zero (uintptr_t addr)
{
  siginfo_t info;
  info.si_signo = SIGFPE;
  info.si_code = FPE_INTDIV;
  info.si_errno = 0;
  send_signal_thread (THIS_THREAD, SIGFPE, &info);
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
  siginfo_t info;
  info.si_signo = SIGTRAP;
  info.si_code = TRAP_BRKPT;
  info.si_errno = 0;
  info.si_addr = (void *) addr;
  send_signal_thread (THIS_THREAD, SIGTRAP, &info);
}

void
int_overflow (uintptr_t addr)
{
  siginfo_t info;
  info.si_signo = SIGFPE;
  info.si_code = FPE_INTOVF;
  info.si_errno = 0;
  send_signal_thread (THIS_THREAD, SIGFPE, &info);
}

void
int_bound_range (uintptr_t addr)
{
  panic ("CPU exception: bound range exceeded\nInstruction: %p\nPID: %d\n"
	 "TID: %d", addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_bad_opcode (uintptr_t addr)
{
  siginfo_t info;
  info.si_signo = SIGILL;
  info.si_code = ILL_ILLOPC;
  info.si_errno = 0;
  info.si_addr = (void *) addr;
  send_signal_thread (THIS_THREAD, SIGILL, &info);
}

void
int_no_device (uintptr_t addr)
{
  panic ("CPU exception: device not available\nInstruction: %p\nPID: %d\n"
	 "TID: %d", addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_double_fault (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: double fault\nInstruction: %p\nPID: %d\nTID: %d",
	 addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_bad_tss (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: invalid TSS selector 0x%02lx\nInstruction: %p\n"
	 "PID: %d\nTID: %d", err, addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_bad_segment (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: invalid segment selector 0x%02lx\nInstruction: %p\n",
	 "PID: %d\nTID: %d", err, addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_stack_segment (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: stack-segment fault on selector 0x%02lx\n"
	 "Instruction: %p\nPID: %d\nTID: %d\n", err, addr, THIS_PROCESS->pid,
	 THIS_THREAD->tid);
}

void
int_gpf (unsigned long err, uintptr_t addr)
{
  siginfo_t info;
  if (err)
    panic ("CPU exception: general protection fault (segment 0x%02lx)\n"
	   "Instruction: %p\nPID: %d\nTID: %d\n", err, addr, THIS_PROCESS->pid,
	   THIS_THREAD->tid);
  info.si_signo = SIGILL;
  info.si_code = ILL_PRVOPC;
  info.si_errno = 0;
  info.si_addr = (void *) addr;
  send_signal_thread (THIS_THREAD, SIGILL, &info);
}

void
int_fpu (uintptr_t addr)
{
  panic ("CPU exception: x87 FPU exception\nInstruction: %p\nPID: %d\nTID: %d",
	 addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_align_check (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: alignment check\nInstruction: %p\nPID: %d\nTID: %d",
	 addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_machine_check (uintptr_t addr)
{
  panic ("CPU exception: machine check\nInstruction: %p\nPID: %d\nTID: %d",
	 addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_simd_fpu (uintptr_t addr)
{
  panic ("CPU exception: SIMD FPU exception\nInstruction: %p\nPID: %d\nTID: %d",
	 addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_virtualization (uintptr_t addr)
{
  panic ("CPU exception: virtualization exception\nInstruction: %p\n"
	 "PID: %d\nTID: %d", addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}

void
int_security (unsigned long err, uintptr_t addr)
{
  panic ("CPU exception: security exception\nInstruction: %p\n"
	 "PID: %d\nTID: %d", addr, THIS_PROCESS->pid, THIS_THREAD->tid);
}
