#!/bin/sh

# Generates the definitions for system calls.

def_macros=
def_protos=
while IFS= read -r line; do
    case "$line" in
	"#"* )
	    continue
	    ;;
    esac
    [ -z "$line" ] && continue

    num=`echo $line | cut -f 1 -d ' '`
    name=`echo $line | cut -f 2 -d ' '`
    ret=`echo $line | cut -f 3 -d ' '`
    params=`echo $line | cut -f 4- -d ' '`

    def_macros="$def_macros\n#define SYS_$name    $num"
    def_protos="$def_protos\n$ret sys_$name ($params);"
done < $1

$SED -e "s/@MACROS@/$def_macros/" -e "s/@PROTOS@/$def_protos/" $2 > syscall.h
