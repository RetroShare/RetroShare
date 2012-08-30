#!/bin/sh

tmpfile=/tmp/toto42314321

cat "$1" | grep -v "CONFIG += version_detail_bash_script" > $tmpfile
echo "INCLUDEPATH += ../../libretroshare ../../retroshare-gui" > $1
cat $tmpfile >> $1


