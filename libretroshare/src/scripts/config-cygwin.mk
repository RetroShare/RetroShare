ifneq ($(OS),Cygwin)
dummy:
	echo "ERROR Cygwin configuration file included, but (OS != Cygwin)

endif

############   LINUX CONFIGURATION    ########################

# flags for components....
PQI_USE_XPGP = 1
#PQI_USE_PROXY = 1
#PQI_USE_CHANNELS = 1
#USE_FILELOOK = 1

###########################################################################

ALT_SRC_ROOT=/cygdrive/c/RetroShareBuild/src
SRC_ROOT=/cygdrive/c/RetroShareBuild/src

# These never change.
PTHREADS_DIR=$(ALT_SRC_ROOT)/pthreads-w32-2-8-0-release
ZLIB_DIR=$(ALT_SRC_ROOT)/zlib-1.2.3

# pretty stable...
SSL_DIR=$(SRC_ROOT)/openssl-0.9.7g-xpgp-0.1c
UPNPC_DIR=$(SRC_ROOT)/miniupnpc-1.0

include $(RS_TOP_DIR)/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING ########################

CC = g++
RM = /bin/rm
RANLIB = ranlib
LIBDIR = $(RS_TOP_DIR)/lib
LIBRS = $(LIBDIR)/libretroshare.a

# Unix: Linux/Cygwin
INCLUDE = -I $(RS_TOP_DIR) 

ifdef PQI_DEBUG
	CFLAGS = -Wall -g $(INCLUDE) 
else
	CFLAGS = -Wall -O2 $(INCLUDE) 
endif

ifdef PQI_USE_XPGP
	INCLUDE += -I $(SSL_DIR)/include 
endif

ifdef PQI_USE_XPGP
	CFLAGS += -DPQI_USE_XPGP
endif

ifdef PQI_USE_PROXY
	CFLAGS += -DPQI_USE_PROXY
endif

ifdef PQI_USE_CHANNELS
	CFLAGS += -DPQI_USE_CHANNELS
endif

ifdef USE_FILELOOK
	CFLAGS += -DUSE_FILELOOK
endif


RSCFLAGS = -Wall -g $(INCLUDE) 

#########################################################################
# OS Compile Options
#########################################################################

# For the SSL BIO compilation. (Copied from OpenSSL compilation flags)
BIOCC  = gcc

# Cygwin - ?same? as Linux flags
BIOCFLAGS =  -I $(SSL_DIR)/include -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H -DOPENSSL_NO_KRB5 -DL_ENDIAN -DTERMIO -O3 -fomit-frame-pointer -m486 -Wall -DSHA1_ASM -DMD5_ASM -DRMD160_ASM

#########################################################################
# OS specific Linking.
#########################################################################

# for static pthread libs....
WININC += -DPTW32_STATIC_LIB
WININC += -mno-cygwin -mwindows -fno-exceptions 
WININC += -DWINDOWS_SYS  

WINLIB = -lws2_32 -luuid -lole32 -liphlpapi 
WINLIB += -lcrypt32 -lwinmm

CFLAGS += -I$(PTHREADS_DIR) $(WININC)
CFLAGS += -I$(ZLIB_DIR)

LIBS =  -L$(LIBDIR) -lretroshare 
ifdef PQI_USE_XPGP
	LIBS +=  -L$(SSL_DIR) 
endif

LIBS +=  -lssl -lcrypto 
LIBS +=  -L$(UPNPC_DIR) -lminiupnpc
LIBS += -L$(ZLIB_DIR) -lz 
LIBS += -L$(PTHREADS_DIR) -lpthreadGC2d 
LIBS += $(WINLIB) 

RSCFLAGS += $(WININC)


