#!/usr/bin/env sh

# create webfiles from sources at compile time (works without npm/node.js)

if [ "$1" = "" ];then
	publicdest=../../webui
	src=..
else
	publicdest=$1/webui
	src=$1/webui-src
fi

if [ -d "$publicdest" ]; then
	echo remove existing $publicdest
	rm $publicdest -R
fi

echo mkdir $publicdest
mkdir $publicdest

echo building app.js
echo - copy template.js ...
cp $src/make-src/template.js $publicdest/app.js

for filename in $src/app/*.js; do
	fname=$(basename "$filename")
	fname="${fname%.*}"
	echo - adding $fname ...
	echo require.register\(\"$fname\", function\(exports, require, module\) { >> $publicdest/app.js
	cat $filename >> $publicdest/app.js
	echo >> $publicdest/app.js
	echo }\)\; >> $publicdest/app.js
done

echo building app.css
cat $src/app/green-black.scss >> $publicdest/app.css
cat $src/make-src/main.css >> $publicdest/app.css
cat $src/make-src/chat.css >> $publicdest/app.css

echo copy index.html
cp $src/app/assets/index.html $publicdest/index.html

echo build.sh complete
