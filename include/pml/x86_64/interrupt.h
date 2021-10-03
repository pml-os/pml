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

/*! @file */

#define PIC_8259_MASTER_COMMAND 0x20
#define PIC_8259_MASTER_DATA    0x21
#define PIC_8259_SLAVE_COMMAND  0xa0
#define PIC_8259_SLAVE_DATA     0xa1

#define PIC_8259_EOI            0x20
#define PIC_8259_READ_IRR       0x0a
#define PIC_8259_READ_ISR       0x0b

#define IDT_GATE_TASK           0x05
#define IDT_GATE_INT            0x0e
#define IDT_GATE_TRAP           0x0f

#define IDT_ATTR_PRESENT        (1 << 7)
#define IDT_SIZE                256

/*! Maximum number of CPUs supported for SMP */
#define MAX_CORES               16

/*! Number of IRQs handled by an I/O APIC */
#define IOAPIC_IRQ_COUNT        24

/*! Default address of local APIC */
#define LOCAL_APIC_DEFAULT_ADDR 0xfee00000

#define LOCAL_APIC_FLAG_ENABLED             (1 << 0)
#define LOCAL_APIC_FLAG_ONLINE_CAP          (1 << 1)

#define LOCAL_APIC_REG_ID                   0x020
#define LOCAL_APIC_REG_VERSION              0x030
#define LOCAL_APIC_REG_TPR                  0x080
#define LOCAL_APIC_REG_APR                  0x090
#define LOCAL_APIC_REG_PPR                  0x0a0
#define LOCAL_APIC_REG_EOI                  0x0b0
#define LOCAL_APIC_REG_RRD                  0x0c0
#define LOCAL_APIC_REG_LOGICAL_DEST         0x0d0
#define LOCAL_APIC_REG_DEST_FORMAT          0x0e0
#define LOCAL_APIC_REG_SPURIOUS_INT_VEC     0x0f0
#define LOCAL_APIC_REG_ISR_BASE             0x100
#define LOCAL_APIC_REG_TMR_BASE             0x180
#define LOCAL_APIC_REG_IRR_BASE             0x200
#define LOCAL_APIC_REG_ERR_STATUS           0x280
#define LOCAL_APIC_REG_LVT_CMCI             0x2f0
#define LOCAL_APIC_REG_ICR_BASE             0x300
#define LOCAL_APIC_REG_LVT_TIMER            0x320
#define LOCAL_APIC_REG_LVT_THERMAL_SENSOR   0x330
#define LOCAL_APIC_REG_LVT_PMC              0x340
#define LOCAL_APIC_REG_LVT_LINT0            0x350
#define LOCAL_APIC_REG_LVT_LINT1            0x360
#define LOCAL_APIC_REG_LVT_ERROR            0x370
#define LOCAL_APIC_REG_INIT_COUNT           0x380
#define LOCAL_APIC_REG_CURR_COUNT           0x390
#define LOCAL_APIC_REG_DIVIDE_CONFIG        0x3e0

#define IOAPIC_REG_ID                       0x00
#define IOAPIC_REG_VERSION                  0x01
#define IOAPIC_REG_MAX_ENTRIES              0x01
#define IOAPIC_REG_APR                      0x02
#define IOAPIC_REG_REDIR_BASE               0x10
#define IOAPIC_REG_REDIR_LOW(x)             (IOAPIC_REG_REDIR_BASE + (x) * 2)
#define IOAPIC_REG_REDIR_HIGH(x)            (IOAPIC_REG_REDIR_LOW (x) + 1)

#define IOAPIC_FLAG_LOW_ACTIVE(f)           ((unsigned char) (f) & 3)
#define IOAPIC_FLAG_LEVEL_TRIGGER(f)        (((unsigned char) (f) & 0xf) >> 2)

#ifndef __ASSEMBLER__

#include <pml/cdefs.h>

#define LOCAL_APIC_REG(reg)						\
  (*((volatile uint32_t *) ((uintptr_t) local_apic_addr + (reg))))

#ifdef USE_APIC
#define EOI(irq)                local_apic_eoi ()
#else
#define EOI(irq)                pic_8259_eoi (irq)
#endif

/*! Format of an entry in the long mode interrupt descriptor table (IDT). */

struct idt_entry
{
  uint16_t offset_low;          /*!< Bits 0-15 of interrupt handler address */
  uint16_t selector;            /*!< Code segment to use for interrupt */
  unsigned char ist;
  unsigned char type_attr;
  uint16_t offset_mid;          /*!< Bits 16-31 of interrupt handler address */
  uint32_t offset_high;         /*!< Bits 32-63 of interrupt handler address */
  uint32_t reserved;
};

struct idt_ptr
{
  uint16_t size;
  struct idt_entry *addr;
} __packed;

/*! Represents an APIC ID. */
typedef unsigned char apic_id_t;

/*!
 * I/O APIC delivery modes.
 */

enum ioapic_mode
{
  IOAPIC_MODE_FIXED = 0,        /*!< Fixed interrupt */
  IOAPIC_MODE_LOW_PRIORITY,     /*!< Low-priority interrupt */
  IOAPIC_MODE_SMI,              /*!< System management interrupt */
  IOAPIC_MODE_NMI,              /*!< Non-maskable interrupt */
  IOAPIC_MODE_INIT,             /*!< @c INIT interrupt */
  IOAPIC_MODE_EXTERNAL          /*!< External interrupt */
};

/*! Represents an I/O APIC delivery mode. */
typedef enum ioapic_mode ioapic_mode_t;

/*!
 * Loads the interrupt descriptor table referenced by the given pointer.
 *
 * @param ptr the IDT pointer to load
 */

__always_inline static inline void
load_idt (struct idt_ptr ptr)
{
  __asm__ volatile ("lidt %0" :: "m" (ptr));
}

/*!
 * Disables hardware-generated interrupts.
 */

__always_inline static inline void
int_disable (void)
{
  __asm__ volatile ("cli");
}

/*!
 * Enables hardware-generated interrupts.
 */

__always_inline static inline void
int_enable (void)
{
  __asm__ volatile ("sti");
}

__BEGIN_DECLS

extern apic_id_t local_apics[MAX_CORES];
extern size_t local_apic_count;
extern void *local_apic_addr;
extern apic_id_t ioapic_id;
extern void *ioapic_addr;
extern unsigned int ioapic_gsi_base;

void pic_8259_remap (void);
void pic_8259_disable (void);
void pic_8259_eoi (unsigned char irq);

void local_apic_clear_errors (void);
void local_apic_eoi (void);

uint64_t ioapic_entry (unsigned char vector, apic_id_t apic_id,
		       ioapic_mode_t mode, int low_active, int level_trigger);
void ioapic_set_irq (unsigned char irq, uint64_t entry);

void set_int_vector (unsigned char num, void *addr, unsigned char privilege,
		     unsigned char type);
void fill_idt_vectors (void);
void init_idt (void);

void int_start (void);

__END_DECLS

/*!
 * Reads a value from an I/O APIC register.
 *
 * @param reg the register to read from
 * @return the value that was read
 */

__always_inline static inline unsigned int
ioapic_read (unsigned char reg)
{
  volatile unsigned int *ioapic = (volatile unsigned int *) ioapic_addr;
  ioapic[0] = reg;
  return ioapic[4];
}

/*!
 * Writes a value to an I/O APIC register.
 *
 * @param reg the register to write to
 * @param value the value to write
 */

__always_inline static inline void
ioapic_write (unsigned char reg, unsigned int value)
{
  volatile unsigned int *ioapic = (volatile unsigned int *) ioapic_addr;
  ioapic[0] = reg;
  ioapic[4] = value;
}

#endif /* !__ASSEMBLER__ */

#endif
