#!/bin/bash

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).

app=EasyMercurial

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`
bundleVersion="$version".0

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/EASYHG_VERSION/$bundleVersion/" deploy/osx/Info.plist \
    > "$app".app/Contents/Info.plist

echo "Done: check $app.app for sanity please"

echo
echo "Making dmg..."

hdiutil create -srcfolder "$app".app "$app"-"$version".dmg -volname "$app"-"$version"

echo "Done"
