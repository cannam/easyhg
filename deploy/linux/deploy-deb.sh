#!/bin/bash
# 
# Run this from the build root (with sudo, I think)

usage() {
    echo
    echo "Usage:"
    echo
    echo "$0 <version> <architecture>"
    echo
    echo "For example: $0 1.4cc1-1 amd64"
    echo
    exit 2
}

version="$1"
arch="$2"

if [ -z "$version" ] || [ -z "$arch" ]; then
    usage
fi

set -eu

program=EasyMercurial
depdir=deploy/linux

targetdir="${program}_${version}_${arch}"

echo "Target dir is $targetdir"

if [ -d "$targetdir" ]; then
    echo "Target directory exists, not overwriting"
    exit
fi

mkdir "$targetdir"

mkdir "$targetdir/DEBIAN"

cp "$depdir"/control "$targetdir"/DEBIAN/

mkdir -p "$targetdir"/usr/bin "$targetdir"/usr/share/pixmaps "$targetdir"/usr/share/applications "$targetdir"/usr/share/doc/"$program"

cp "$program" "$targetdir"/usr/bin/

cp images/icon/scalable/easyhg-icon.svg "$targetdir"/usr/share/pixmaps/
cp images/icon/128/easyhg-icon.png "$targetdir"/usr/share/pixmaps/
cp deploy/linux/"$program".desktop "$targetdir"/usr/share/applications/
cp README.txt "$targetdir"/usr/share/doc/"$program"/

perl -i -p -e "s/Architecture: .*/Architecture: $arch/" "$targetdir"/DEBIAN/control

deps=`bash "$depdir"/debian-dependencies.sh "$program"`

perl -i -p -e "s/Depends: .*/$deps/" "$targetdir"/DEBIAN/control

control_ver=${version%-?}

perl -i -p -e "s/Version: .*/Version: $control_ver/" "$targetdir"/DEBIAN/control

bash "$depdir"/fix-lintian-bits.sh "$targetdir"

dpkg-deb --build "$targetdir" && lintian "$targetdir".deb

