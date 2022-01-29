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
        super(ExcUp, self).__init__('exc-up', gdb.COMMAND_STACK);

    def invoke(self, arg, from_tty):
        rip = hex(read_stack(0))
        cs = hex(read_stack(8))
        eflags = hex(read_stack(16))
        rsp = hex(read_stack(24))
        ss = hex(read_stack(32))
        gdb.newest_frame().select()
        gdb.execute('set $rip = ' + rip, from_tty)
        gdb.execute('set $cs = ' + cs, from_tty)
        gdb.execute('set $eflags = ' + eflags, from_tty)
        gdb.execute('set $rsp = ' + rsp, from_tty)
        gdb.execute('set $ss = ' + ss, from_tty)
        print('Stepping out of exception')
        print('RIP: ' + rip)
        print('CS: ' + cs)
        print('EFLAGS: ' + eflags)
        print('RSP: ' + rsp)
        print('SS: ' + ss)

ExcUp()
