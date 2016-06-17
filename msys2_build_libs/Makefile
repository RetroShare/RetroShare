SQLCIPHER_VERSION=3.3.1
MSYS2AUTOMAKE=/c/msys64/usr/share/automake-1.15

BLD=../build-libs
DLD=../downloads

all: dirs sqlcipher

dirs:
	mkdir -p ../../libs/include
	mkdir -p ../../libs/lib
	mkdir -p ../../libs/bin
	mkdir -p $(DLD)
	mkdir -p $(BLD)

sqlcipher: | ../../libs/lib/libsqlcipher.a

$(DLD)/sqlcipher-$(SQLCIPHER_VERSION).tar.gz:
	curl.exe -L -k https://github.com/sqlcipher/sqlcipher/archive/v$(SQLCIPHER_VERSION).tar.gz -o $(DLD)/sqlcipher-$(SQLCIPHER_VERSION).tar.gz

../../libs/lib/libsqlcipher.a: $(DLD)/sqlcipher-$(SQLCIPHER_VERSION).tar.gz
	tar xvf $(DLD)/sqlcipher-$(SQLCIPHER_VERSION).tar.gz -C $(BLD)
	cp $(MSYS2AUTOMAKE)/config.guess $(BLD)/sqlcipher-$(SQLCIPHER_VERSION)/config.guess
	cp $(MSYS2AUTOMAKE)/config.sub $(BLD)/sqlcipher-$(SQLCIPHER_VERSION)/config.sub
	cd $(BLD)/sqlcipher-$(SQLCIPHER_VERSION) && LIBS="-L`pwd`/../../libs/lib -lgdi32 $$LIBS" && export LIBS && ./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC -I`pwd`/../../libs/include" LDFLAGS="-L`pwd`/../../libs/lib -lcrypto -lgdi32" && make install prefix="`pwd`/install"
	cp -r $(BLD)/sqlcipher-$(SQLCIPHER_VERSION)/install/include/* ../../libs/include/
	cp $(BLD)/sqlcipher-$(SQLCIPHER_VERSION)/install/lib/libsqlcipher.a ../../libs/lib/
	cp $(BLD)/sqlcipher-$(SQLCIPHER_VERSION)/install/bin/sqlcipher.exe ../../libs/bin/
