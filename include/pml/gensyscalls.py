import configparser
import sys

if len(sys.argv) != 3:
    print('Usage: gensyscalls.py CONFIG INPUT', file=sys.stderr)
    sys.exit(1)

config = configparser.ConfigParser()
config.read(sys.argv[1])

with open(sys.argv[2], 'r') as f:
    header = f.read()

macros = ""
protos = ""
for syscall in config.sections():
    number = config[syscall]["number"]
    return_type = config[syscall]["return_type"]
    params = config[syscall]["params"]
    macros += '\n#define SYS_{}    {}'.format(syscall, number)
    protos += '\n{} sys_{} ({});'.format(return_type, syscall, params)

header = header.replace('@MACROS@', macros).replace('@PROTOS@', protos)
with open('syscall.h', 'w') as f:
    f.write(header)
