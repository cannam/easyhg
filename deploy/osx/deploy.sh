#!/bin/bash

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).

app=EasyMercurial

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`
case "$version" in
    [0-9].[0-9]) bundleVersion="$version".0 ;;
    [0-9].[0-9].[0-9]) bundleVersion="$version" ;;
    *) echo "Error: Version $version is neither two- nor three-part number" ;;
esac

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/EASYHG_VERSION/$bundleVersion/" deploy/osx/Info.plist \
    > "$app".app/Contents/Info.plist

echo "Done: check $app.app for sanity please"

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

echo
echo "Making dmg..."

hdiutil create -srcfolder "$app".app "$app"-"$version".dmg -volname "$app"-"$version"

echo "Done"
