# ==============================================================================
# RetroShare Clean Workspace Orchestrator Makefile
# ==============================================================================

# Default build options
RS_RNPLIB            ?= ON
RS_JSON_API          ?= ON
RS_WEBUI             ?= ON
RS_DEVELOPMENT_BUILD ?= OFF
CMAKE_BUILD_TYPE     ?= 

# Detect Windows MSYS2 / MinGW environment to force Makefile generation
ifeq ($(OS),Windows_NT)
    CMAKE_GEN ?= -G "MSYS Makefiles"
else
    CMAKE_GEN ?=
endif

# Global CMake flags (forces policy compatibility for old submodules like libbitdht)
CMAKE_FLAGS ?= -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Dynamically detect number of CPUs (defaults to 4 if not found)
NPROC ?= $(shell nproc 2>/dev/null || echo 4)

# Global build directory at the root
BUILD_DIR = Build

.PHONY: all clean rnp libretroshare retroshare-service retroshare-friendserver retroshare-gui

all: rnp
	@echo ">>> Compiling all functional RetroShare modules..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=ON \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=ON \
		.. && make -j$(NPROC)

rnp:
	@echo ">>> Step 1: Compiling RNP..."
	mkdir -p supportlibs/librnp/Build
	cd supportlibs/librnp/Build && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCRYPTO_BACKEND=botan -DBUILD_TESTING=off .. && make -j$(NPROC)

libretroshare: rnp
	@echo ">>> Step 2: Compiling libretroshare..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF \
		.. && make -j$(NPROC) retroshare

retroshare-service: rnp
	@echo ">>> Step 3: Compiling retroshare-service..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=OFF \
		.. && make -j$(NPROC) retroshare-service

retroshare-friendserver: rnp
	@echo ">>> Step 4: Compiling retroshare-friendserver..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=ON \
		.. && make -j$(NPROC) retroshare-friendserver

retroshare-gui: rnp
	@echo ">>> Step 5: Compiling retroshare-gui (UI)..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_GEN) $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=ON \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF \
		.. && make -j$(NPROC) retroshare-gui

clean:
	@echo "Cleaning up all build directories..."
	rm -rf supportlibs/librnp/Build
	rm -rf $(BUILD_DIR)
