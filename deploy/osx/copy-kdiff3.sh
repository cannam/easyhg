#!/bin/bash

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

set -eu

qtdir=$(grep "Command:" Makefile | head -1 | awk '{ print $3; }' | sed s,/bin/.*,,)

if [ ! -d "$qtdir" ]; then
    echo "Failed to discover Qt installation directory from Makefile, exiting"
    exit 2
fi

kdiff_dir=easyhg-kdiff3

if [ ! -d "$kdiff_dir" ]; then
    echo
    echo "Directory $kdiff_dir not found: cloning and building it..."
    hg clone https://code.soundsoftware.ac.uk/hg/easyhg-kdiff3 "$kdiff_dir"
    cd "$kdiff_dir"/kdiff3/src-QT4
    "$qtdir"/bin/qmake -r kdiff3.pro
    make
    cd ../../..
fi

echo
echo "Copying in kdiff3 executable..."

cp "$kdiff_dir"/kdiff3/src-QT4/kdiff3.app/Contents/MacOS/kdiff3 "$app.app/Contents/MacOS/easyhg-kdiff3"

echo "Done"

