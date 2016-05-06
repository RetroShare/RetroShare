#!/usr/bin/env sh

# create dummy webfiles at qmake run

if [ "$1" = "" ];then
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
touch $publicdest/app.js -d 1970-01-01
touch $publicdest/app.css -d 1970-01-01
touch $publicdest/index.html -d 1970-01-01
