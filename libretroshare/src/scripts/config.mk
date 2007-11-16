
# determine which operating system
# 
###########################################################################
#Define OS.
#
OS = Linux
#OS = Cygwin
#OS = Win # MinGw.
###########################################################################

ifeq ($(OS),Linux)
  include $(RS_TOP_DIR)/scripts/config-linux.mk
else
  ifeq ($(OS),Cygwin)
    include $(RS_TOP_DIR)/scripts/config-cygwin.mk
  else
    include $(RS_TOP_DIR)/scripts/config-mingw.mk
  endif
endif

###########################################################################
