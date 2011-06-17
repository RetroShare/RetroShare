TEMPLATE = lib
CONFIG = 

linux-g++ {
	INCLUDEPATH += ../../libretroshare/src/ ../../retroshare-gui/src/
	LIBS *= -ldl
}

