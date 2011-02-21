#!/bin/bash -x
p=`dirname $0`
if [ $# -lt 2 ]; then 
    echo Insufficient arguments: $@
    exit 2
fi
while [ $# -gt 2 ]; do
    shift
done
found=""
for d in kdiff3 kdiff3.exe; do
    if [ -x "$p/$d" ]; then
	found=true
	"$p/$d" "$1" "$2"
	break
    elif [ -x "$(type -path $d)" ]; then
	found=true
	"$d" "$1" "$2"
	break;
    fi
done
if [ -z "$found" ]; then
    od=/usr/bin/opendiff
    if [ -x "$od" ]; then
	found=true
	"$od" "$1" "$2" | cat
    fi
fi
[ -n "$found" ]

