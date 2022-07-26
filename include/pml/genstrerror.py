import sys

if len(sys.argv) != 3:
    print('Usage: genstrerror.py HEADER INPUT', file=sys.stderr)
    sys.exit(1)

with open(sys.argv[2], 'r') as f:
    header = f.read()

with open(sys.argv[1], 'r') as f:
    line = ''
    table = '{ \\\n'
    while not line.startswith('#define E'):
        line = f.readline()
    while line and line != '\n':
        if line.startswith('#define'):
            errno = line.split()[1]
            desc = line.partition('/*!< ')[2].partition(' */')[0]
        elif line.startswith('/*! '):
            desc = line.partition('/*! ')[2].partition(' */')[0]
            line = f.readline()
            errno = line.split()[1]
        table += '    [{}] = "{}", \\\n'.format(errno, desc)
        line = f.readline()
    table += '  }'

header = header.replace('@TABLE@', table)
with open('strerror.h', 'w') as f:
    f.write(header)
