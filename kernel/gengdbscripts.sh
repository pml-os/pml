#!/bin/sh

# Generates a source file containing inlined GDB scripts.

out_file=gdb-scripts.c
cp $2 $out_file
echo >> $out_file

while IFS= read -r line; do
    case "$line" in
	"#"* )
	    continue
	    ;;
    esac
    [ -z "$line" ] && continue

    name=`echo $line | cut -f 1 -d ' '`
    path="$3/`echo $line | cut -f 2 -d ' '`"
    name_id=`echo "$name" | tr -c [:alnum:][:blank:] _`
    name_ch="{ \\\\
`echo "$name" | $XXD -i | sed 's/$/ \\\\/'`
}"
    source="{ \\\\
`cat "$path" | $XXD -i | sed 's/$/ \\\\/'`
}"
    echo "#define __NAME_$name_id $name_ch" >> $out_file
    echo "#define __SOURCE_$name_id $source" >> $out_file
    echo "GDB_SCRIPT ($name_id, __NAME_$name_id, __SOURCE_$name_id)" \
	 >> $out_file
done < $1
