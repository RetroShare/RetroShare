clear
echo In \"control\" versionsnummer angepasst ?
echo \"changelog.Debian.gz\" angepasst ?
echo In dieses Skript den Ausgabedateinamen angepasst ?
echo
echo retroshare-nogui soll auch standalone laufen
echo \(1\) delete src/miniupnpc-1.0/libminiupnpc.so 
echo \(2\) delete lib/libretroshare.lib
echo \(3\) remake libretroshare, and the execs 

echo cd src/retroshare-svn/libretroshare/src 
echo retroshare-nogui loeschen
echo make clean und dann  make
echo
echo ENTER

read

cd retroshare-package-v0.4.04b
#./compile_rs_latest_svn.sh

cd ..
read

cd retroshare
find usr -type f -exec md5sum {} \; > DEBIAN/md5sums
cd ..
dpkg-deb -b retroshare RetroShare_0.4.10a_ubuntu_gutsy.deb