#!/bin/bash

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

echo
echo "I expect you to have already copied QtCore, QtNetwork and QtGui to "
echo "$app.app/Contents/Frameworks and PyQt4/QtCore.so and PyQt4/QtGui.so to "
echo "$app.app/Contents/MacOS -- expect errors to follow if they're missing"
echo

echo "Fixing up loader paths in binaries..."

install_name_tool -id QtCore "$app.app/Contents/Frameworks/QtCore"
install_name_tool -id QtGui "$app.app/Contents/Frameworks/QtGui"
install_name_tool -id QtNetwork "$app.app/Contents/Frameworks/QtNetwork"

for fwk in QtCore QtGui QtNetwork; do
        find "$app.app" -type f -print | while read x; do
                current=$(otool -L "$x" | grep "$fwk.framework/" | awk '{ print $1; }')
                [ -z "$current" ] && continue
                echo "$x has $current"
                relative=$(echo "$x" | sed -e "s,$app.app/Contents/,," \
                        -e 's,[^/]*/,../,g' -e 's,/[^/]*$,/Frameworks/'"$fwk"',' )
                echo "replacing with relative path $relative"
                install_name_tool -change "$current" "@loader_path/$relative" "$x"
        done
done

echo "Done: be sure to run the app and see that it works!"

