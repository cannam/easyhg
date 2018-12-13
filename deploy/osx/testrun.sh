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

pyqtdir=/Library/Python/2.7/site-packages/PyQt5

if [ ! -d "$pyqtdir" ]; then
    echo "PyQt directory $pyqtdir not found - is it installed?"
fi

enumegg=/Library/Python/2.7/site-packages/enum34-1.1.6-py2.7.egg

if [ ! -f "$enumegg" ]; then
    echo "Enum module egg $enumegg not found - check and maybe update the reference in this script"
fi

set -x

suffix=$$

move_aside() {
    sudo mv "$qtdir" "$qtdir"_$suffix
    sudo mv "$pyqtdir" "$pyqtdir"_$suffix
    sudo mv "$enumegg" "$enumegg"_$suffix
}

restore() {
    sudo mv "$qtdir"_$suffix "$qtdir"
    sudo mv "$pyqtdir"_$suffix "$pyqtdir"
    sudo mv "$enumegg"_$suffix "$enumegg"
}

move_aside
trap restore 0

"$app.app"/Contents/MacOS/$app

