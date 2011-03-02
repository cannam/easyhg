#!/bin/bash
app=EasyMercurial
if [ ! -d "$app.carbon.app" ] || [ ! -d "$app.cocoa.app" ]; then
	echo Carbon or Cocoa bundle not found
	exit 1
fi
(cd "$app.cocoa.app" ; find . -type f -print) | while read f; do
        d=$(dirname "$f")
        mkdir -p "$app.app/$d"
	case $(file "$app.cocoa.app/$f") in
	*universal*x86_64*)
		lipo "$app.cocoa.app/$f" -extract x86_64 -output "/tmp/$$.x86_64";;
	*x86_64*)
		lipo "$app.cocoa.app/$f" -create -output "/tmp/$$.x86_64";;
	*)
		cp "$app.cocoa.app/$f" "$app.app/$f"
		continue;;
	esac
	case $(file "$app.carbon.app/$f") in
	*x86_64*)
		lipo "$app.carbon.app/$f" -remove x86_64 -output "/tmp/$$.rest"
		;;
	*universal*)
		cp "$app.carbon.app/$f" "/tmp/$$.rest"
		;;
	*)
		lipo "$app.carbon.app/$f" -create -output "/tmp/$$.rest"
		;;
	esac
	lipo "/tmp/$$.x86_64" "/tmp/$$.rest" -create -output "$app.app/$f"
	rm "/tmp/$$".*
done


