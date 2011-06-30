
# determine which operating system
# 
###########################################################################
#Define OS.
#
OS = Linux
#OS = MacOSX
#OS = Cygwin
#OS = Win # MinGw.
###########################################################################

ifeq ($(OS),Linux)
  include $(TEST_TOP_DIR)/scripts/config-linux.mk
else
  ifeq ($(OS),MacOSX)
    include $(TEST_TOP_DIR)/scripts/config-macosx.mk
  else
    ifeq ($(OS),Cygwin)
      include $(TEST_TOP_DIR)/scripts/config-cygwin.mk
    else
      include $(TEST_TOP_DIR)/scripts/config-mingw.mk
    endif
  endif
endif

###########################################################################
