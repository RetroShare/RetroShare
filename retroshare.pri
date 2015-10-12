# Gxs is always enabled now.
DEFINES *= RS_ENABLE_GXS

unix {
	isEmpty(PREFIX)   { PREFIX   = "/usr" }
	isEmpty(BIN_DIR)  { BIN_DIR  = "$${PREFIX}/bin" }
	isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include/retroshare06" }
	isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/RetroShare06" }
	isEmpty(PLUGIN_DIR) { PLUGIN_DIR  = "$${LIB_DIR}/retroshare/extensions6" }
}

unfinished {
	CONFIG += gxscircles
	CONFIG += gxsthewire
	CONFIG += gxsphotoshare
	CONFIG += wikipoos
}

wikipoos:DEFINES *= RS_USE_WIKI
