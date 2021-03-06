#!/bin/sh

# Generates interrupt handler stubs and a function to fill the IDT with
# interrupt vectors.

# This file contains embedded tabs; do not untabify!

discard_change_warning()
{
    echo '   DO NOT EDIT THIS FILE! This file is automatically generated,' >> $1
    echo '   changes will be destroyed when regenerating! */' >> $1
    echo >> $1
}

stubs_file=irq-stubs.S
echo '/* Interrupt handler stubs for the x86-64 platform.' > $stubs_file
discard_change_warning $stubs_file
echo '#include <pml/asm.h>' >> $stubs_file

vec_decls=
vec_calls='  /* Add interrupt handlers to the IDT */'
while IFS= read -r line; do
    case "$line" in
	"#"* )
	    continue
	    ;;
    esac
    [ -z "$line" ] && continue

    num=`echo $line | cut -f 1 -d ' '`
    name=`echo $line | cut -f 2 -d ' '`
    type=IDT_GATE_`echo $line | cut -f 3 -d ' '`
    stub=`echo $line | cut -f 4 -d ' '`
    error=`echo $line | cut -f 5 -d ' '`
    [ -z "$error" ] && error=false

    if $stub; then
	# Create the stub assembly routine
	echo >> $stubs_file
	echo "	.global int_stub_$name" >> $stubs_file
	echo "ASM_FUNC_BEGIN (int_stub_$name):" >> $stubs_file
	if $error; then
	    echo '	push	%rsi' >> $stubs_file
	    echo '	mov	16(%rsp), %rsi' >> $stubs_file
	    echo '	push	%rdi' >> $stubs_file
	    echo '	mov	16(%rsp), %rdi' >> $stubs_file
	else
	    echo '	push	%rdi' >> $stubs_file
	    echo '	mov	8(%rsp), %rdi' >> $stubs_file
	fi
	echo '	call	int_save_registers' >> $stubs_file
	echo "	call	int_$name" >> $stubs_file
	echo '  call	run_signal' >> $stubs_file
	echo '	call	int_restore_registers' >> $stubs_file
	echo '	pop	%rdi' >> $stubs_file
	if $error; then
	    echo '	pop	%rsi' >> $stubs_file
	    echo '	add	$8, %rsp' >> $stubs_file
	fi
	echo '	iretq' >> $stubs_file
	echo "ASM_FUNC_END (int_stub_$name)" >> $stubs_file

	# Add the stub function to the IDT
        vec_decls="$vec_decls
void int_stub_$name (void);"
	vec_calls="$vec_calls
  set_int_vector ($num, int_stub_$name, 3, $type);"
    else
	# Add the handler function to the IDT
	vec_decls="$vec_decls
void int_$name (void);"
	vec_calls="$vec_calls
  set_int_vector ($num, int_$name, 3, $type);"
    fi
done < $1

vec_file=irq-vectors.c
echo '/* Initializes IDT with interrupt and exception handlers.' > $vec_file
discard_change_warning $vec_file
echo '#include <pml/interrupt.h>' >> $vec_file
echo "$vec_decls" >> $vec_file
echo >> $vec_file
echo 'void' >> $vec_file
echo 'fill_idt_vectors (void)' >> $vec_file
echo '{' >> $vec_file
echo "$vec_calls" >> $vec_file
echo '}' >> $vec_file
