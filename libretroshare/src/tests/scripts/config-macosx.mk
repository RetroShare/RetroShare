
ifneq ($(OS),MacOSX)
dummy:
	echo "ERROR MacOSX configuration file included, but (OS != MacOSX)

endif

############   LINUX CONFIGURATION    ########################


# FLAGS to decide if we want i386 Build or ppc Build
# 
# 

# PPC is default 
# Could define both for combined compilation...
# except might not work for bio_tou.c file! 
# 
# MAC_I386_BUILD = 1
# MAC_PPC_BUILD = 1

MAC_I386_BUILD = 1
#MAC_PPC_BUILD = 1

ifndef MAC_I386_BUILD
	MAC_PPC_BUILD = 1
endif

UPNPC_DIR=../../../../../../src/miniupnpc-1.0

include $(RS_TOP_DIR)/tests/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING ########################

CC = g++
RM = /bin/rm

RANLIB = ranlib

# Dummy ranlib -> can't do it until afterwards with universal binaries.
# RANLIB = ls -l 

BITDIR = $(DHT_TOP_DIR)/lib
LIBDIR = $(RS_TOP_DIR)/lib
LIBRS = $(LIBDIR)/libretroshare.a

OPT_DIR = /opt/local
OPT_INCLUDE = $(OPT_DIR)/include
OPT_LIBS = $(OPT_DIR)/lib

INCLUDE = -I $(RS_TOP_DIR)  -I $(OPT_INCLUDE)
#CFLAGS = -Wall -O3 
CFLAGS = -Wall -g

# Flags for architecture builds. 
ifdef MAC_I386_BUILD
	CFLAGS += -arch i386 
endif

ifdef MAC_PPC_BUILD
	CFLAGS += -arch ppc 
endif

CFLAGS += $(INCLUDE) 

# This Line is for Universal BUILD for 10.4 + 10.5 
# (but unlikely to work unless Qt Libraries are build properly)
#CFLAGS += -isysroot /Developer/SDKs/MacOSX10.5.sdk 

# RSCFLAGS = -Wall -O3 $(INCLUDE) 

#########################################################################
# OS Compile Options
#########################################################################

#########################################################################
# OS specific Linking.
#########################################################################

LIBS = -Wl,-search_paths_first

# for Univeral BUILD
# LIBS += -arch ppc -arch i386
#LIBS += -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5u.sdk 

LIBS =  -lgpgme -L$(LIBDIR) -lretroshare
LIBS +=  -L$(BITDIR) -lbitdht 
ifdef PQI_USE_XPGP
        LIBS +=  -L$(SSL_DIR)
  endif
LIBS +=  -lssl -lcrypto  -lpthread
LIBS +=  -L$(UPNPC_DIR) -lminiupnpc
LIBS +=  $(XLIB) -ldl -lz



