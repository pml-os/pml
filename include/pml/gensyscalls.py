import configparser
import sys

if len(sys.argv) != 3:
    print('Usage: gensyscalls.py DIR INPUT', file=sys.stderr)
    sys.exit(1)

config = configparser.ConfigParser()
config.read(sys.argv[1] + '/syscalls.lst')

with open(sys.argv[2], 'r') as f:
    header = f.read()

macros = ""
protos = ''
for syscall in config.sections():
    number = config[syscall]['number']
    return_type = config[syscall]['return_type']
    params = config[syscall]['params']
    if config[syscall]['attr']:
        attr = ' ' + config[syscall]['attr']
    else:
        attr = ''
    macros += '\n#define SYS_{}    {}'.format(syscall, number)
    protos += '\n{} sys_{} ({}){};'.format(return_type, syscall, params, attr)

with open(sys.argv[1] + '/syscall-part.h') as f:
    syscall_part = f.read()
header = header.replace('@MACROS@', macros)
header = header.replace('@PROTOS@', protos)
header = header.replace('@PART@', syscall_part)
with open('syscall.h', 'w') as f:
    f.write(header)
