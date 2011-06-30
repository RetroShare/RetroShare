TEMPLATE = lib

INCLUDEPATH += ../../libretroshare/src/ ../../retroshare-gui/src/

linux-g++ {
	LIBS *= -ldl
}
linux-g++-64 {
	LIBS *= -ldl
}

