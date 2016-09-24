#!/usr/bin/python

# Requires packages
#	python-launchpad-integration               
#	python-launchpadlib        

from launchpadlib.launchpad import Launchpad
PPAOWNER = "csoler-users" #the launchpad PPA owener. It's usually the first part of a PPA. Example: in "webupd8team/vlmc", the owener is "webupd8team".

distribs = ['jaunty','karmic','lucid','maverick','natty']
archs = ['i386','amd64']
ppas = ['retroshare','retroshare-snapshots']
total = 0


for PPA in ppas:
	for distrib in distribs:
		for arch in archs:
			desired_dist_and_arch = 'https://api.edge.launchpad.net/devel/ubuntu/' + distrib + '/' + arch 
	#here, edit "maverick" and "i386" with the Ubuntu version and desired arhitecture
	
			cachedir = "~/.launchpadlib/cache/"
			lp_ = Launchpad.login_anonymously('ppastats', 'edge', cachedir, version='devel')
			owner = lp_.people[PPAOWNER]
			archive = owner.getPPAByName(name=PPA)
	
			for individualarchive in archive.getPublishedBinaries(status='Published',distro_arch_series=desired_dist_and_arch):
				print PPA + "\t" + arch + "\t" + individualarchive.binary_package_version + "\t" + str(individualarchive.getDownloadCount())
				total += individualarchive.getDownloadCount()
	
print "Total downloads: " + str(total)

