#!/bin/bash
if [ $# -lt 2 ]; then 
    echo Insufficient arguments: $@
    exit 2
fi
while [ $# -gt 2 ]; do
    shift
done
/usr/bin/opendiff "$1" "$2"
