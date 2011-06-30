#ifneq ($(OS),"Win ")
#dummy:
#	echo "ERROR OS = $(OS)"
#	echo "ERROR MinGW configuration file included, but (OS != Win)
#
#endif

############   LINUX CONFIGURATION    ########################

# flags for components....
#PQI_USE_XPGP = 1
#PQI_USE_PROXY = 1
#PQI_USE_CHANNELS = 1
#USE_FILELOOK = 1

###########################################################################

#### DrBobs Versions.... Please Don't Delete.
### Comment out if needed.
SRC_ROOT_PKG=/home/Mark/prog/retroshare/package/rs-win-v0.5.0/src
SRC_ROOT_GPG=/local

#ALT_SRC_ROOT=/cygdrive/c/home/rmfern/prog/MinGW
#SRC_ROOT=../../../..

PTHREADS_DIR=$(SRC_ROOT_PKG)/pthreads-w32-2-8-0/Pre-built.2
ZLIB_DIR=$(SRC_ROOT_PKG)/zlib-1.2.3
SSL_DIR=$(SRC_ROOT_PKG)/openssl-tmp
UPNPC_DIR=$(SRC_ROOT_PKG)/miniupnpc-1.3

###########################################################################

#### Enable this section for compiling with MSYS/MINGW compile
#SRC_ROOT=/home/linux

#SSL_DIR=$(SRC_ROOT)/OpenSSL
#GPGME_DIR=$(SRC_ROOT)/gpgme-1.1.8
#GPG_ERROR_DIR=$(SRC_ROOT)/libgpg-error-1.7

#ZLIB_DIR=$(SRC_ROOT)/zlib-1.2.3
#UPNPC_DIR=$(SRC_ROOT)/miniupnpc-1.0
#PTHREADS_DIR=$(SRC_ROOT)/pthreads-w32-2-8-0-release

include $(RS_TOP_DIR)/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING #######################################

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

# These aren't used anymore.... really.
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


# SSL / pthreads /  Zlib 
# included by default for Windows compilation.
INCLUDE += -I $(SSL_DIR)/include 
INCLUDE += -I$(PTHREADS_DIR) 
INCLUDE += -I$(ZLIB_DIR)


#########################################################################
# OS Compile Options
#########################################################################

# For the SSL BIO compilation. (Copied from OpenSSL compilation flags)
BIOCC  = gcc

# Cygwin - ?same? as Linux flags
BIOCFLAGS =  -I $(SSL_DIR)/include -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H -DOPENSSL_NO_KRB5 -DL_ENDIAN -DTERMIO -O3 -fomit-frame-pointer -m486 -Wall -DSHA1_ASM -DMD5_ASM -DRMD160_ASM 
BIOCFLAGS += -DWINDOWS_SYS

#########################################################################
# OS specific Linking.
#########################################################################

# for static pthread libs....
#WININC += -DPTW32_STATIC_LIB
#WININC += -mno-cygwin -mwindows -fno-exceptions 

WININC += -DWINDOWS_SYS  

WINLIB = -lws2_32 -luuid -lole32 -liphlpapi 
WINLIB += -lcrypt32 -lwinmm

CFLAGS += -I$(SSL_DIR)/include
CFLAGS += -I$(PTHREADS_DIR)/include
CFLAGS += -I$(ZLIB_DIR)
CFLAGS += -I$(SRC_ROOT_GPG)/include

### Enable this for GPGME and GPG ERROR dirs
#CFLAGS += -I$(GPGME_DIR)/src
#CFLAGS += -I$(GPG_ERROR_DIR)/src

CFLAGS += $(WININC)



LIBS =  -L$(LIBDIR) -lretroshare 

LIBS +=  -L$(SSL_DIR) 

LIBS +=  -lssl -lcrypto 
LIBS +=  -L$(UPNPC_DIR) -lminiupnpc
LIBS += -L$(ZLIB_DIR) -lz 
LIBS += -L$(PTHREADS_DIR) -lpthreadGC2d 
LIBS += $(WINLIB) 

#RSCFLAGS = -Wall -g $(INCLUDE) 
#RSCFLAGS += $(WININC)


