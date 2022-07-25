# Python script for enhanced GDB debugging support in PML kernels.

# gdb.py -- This file is part of PML.
# Copyright (C) 2021 XNSC
#
# PML is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PML is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PML. If not, see <https://www.gnu.org/licenses/>.

# This file is included as an inlined script in a PML kernel executable
# built using --with-gdb-script. Its features can be used by adding the
# PML executable to GDB's auto-load safe path and setting the symbol file to
# the PML executable. This script introduces several commands that are useful
# when debugging a PML kernel.

import gdb

def memv_addr(mem):
    return int.from_bytes(bytes(mem), 'little')

def read_stack(offset, size = 8):
    rsp = gdb.selected_frame().read_register('rsp')
    inf = gdb.selected_inferior()
    mem = inf.read_memory(rsp + offset, size)
    return memv_addr(mem)

class ExcUp(gdb.Command):
    '''Moves up a stack frame that was caused by an x86-64 CPU exception.'''

    def __init__(self):
        super(ExcUp, self).__init__('exc-up', gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        self.dont_repeat()
        rip = hex(read_stack(0))
        cs = hex(read_stack(8))
        eflags = hex(read_stack(16))
        rsp = hex(read_stack(24))
        ss = hex(read_stack(32))
        gdb.newest_frame().select()
        gdb.execute('set $rip = ' + rip, from_tty)
        gdb.execute('set $eflags = ' + eflags, from_tty)
        gdb.execute('set $cs = ' + cs, from_tty)
        gdb.execute('set $ss = ' + ss, from_tty)
        gdb.execute('set $rsp = ' + rsp, from_tty)
        print('Stepping out of exception')
        print('RIP: ' + rip)
        print('CS: ' + cs)
        print('EFLAGS: ' + eflags)
        print('RSP: ' + rsp)
        print('SS: ' + ss)

class PrintPage(gdb.Command):
    '''Pretty-prints an x86-64 paging structure (PT, PDT, PDPT, or PML4T).'''

    def __init__(self):
        super(PrintPage, self).__init__('print-page', gdb.COMMAND_DATA)
        self.page_flags = {
            1 << 8: 'G',
            1 << 7: 'S',
            1 << 6: 'D',
            1 << 5: 'A',
            1 << 4: 'N',
            1 << 3: 'C',
            1 << 2: 'U',
            1 << 1: 'W',
            1 << 0: 'P'
        }
        self.custom_page_flags = {
            1 << 9: 'COW'
        }

    def flags(self, addr):
        cf = []
        f = ''
        if addr & 1:
            if addr & (1 << 9):
                cf.append('COW')
            for k, v in self.page_flags.items():
                if addr & k:
                    f += v
                else:
                    f += '.'
        else:
            f = '.' * len(self.page_flags)
            for k, v in self.custom_page_flags.items():
                if addr & k:
                    cf.append(v)
        return f + ' ' + ' '.join(cf)

    def invoke(self, arg, from_tty):
        value = gdb.parse_and_eval('*{}@512'.format(arg))
        for i in range(512):
            entry = int(value[i])
            if entry:
                print('[{0:3}] {1:20}    {2}'.format(i, hex(entry & ~0xfff),
                                                     self.flags(entry)))

class ThisProcess(gdb.Command):
    '''Prints the current process structure.'''

    def __init__(self):
        super(ThisProcess, self).__init__('this-process', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute('print *process_queue.queue[process_queue.front]', from_tty)

class ThisThread(gdb.Command):
    '''Prints the current thread structure.'''

    def __init__(self):
        super(ThisThread, self).__init__('this-thread', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute('print *process_queue.queue[process_queue.front]->' +
                    'threads.queue[process_queue.queue[process_queue.front]->' +
                    'threads.front]', from_tty)

class PrintHeap(gdb.Command):
    '''Prints the contents of the kernel heap.'''

    def __init__(self):
        super(PrintHeap, self).__init__('print-heap', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        header_type = gdb.lookup_type('struct kh_header')
        tail_type = gdb.lookup_type('struct kh_tail')
        header_size = header_type.sizeof
        tail_size = tail_type.sizeof
        header_type = header_type.pointer()
        tail_type = tail_type.pointer()
        void_type = gdb.lookup_type('void').pointer()
        header = gdb.parse_and_eval('(struct kh_header *) kh_base_addr')
        end_addr = gdb.parse_and_eval('kh_end_addr')
        i = 0
        while header.address < end_addr:
            if int(header['magic']) != 0x07242005:
                print('Bad header magic number: ' + hex(header['magic']))
                break
            block = header.cast(void_type) + header_size
            tail = (block + header['size']).cast(tail_type)
            if int(tail['magic']) != 0xdeadc0de:
                print('Bad tail magic number: ' + hex(tail['magic']))
                break
            if tail['header'] != header:
                print('Unmatched header in tail block')
                break
            alloc = 'allocated' if header['flags'] & 1 else 'free'
            addr = str(block)
            size = int(header['size'])
            i += 1
            print('{}: {}, size {} ({})'.format(addr, alloc, size, hex(size)))
            header = (tail.cast(void_type) + tail_size).cast(header_type)
        print('{} objects total'.format(i))

class Relocate(gdb.Command):
    '''Prints the virtual address of a physical address.'''

    def __init__(self):
        super(Relocate, self).__init__('relocate', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        value = gdb.parse_and_eval(arg)
        if value.type.code == gdb.TYPE_CODE_PTR:
            type_str = '(typeof({}))'.format(arg)
        else:
            type_str = '(void *)'
        gdb.execute('print {} (0xfffffe0000000000 | (uintptr_t) ({}))'
                    .format(type_str, arg), from_tty)

class PML4T(gdb.Command):
    '''Prints the address of the current PML4T structure.'''

    def __init__(self):
        super(PML4T, self).__init__('pml4t', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute('relocate (uintptr_t *) $cr3', from_tty)

class PrintPageIndex(gdb.Command):
    '''Prints the address of an index in a page structure.'''

    def __init__(self):
        super(PrintPageIndex, self).__init__('print-page-index',
                                             gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute('relocate (uintptr_t *) ((uintptr_t) ({}) & ~0xfff)'
                    .format(arg), from_tty)

class PageState(gdb.Command):
    '''Prints the allocation state of a physical page frame.'''

    def __init__(self):
        super(PageState, self).__init__('page-state', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute('print phys_alloc_table[(uintptr_t) ({}) >> 12]'
                    .format(arg), from_tty)

class InterruptsOff(gdb.Command):
    '''Disables hardware interrupts.'''

    def __init__(self):
        super(InterruptsOff, self).__init__('int-off', gdb.COMMAND_RUNNING)

    def invoke(self, arg, from_tty):
        gdb.execute('set $eflags = $eflags & ~0x200', from_tty)

class InterruptsOn(gdb.Command):
    '''Enables hardware interrupts.'''

    def __init__(self):
        super(InterruptsOn, self).__init__('int-on', gdb.COMMAND_RUNNING)

    def invoke(self, arg, from_tty):
        gdb.execute('set $eflags = $eflags | 0x200', from_tty)

ExcUp()
PrintPage()
ThisProcess()
ThisThread()
PrintHeap()
Relocate()
PML4T()
PrintPageIndex()
PageState()
InterruptsOff()
InterruptsOn()

gdb.execute('alias -a asf = add-symbol-file')
gdb.execute('alias -a ppage = print-page')
gdb.execute('alias -a thpr = this-process')
gdb.execute('alias -a thtd = this-thread')
gdb.execute('alias -a ph = print-heap')
gdb.execute('alias -a ppi = print-page-index')
gdb.execute('alias -a ps = page-state')
