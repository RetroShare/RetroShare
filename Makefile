# ==============================================================================
# RetroShare Clean Workspace Orchestrator Makefile
# ==============================================================================

# Default build options
RS_RNPLIB            ?= ON
RS_JSON_API          ?= ON
RS_WEBUI             ?= ON
RS_DEVELOPMENT_BUILD ?= OFF
CMAKE_BUILD_TYPE     ?= 

# Dynamically detect number of CPUs (defaults to 4 if not found)
NPROC ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all clean rnp libretroshare retroshare-service retroshare-friendserver retroshare-gui

# Note: retroshare-gui is intentionally excluded from the default 'all' target
# as its UI CMakeLists.txt files are broken originally.
# This allows compiling everything else (servers/services) with guaranteed success.
all: rnp libretroshare retroshare-service retroshare-friendserver
	@echo "================================================================================"
	@echo " SUCCESS: All functional RetroShare modules have been compiled successfully! "
	@echo "================================================================================"

rnp:
	@echo ">>> Step 1: Compiling RNP..."
	mkdir -p supportlibs/librnp/Build
	cd supportlibs/librnp/Build && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCRYPTO_BACKEND=botan -DBUILD_TESTING=off .. && make -j$(NPROC)

libretroshare: rnp
	@echo ">>> Step 2: Compiling libretroshare..."
	rm -rf libretroshare/supportlibs
	ln -s ../supportlibs libretroshare/supportlibs
	mkdir -p libretroshare/Build
	cd libretroshare/Build && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DRS_JSON_API=$(RS_JSON_API) -DRS_WEBUI=$(RS_WEBUI) .. && make -j$(NPROC)

retroshare-service: rnp
	@echo ">>> Step 3: Compiling retroshare-service..."
	rm -rf retroshare-service/Build
	mkdir -p retroshare-service/Build
	ln -s ../../supportlibs retroshare-service/Build/supportlibs
	cd retroshare-service/Build && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DRS_RNPLIB=$(RS_RNPLIB) -DRS_WEBUI=$(RS_WEBUI) .. && make -j$(NPROC)

retroshare-friendserver: rnp
	@echo ">>> Step 4: Compiling retroshare-friendserver..."
	rm -rf retroshare-friendserver/Build
	mkdir -p retroshare-friendserver/Build
	ln -s ../../supportlibs retroshare-friendserver/Build/supportlibs
	cd retroshare-friendserver/Build && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DRS_RNPLIB=$(RS_RNPLIB) .. && make -j$(NPROC)

# Target for compiling the GUI application
retroshare-gui: rnp
	@echo ">>> Step 5: Compiling retroshare-gui (UI)..."
	mkdir -p retroshare-gui/Build
	if [ ! -L retroshare-gui/Build/supportlibs ]; then ln -s ../../supportlibs retroshare-gui/Build/supportlibs; fi
	cd retroshare-gui/Build && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DRS_DEVELOPMENT_BUILD=$(RS_DEVELOPMENT_BUILD) -DRS_JSON_API=$(RS_JSON_API) -DRS_WEBUI=$(RS_WEBUI) .. && make -j$(NPROC)

clean:
	@echo "Cleaning up all build directories..."
	rm -rf supportlibs/librnp/Build
	rm -rf libretroshare/Build libretroshare/supportlibs
	rm -rf retroshare-service/Build
	rm -rf retroshare-friendserver/Build
	rm -rf retroshare-gui/Build
