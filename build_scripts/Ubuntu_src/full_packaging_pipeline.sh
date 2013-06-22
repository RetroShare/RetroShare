#!/bin/sh

./clean.sh
\rm -rf retroshare-0.5.4
./makeSourcePackage.sh

pbuilder-dist precise build retroshare_0.5.4-0.????~precise.dsc
