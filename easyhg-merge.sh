#!/bin/bash
p=`dirname $0`
echo Args are: $@ 1>&2
if [ "$#" -lt 3 ]; then
	echo Insufficient arguments: $@
	exit 2
fi
while [ "$#" -gt 3 ]; do
	shift
done
out="$1"
left="$1"
ancestor="$2"
right="$3"
found=""
for d in kdiff3 kdiff3.exe; do
    if [ -x "$p/$d" ]; then
	found=true
	"$p/$d" "$ancestor" "$left" "$right" -o "$out"
	break
    elif [ -x "$(type -path $d)" ]; then
	found=true
	"$d" "$ancestor" "$left" "$right" -o "$out"
	break;
    fi
done
if [ -z "$found" ]; then
    fm=/Developer/Applications/Utilities/FileMerge.app/Contents/MacOS/FileMerge
    if [ -x "$fm" ]; then
	found=true
	"$fm" -left "$left" -merge "$out" -ancestor "$ancestor" -right "$right"
    fi
fi
[ -n "$found" ]
