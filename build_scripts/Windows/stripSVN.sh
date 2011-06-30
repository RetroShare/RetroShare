#!/bin/sh


if [ $# -le 0 ]
then
	echo usage $0 directory
	exit
fi

echo find $1 -name .svn
rm -vrf `find $1 -name .svn`

