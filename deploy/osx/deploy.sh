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

install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore "$app.app/Contents/MacOS/$app"
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui "$app.app/Contents/MacOS/$app"
install_name_tool -change QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork "$app.app/Contents/MacOS/$app"

install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore "$app.app/Contents/MacOS/kdiff3"
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui "$app.app/Contents/MacOS/kdiff3"
install_name_tool -change QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork "$app.app/Contents/MacOS/kdiff3"

install_name_tool -change QtCore.framework/Versions/4/QtCore @loader_path/QtCore "$app.app/Contents/Frameworks/QtGui"
install_name_tool -change QtCore.framework/Versions/4/QtCore @loader_path/QtCore "$app.app/Contents/Frameworks/QtNetwork"

install_name_tool -change QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore "$app.app/Contents/MacOS/PyQt4/QtCore.so"
install_name_tool -change QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore "$app.app/Contents/MacOS/PyQt4/QtGui.so"
install_name_tool -change QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtGui "$app.app/Contents/MacOS/PyQt4/QtGui.so"

echo "Done: be sure to run the app and see that it works!"

echo
echo "Making dmg..."

hdiutil create -srcfolder "$app".app "$app"-"$version".dmg -volname "$app"-"$version"

echo "Done"
