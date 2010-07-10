
ifneq ($(OS),Linux)
dummy:
	echo "ERROR Linux configuration file included, but (OS != Linux)

endif

############   LINUX CONFIGURATION    ########################

include $(TEST_TOP_DIR)/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING ########################

CC = g++
RM = /bin/rm
RANLIB = ranlib
LIBDIR = $(LIB_TOP_DIR)/lib
LIBRS = $(LIBDIR)/libretroshare.a

# Unix: Linux/Cygwin
INCLUDE = -I $(LIB_TOP_DIR)
CFLAGS = -Wall -g $(INCLUDE) 
CFLAGS += ${DEFINES} -D BE_DEBUG

#########################################################################
# OS Compile Options
#########################################################################

#########################################################################
# OS specific Linking.
#########################################################################

LIBS =  -L$(LIBDIR) -lbitdht
LIBS +=  -lpthread
#LIBS +=  $(XLIB) -ldl -lz 
#LIBS +=  -lupnp
#LIBS +=  -lgpgme
#	
#RSLIBS = $(LIBS)


