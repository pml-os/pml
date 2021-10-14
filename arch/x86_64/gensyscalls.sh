#!/bin/sh

# Generates the system call table and definitions for system calls.

discard_change_warning()
{
    echo '   DO NOT EDIT THIS FILE! This file is automatically generated,' >> $1
    echo '   changes will be destroyed when regenerating! */' >> $1
    echo >> $1
}

tab_file=systab.c
echo '/* Syscall table for the x86-64 platform.' > $tab_file
discard_change_warning $tab_file
echo '#include "syscall.h"' >> $tab_file
echo >> $tab_file
echo 'void *syscall_table[] = {' >> $tab_file

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

    echo "  [SYS_$name] = sys_$name," >> $tab_file
    def_macros="$def_macros
#define SYS_$name    $num"
    def_protos="$def_protos
$ret sys_$name ($params);"
done < $1

def_file=syscall.h
echo '/* Syscall number macros and function definitions.' > $def_file
discard_change_warning $def_file
echo '#ifndef __PML_SYSCALL_H' >> $def_file
echo '#define __PML_SYSCALL_H' >> $def_file
echo "$def_macros" >> $def_file
echo >> $def_file
echo '#ifndef __ASSEMBLER__' >> $def_file
echo >> $def_file
echo '#include <pml/cdefs.h>' >> $def_file
echo '#include <pml/types.h>' >> $def_file
echo >> $def_file
echo '__BEGIN_DECLS' >> $def_file
echo >> $def_file
echo 'extern void *syscall_table[];' >> $def_file
echo "$def_protos" >> $def_file
echo >> $def_file
echo 'void syscall_init (void);' >> $def_file
echo 'long syscall (long num, ...);' >> $def_file
echo >> $def_file
echo '__END_DECLS' >> $def_file
echo >> $def_file
echo '#endif /* !__ASSEMBLER__ */' >> $def_file
echo >> $def_file
echo '#endif' >> $def_file

echo '};' >> $tab_file
