#!/bin/sh
for i in `ls retroshare06_0.6.0-1.*.changes` ; do
	dput ppa:retroshare/unstable $i
done
