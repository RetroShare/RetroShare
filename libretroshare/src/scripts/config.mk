
# determine which operating system
# 
###########################################################################
#Define OS.
#
#OS = Linux
#OS = MacOSX
OS = Cygwin
#OS = Win # MinGw.
###########################################################################

ifeq ($(OS),Linux)
  include $(RS_TOP_DIR)/scripts/config-linux.mk
else
  ifeq ($(OS),MacOSX)
    include $(RS_TOP_DIR)/scripts/config-macosx.mk
  else
    ifeq ($(OS),Cygwin)
      include $(RS_TOP_DIR)/scripts/config-cygwin.mk
    else
      include $(RS_TOP_DIR)/scripts/config-mingw.mk
    endif
  endif
endif

###########################################################################
