#Basic checks

ifndef TEST_TOP_DIR
dummy:
	echo "TEST_TOP_DIR is not defined in your makefile"
endif

ifneq ($(OS),Linux)
  ifneq ($(OS),MacOSX)
    ifndef PTHREADS_DIR
dummy:
	echo "you must define PTHREADS_DIR before you can compile"

    endif
  endif
endif



