mkdir -p 10.5/src
cd 10.5
cp -a ../EasyMercurial.app .
lipo ../EasyMercurial.app/Contents/MacOS/EasyMercurial -remove x86_64 -output ./EasyMercurial.app/Contents/MacOS/EasyMercurial
cp -a ../src/version.h src/
cp -a ../COPYING .
bash ../deploy/osx/deploy.sh EasyMercurial.app EasyMercurial_leopard
cp *_leopard*.dmg ..
cd ..
rm -r 10.5
echo Done


