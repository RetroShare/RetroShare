This directory contains the material for buildign a debian/ubuntu source package from 
the svn repository of RetroShare. The various files here are:

  BaseRetroShareDirs.tgz: basic directory structure. Does not contain any source file.
  makeSourcePackage.sh  : main script. Produces source binary debian package (.dsc+.tgz files)
                          from the base directory structure and the sources in the svn repository.
  clean.sh              : compiles the source package. Produce a binary pkg for a given ubuntu distribution
  make_tgz.sh           : rebuilds the basic directory structure if some config/debian files where modified
                          in the retroshare-0.5 directory

Notes about pbuilder
====================

To add a ppa dependency:
	- add this line to ~/.pbuilderrc: OTHERMIRROR="deb http://ppa.launchpad.net/guardianproject/ppa/ubuntu precise main"

			pbuilder-dist precise login --save-after-login
			# apt-key adv --keyserver pgp.mit.edu --recv-keys  2234F563
			# exit
			pbuilder-dist precise update --release-only			# the --release-only is really required!

