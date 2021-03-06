#!/bin/bash

set -eu

program=EasyMercurial
kdiff=easyhg-kdiff3
merge=easyhg-merge.sh
extdiff=easyhg-extdiff.sh

programs="$program $kdiff $merge $extdiff"

get_id() {
    if [ -d .hg ]; then
        hg id | sed 's/[+ ].*$//'
    elif [ -d .git ]; then
        git rev-parse --short HEAD
    else
        echo "WARNING: can't figure out revision from VCS metadata" 1>&2
        echo "unknown"
    fi
}

version=$(get_id)

if [ ! -x ./appimagetool-x86_64.AppImage ]; then
    echo "Failed to find executable ./appimagetool-x86_64.AppImage, please provide that first"
    exit 2
fi

for p in $programs; do
    if [ ! -x "$p" ]; then
        echo "Failed to find executable ./$p, please build and check"
        exit 2
    fi
done

targetdir="${program}.AppDir"

echo "Target dir is $targetdir"

if [ -d "$targetdir" ]; then
    echo "Target directory exists, not overwriting"
    exit 2
fi

mkdir "$targetdir"

mkdir -p "$targetdir"/usr/bin
mkdir -p "$targetdir"/usr/lib

cp $programs "$targetdir"/usr/bin/

add_dependencies() {

    local binary="$1"

    echo "ldd $binary yields:"
    ldd "$binary"
    
    for lib in $(ldd "$binary" | grep '=> [^ ]*/lib/' | \
                     sed 's/^.*=> //' | sed 's/ .*$//'); do

        base=$(basename "$lib")
        if grep -v '^#' deploy/linux/appimage-excludelist |
                grep -q "^$base$" ; then
            echo "excluding: $lib"
            continue
        fi

        target="$targetdir/usr/lib/$(basename $lib)"
        
        mkdir -p "$(dirname $target)"

        if [ ! -f "$target" ]; then

            cp -Lv "$lib" "$target"
            chmod +x "$target"
            
            add_dependencies "$lib"
            
        fi
    done
}

add_dependencies "$program"
add_dependencies "$kdiff"

qtplugins="gif icns ico jpeg tga tiff wbmp webp cocoa minimal offscreen xcb"
qtlibdirs="/usr/lib/x86_64-linux-gnu/qt5 /usr/lib/x86_64-linux-gnu/qt /usr/lib/qt5 /usr/lib/qt"

QTDIR=${QTDIR:-}
if [ -n "$QTDIR" ]; then
    qtlibdirs="$QTDIR $qtlibdirs"
fi

for plug in $qtplugins; do
    for libdir in $qtlibdirs; do
        lib=$(find $libdir/plugins -name libq$plug.so -print 2>/dev/null || true)
        if [ -n "$lib" ]; then
            if [ -f "$lib" ]; then
                subdir=$(basename $(dirname $lib))
                if [ t"$subdir" = t"plugins" ]; then
                    subdir=""
                fi
                target="$targetdir/usr/lib/qt5/plugins/$subdir/$(basename $lib)"
                mkdir -p "$(dirname $target)"
                cp -v "$lib" "$target"
                chmod +x "$target"
                add_dependencies "$lib"
                break
            fi
        fi
    done
done

cp "deploy/linux/$program.desktop" "$targetdir/"

cp images/icon/scalable/easyhg-icon.svg "$targetdir/"
cp images/icon/scalable/easyhg-icon.svg "$targetdir/$program.svg"
cp images/icon/128/easyhg-icon.png "$targetdir/"

cp "deploy/linux/AppRun" "$targetdir/"

chmod +x "$targetdir/AppRun"

# Do this with a separate extraction step, so as to make it work even
# in situations where FUSE is unavailable like in a Docker container
export ARCH=x86_64
./appimagetool-x86_64.AppImage --appimage-extract
./squashfs-root/AppRun "$targetdir" "$program-$version-x86_64.AppImage"
