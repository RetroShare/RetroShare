#!/bin/sh

./clean.sh
\rm -rf retroshare-0.5.5
./makeSourcePackage.sh

pbuilder-dist precise build retroshare_0.5.5-0.????~precise.dsc
