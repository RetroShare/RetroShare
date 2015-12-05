TCL_VERSION=8.6.2
SQLCIPHER_VERSION=2.2.1

all: dirs sqlcipher copylibs

dirs:
	mkdir -p libs/include
	mkdir -p libs/lib
	mkdir -p libs/bin

tcl$(TCL_VERSION)-src.tar.gz:
	curl.exe -L http://prdownloads.sourceforge.net/tcl/tcl$(TCL_VERSION)-src.tar.gz -o tcl$(TCL_VERSION)-src.tar.gz

sqlcipher-$(SQLCIPHER_VERSION).tar.gz:
	curl.exe -L -k https://github.com/sqlcipher/sqlcipher/archive/v$(SQLCIPHER_VERSION).tar.gz -o sqlcipher-$(SQLCIPHER_VERSION).tar.gz

sqlcipher: tcl$(TCL_VERSION)-src.tar.gz sqlcipher-$(SQLCIPHER_VERSION).tar.gz
	# tcl
	tar xvf tcl$(TCL_VERSION)-src.tar.gz
	mkdir -p tcl$(TCL_VERSION)/build
	cd tcl$(TCL_VERSION)/build && ../win/configure
	cd tcl$(TCL_VERSION)/build && make
	#sqlcipher
	tar xvf sqlcipher-$(SQLCIPHER_VERSION).tar.gz
	cd sqlcipher-$(SQLCIPHER_VERSION) && ln -s ../tcl$(TCL_VERSION)/build/tclsh86.exe tclsh
	mkdir -p tcl$(TCL_VERSION)/lib
	ln -s `pwd`/tcl$(TCL_VERSION)/library `pwd`/tcl$(TCL_VERSION)/lib/tcl8.6
	cd sqlcipher-$(SQLCIPHER_VERSION) && PATH=$$PATH:`pwd`/../tcl$(TCL_VERSION)/build && LIBS="-L`pwd`/../libs/lib -lgdi32 $$LIBS" && export LIBS && ./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC -I`pwd`/../libs/include -I`pwd`/../tcl$(TCL_VERSION)/generic" LDFLAGS="-L`pwd`/../libs/lib -lcrypto -lgdi32" --with-tcl="`pwd`/../tcl$(TCL_VERSION)/build" && make install prefix="`pwd`/install"
	cp -r sqlcipher-$(SQLCIPHER_VERSION)/install/include/* libs/include/
	cp sqlcipher-$(SQLCIPHER_VERSION)/install/lib/libsqlcipher.a libs/lib/
	cp sqlcipher-$(SQLCIPHER_VERSION)/install/bin/sqlcipher.exe libs/bin/
	rm -r -f sqlcipher-$(SQLCIPHER_VERSION)
	rm -r -f tcl$(TCL_VERSION)
	touch sqlcipher

copylibs: 
	read -p "Do you want to copy libs to retroshare? (yes|no)" answer; \
	if [ "$$answer" = "yes" ] ; then  \
	  cp -r libs ../../ ; \
	fi

