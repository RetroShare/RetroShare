
ifneq ($(OS),Linux)
dummy:
	echo "ERROR Linux configuration file included, but (OS != Linux)

endif

############   LINUX CONFIGURATION    ########################

# flags for components....
#PQI_USE_SSLONLY = 1
PQI_USE_XPGP = 1


#PQI_USE_PROXY = 1
#PQI_USE_CHANNELS = 1
#USE_FILELOOK = 1

SSL_DIR=../../../../openssl-0.9.7g-xpgp-0.1c
UPNPC_DIR=../../../../miniupnpc-1.0

# Need to define miniupnpc version because API  changed a little between v1.0 and 1.2
# put 10 for 1.0 and 12 for 1.2
DEFINES += -DMINIUPNPC_VERSION=10

include $(RS_TOP_DIR)/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING ########################

CC = g++
RM = /bin/rm
RANLIB = ranlib
LIBDIR = $(RS_TOP_DIR)/lib
LIBRS = $(LIBDIR)/libretroshare.a

# Unix: Linux/Cygwin
INCLUDE = -I $(RS_TOP_DIR) 
#-I$(KADC_DIR)
CFLAGS = -Wall -g $(INCLUDE) 
CFLAGS += ${DEFINES}

ifdef PQI_USE_XPGP
	INCLUDE += -I $(SSL_DIR)/include 
endif

ifdef PQI_USE_XPGP
	CFLAGS += -DPQI_USE_XPGP
endif

ifdef PQI_USE_SSLONLY
	CFLAGS += -DPQI_USE_SSLONLY
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

# march=i686 causes problems while 64Bit compiling, GCC tries to generate Output for a m64 machine, but the marchi686
# doesnt allow the instructionfs for that.
#
# gcc docu: http://gcc.gnu.org/onlinedocs/gcc-4.0.3/gcc/i386-and-x86_002d64-Options.html#i386-and-x86_002d64-Options

# Linux flags
BIOCFLAGS =  -I $(SSL_DIR)/include ${DEFINES} -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H -DOPENSSL_NO_KRB5 -DL_ENDIAN -DTERMIO -O3 -fomit-frame-pointer -Wall -DSHA1_ASM -DMD5_ASM -DRMD160_ASM  

#########################################################################
# OS specific Linking.
#########################################################################

LIBS =  -L$(LIBDIR) -lretroshare 
ifdef PQI_USE_XPGP
	LIBS +=  -L$(SSL_DIR) 
  endif
LIBS +=  -lssl -lcrypto  -lpthread
LIBS +=  -L$(UPNPC_DIR) -lminiupnpc
LIBS +=  $(XLIB) -ldl -lz 
LIBS +=  -lgpgme
	
RSLIBS = $(LIBS)


