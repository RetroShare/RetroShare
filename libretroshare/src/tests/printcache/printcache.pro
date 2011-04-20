TEMPLATE = app
CONFIG = debug

SOURCES = main.cpp

INCLUDEPATH *= ../..
LIBS = -lstdc++ -lm ../../lib/libretroshare.a ../../../../libbitdht/src/lib/libbitdht.a \
		 -lssl -lcrypto -lgpgme -lupnp -lgnome-keyring

