
ifneq ($(OS),MacOSX)
dummy:
	echo "ERROR MacOSX configuration file included, but (OS != MacOSX)

endif

############  MACOSX CONFIGURATION    ########################


# FLAGS to decide if we want i386 Build or ppc Build
# 
# 

# MAC_I386_BUILD = 1
# MAC_PPC_BUILD = 1

MAC_I386_BUILD = 1
#MAC_PPC_BUILD = 1

ifndef MAC_I386_BUILD
	MAC_PPC_BUILD = 1
endif

include $(TEST_TOP_DIR)/scripts/checks.mk

############ ENFORCE DIRECTORY NAMING ########################

CC = g++
RM = /bin/rm

RANLIB = ranlib

# Dummy ranlib -> can't do it until afterwards with universal binaries.
# RANLIB = ls -l 

LIBDIR = $(LIB_TOP_DIR)/lib
LIBRS = $(LIBDIR)/libretroshare.a

OPT_DIR = /opt/local
OPT_INCLUDE = $(OPT_DIR)/include
OPT_LIBS = $(OPT_DIR)/lib

INCLUDE = -I $(LIB_TOP_DIR)  
#-I $(OPT_INCLUDE)
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

#########################################################################
# OS Compile Options
#########################################################################

#########################################################################
# OS specific Linking.
#########################################################################

#LIBS = -Wl,-search_paths_first

# for Univeral BUILD
# LIBS += -arch ppc -arch i386
# LIBS += -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386

#LIBS += -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk 
LIBS +=  -L$(LIBDIR) -lbitdht


