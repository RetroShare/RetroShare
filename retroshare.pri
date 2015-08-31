unix {
	isEmpty(PREFIX)   { PREFIX   = "/usr" }
	isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include/retroshare06" }
	isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/RetroShare06" }
}
