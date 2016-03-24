#!/usr/bin/sh

# create dummy webfiles at qmake run

if [ "$1" == "" ];then
	publicdest=../../webui
else
	publicdest=$1/webui
fi

if [ -d "$publicdest" ]; then
	echo remove $publicdest
	rm $publicdest -R
fi

echo create $publicdest
mkdir $publicdest

echo touch $publicdest/app.js, $publicdest/app.css, $publicdest/index.html
touch $publicdest/app.js
touch $publicdest/app.css
touch $publicdest/index.html
