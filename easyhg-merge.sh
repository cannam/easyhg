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
for d in easyhg-kdiff3 easyhg-kdiff3.exe kdiff3 kdiff3.exe; do
    exe="$p/$d"
    if [ ! -x "$exe" ]; then
	exe="$(type -path $d)"
	if [ ! -x "$exe" ]; then
	    exe=""
	fi
    fi
    if [ -n "$exe" ]; then
	exec "$exe" --auto "$ancestor" "$left" "$right" --output "$out" --auto -L1 "`basename $left` (Common ancestor)" -L2 "$left (Your current version)" -L3 "`basename $left` (Version being merged)"
    fi
done
fm=/Developer/Applications/Utilities/FileMerge.app/Contents/MacOS/FileMerge
if [ -x "$fm" ]; then
    exec "$fm" -left "$left" -merge "$out" -ancestor "$ancestor" -right "$right"
fi
exit 1
