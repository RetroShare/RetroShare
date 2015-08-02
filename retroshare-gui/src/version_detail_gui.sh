#!/bin/bash

source ../../get_version.sh

echo "Retroshare Gui version : $version" > gui/help/version.html
	
echo "Git Hash : $githash" >> gui/help/version.html
echo "Git info : $gitinfo" >> gui/help/version.html
echo "Git branch : $gitbranch" >> gui/help/version.html
date >> gui/help/version.html
echo "" >> gui/help/version.html
echo "" >> gui/help/version.html

echo "version_detail_gui.sh script finished"
exit 0
