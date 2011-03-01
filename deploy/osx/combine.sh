#!/bin/bash
app=EasyMercurial.app
if [ ! -d "$app.carbon" ] || [ ! -d "$app.cocoa" ]; then
	echo Carbon or Cocoa bundle not found
	exit 1
fi
(cd "$app.cocoa" ; find . -type f -print) | while read f; do
        d=$(dirname "$f")
        mkdir -p "$app.output/$d"
	case $(file "$app.cocoa/$f") in
	*x86_64*)
		lipo "$app.cocoa/$f" -extract x86_64 -output "/tmp/$$.x86_64"
		case $(file "$app.carbon/$f") in
		*x86_64*)
			lipo "$app.carbon/$f" -remove x86_64 -output "/tmp/$$.rest"
			;;
		*)
			cp "$app.carbon/$f" "/tmp/$$.rest"
			;;
		esac
		lipo "/tmp/$$.x86_64" "/tmp/$$.rest" -create -output "$app.output/$f"
		rm "/tmp/$$".*
		;;
	*)
		cp "$app.cocoa/$f" "$app.output/$f"
		;;
	esac
done


