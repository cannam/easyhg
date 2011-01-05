#!/bin/bash
echo Args are: $@ 1>&2
if [ "$#" -lt 3 ]; then
	echo Insufficient arguments: $@
	exit 2
fi
while [ "$#" -gt 3 ]; do
	shift
done
/Developer/Applications/Utilities/FileMerge.app/Contents/MacOS/FileMerge \
	-left "$1" -merge "$1" -ancestor "$2" -right "$3"

