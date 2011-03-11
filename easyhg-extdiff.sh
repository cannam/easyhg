#!/bin/bash -x
p=`dirname $0`
if [ $# -lt 2 ]; then 
    echo Insufficient arguments: $@
    exit 2
fi
while [ $# -gt 2 ]; do
    shift
done
for d in kdiff3 kdiff3.exe; do
    if [ -x "$p/$d" ]; then
	exec "$p/$d" "$1" "$2"
    elif [ -x "$(type -path $d)" ]; then
	exec "$d" "$1" "$2"
    fi
done
od=/usr/bin/opendiff
if [ -x "$od" ]; then
    "$od" "$1" "$2" | cat
    exit 0
fi
exit 1

