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

echo "Done: check $app.app/Contents/Info.plist for sanity please"

bash deploy/osx/paths.sh "$app"

echo
echo "Making dmg..."

mkdir "$app"-"$version" &&
	ln -s /Applications "$app"-"$version"/Applications &&
	cp -rp "$app".app "$app"-"$version"/ &&
	hdiutil create -srcfolder "$app"-"$version" "$app"-"$version".dmg -volname "$app"-"$version" && 
	rm -r "$app"-"$version"

echo "Done"
