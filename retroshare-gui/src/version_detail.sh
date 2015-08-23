#!/bin/bash
#check if we're on *nix system
#write the version.html file
#update the rsversion.h file
#don't exit even if a command fails
set +e
if (ls &> /dev/null); then
		#retrieve git information
		#Obtain Current RetroShare base filename from Git values
		svnversion=$(($(git rev-list --count HEAD)+951))
 		commitdate=$(git log -1 --date=short --pretty=format:%cd)
 		cdate="${commitdate//[-]/}" # Eliminate Hypens in Date
		tdate=${cdate:2}
 		commithash=$(git log --format="%H" -n 1)
 		hsnippet=`expr substr $commithash 1 6` #First 6 characters
 		gitinfo=$(git describe --tags)
 		gitsnip=`expr substr $gitinfo 1 7` #Select first 7 characters
		gittag=${gitinfo:1:6}
 		now=$gittag$cdate"_"$hsnippet
 		basename="Retroshare"$now$extGui
 		#Retroshare0.6.0-20150818_42bbf7 basename if $extGui empty
		echo "Retroshare Gui version : $basename" > gui/help/version.html
		#Calculate Sequentual Version Number based on total Master Branch Commits
		#8681 - Example
		echo "Svn version : $svnversion" >> gui/help/version.html
		#Provide current,accurate RFC 2822 format timestamps from latest Commit used for the build listed in About - About
		#Tue, 18 Aug 2015 20:21:45 -0400 - Example
		echo $(git log -1 --date=rfc --pretty=format:%cd) >> gui/help/version.html
		echo "" >> gui/help/version.html
		echo "" >> gui/help/version.html
		cd ../../libretroshare/src/retroshare
		#Write Trimmed Date of Latest Commit for Internal Revision
		REVISIONUMBER="#define RS_REVISION_NUMBER""   0x"$tdate
		sed -i '12d' rsversion.h
		echo $REVISIONUMBER >> rsversion.h
		cd ../../retroshare-gui/src
fi
echo "version_detail.sh scripts finished"
exit 0
