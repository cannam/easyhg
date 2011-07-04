#!/bin/bash

# Carbon (10.4+ PPC/i386). 
#
$HOME/qt-builds/qt-471-carbon-10.4u/bin/qmake -spec macx-g++40 || exit 
make clean && make && \
	cp EasyMercurial.app/Contents/MacOS/EasyMercurial \
	   EasyMercurial.carbon.app/Contents/MacOS/ && \
	bash deploy/osx/paths.sh EasyMercurial.carbon

# Cocoa (10.6+ x86_64)
#
/usr/bin/qmake -spec macx-g++ || exit
make clean && make && \
        cp EasyMercurial.app/Contents/MacOS/EasyMercurial \
           EasyMercurial.cocoa.app/Contents/MacOS/ && \
	bash deploy/osx/paths.sh EasyMercurial.cocoa

# Complicated lipo business
#
bash deploy/osx/combine.sh || exit 1

# Info.plist etc and make dmg
bash deploy/osx/deploy.sh EasyMercurial.app EasyMercurial




