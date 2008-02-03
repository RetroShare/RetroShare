#Basic checks

ifndef RS_TOP_DIR
dummy:
	echo "RS_TOP_DIR is not defined in your makefile"
endif

ifndef SSL_DIR
dummy:
	echo "you must define SSL_DIR before you can compile"

endif

ifneq ($(OS),Linux)
  ifneq ($(OS),MacOSX)
    ifndef PTHREADS_DIR
dummy:
	echo "you must define PTHREADS_DIR before you can compile"

    endif
  endif
endif



