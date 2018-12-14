#!/bin/bash

set -eu

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

PYQT_DIR=/Library/Python/2.7/site-packages/PyQt5

if [ ! -d "$PYQT_DIR" ]; then
    echo "PyQt directory $PYQT_DIR not found - is it installed?"
    exit 2
fi

if [ ! -f "$PYQT_DIR/sip.so" ]; then
    echo "sip.so not found in $PYQT_DIR - did you remember --sip-module PyQt5.sip when building sip?"
    exit 2
fi

ENUM_EGG=/Library/Python/2.7/site-packages/enum34-1.1.6-py2.7.egg

if [ ! -f "$ENUM_EGG" ]; then
    echo "Enum module egg $ENUM_EGG not found - install it or update the reference in this script"
    exit 2
fi

pydir="$app.app/Contents/MacOS/Py27"
mkdir -p "$pydir/PyQt5"
mkdir -p "$pydir/enum"

echo 
echo "Copying PyQt libraries..."
for library in Qt QtCore QtGui QtWidgets sip; do
    cp "$PYQT_DIR/$library.so" "$pydir/PyQt5/"
done
cp "$PYQT_DIR/__init__.py" "$pydir/PyQt5/"

echo
echo "Copying enum library..."
( cd "$pydir" ; mkdir -p tmp ; cd tmp ; unzip "$ENUM_EGG" ; cp enum/__init__.py ../enum/ ; cd .. ; rm -rf tmp )

echo "Done"

