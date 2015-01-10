This directory contains the material for building a debian/ubuntu source package from 
the svn repository of RetroShare. The various files here are:

  BaseRetroShareDirs.tgz: basic directory structure. Does not contain any source file.
  makeSourcePackage.sh  : main script. Produces source binary debian package (.dsc+.tgz files)
                          from the base directory structure and the sources in the svn repository.
  clean.sh              : compiles the source package. Produce a binary pkg for a given ubuntu distribution
  make_tgz.sh           : rebuilds the basic directory structure if some config/debian files where modified
                          in the retroshare-0.5 directory

The pipeline is the following:

      [This directory]   =>    [Create a source package]   =>   [Compile the package for various ubuntu/debian and various arch]

How do I create a source package?
=================================

   Use the script!

      > ./makeSourcePackage.sh  [-h for options]

      The script gets the svn number, grabs the sources from the svn
      repository, removed .svn files from it, and calls debuild to create a
      source package. 

      You might:
         - specify a given svn revision (usually not needed)
         - specify a list of distribs to build for

      Example:

         > ./makeSourcePackage.sh -distribution "precise wheezy trusty"

      You should get as many source packages as wanted distributions. For each
      of them you get (example here for wheezy):

         retroshare06_0.6.0-0.7829~wheezy.dsc
         retroshare06_0.6.0-0.7829~wheezy_source.build
         retroshare06_0.6.0-0.7829~wheezy_source.changes
         retroshare06_0.6.0-0.7829~wheezy.tar.gz
         
How do I create a binary package?
=================================

*Initial steps*

   1 - You need to use/install pbuilder-dist:

      > sudo apt-get install ubuntu-dev-tools
   
   2 - To do once: Init the building environment for each distribution you need:

      > pbuilder-dist wheezy amd64 create

   Then, when needed, keep t up to date (this solved almost all compiling
   issues you might have:

      > pbuilder-dist wheezy amd64 update

   3 - For ubuntu distribs, add a ppa dependency for sqlcipher:

      * add this line to ~/.pbuilderrc: OTHERMIRROR="deb http://ppa.launchpad.net/guardianproject/ppa/ubuntu precise main"
      * add the ppa to the build environment:

         > pbuilder-dist precise login --save-after-login
         # apt-key adv --keyserver pgp.mit.edu --recv-keys  2234F563
         # exit
         > pbuilder-dist precise update --release-only         # the --release-only is really required!

*Package compilation*

      > pbuilder-dist wheezy build retroshare06_0.6.0~7856~wheezy.dsc

   The generated compiled binary packages (including all package plugins etc)
   will  be in ~/pbuilder/wheezy_result/


