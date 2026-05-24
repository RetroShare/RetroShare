# ==============================================================================
# RetroShare Clean Workspace Orchestrator Makefile
# ==============================================================================

# Default build options
RS_RNPLIB            ?= ON
RS_JSON_API          ?= ON
RS_WEBUI             ?= ON
RS_PLUGINS           ?= OFF
RS_FORUM_DEEP_INDEX  ?= OFF
RS_DEVELOPMENT_BUILD ?= OFF
CMAKE_BUILD_TYPE     ?=

# ==============================================================================
# Options validation (stops immediately if any option is invalid)
# ==============================================================================
VALID_OPTIONS := ON OFF
VALID_BUILD_TYPES := Debug Release RelWithDebInfo MinSizeRel

define VALIDATE_OPTION
  ifeq ($$(filter $$($(1)),$$(VALID_OPTIONS)),)
    $$(error Invalid option value for $(1): '$$($(1))'. Valid values are: $$(VALID_OPTIONS))
  endif
endef

# Validate ON/OFF options
$(eval $(call VALIDATE_OPTION,RS_RNPLIB))
$(eval $(call VALIDATE_OPTION,RS_JSON_API))
$(eval $(call VALIDATE_OPTION,RS_WEBUI))
$(eval $(call VALIDATE_OPTION,RS_PLUGINS))
$(eval $(call VALIDATE_OPTION,RS_FORUM_DEEP_INDEX))
$(eval $(call VALIDATE_OPTION,RS_DEVELOPMENT_BUILD))

# Specific validation for CMAKE_BUILD_TYPE (if not empty)
ifneq ($(CMAKE_BUILD_TYPE),)
  ifeq ($(filter $(CMAKE_BUILD_TYPE),$(VALID_BUILD_TYPES)),)
    $(error Invalid option value for CMAKE_BUILD_TYPE: '$(CMAKE_BUILD_TYPE)'. Valid values are: $(VALID_BUILD_TYPES) (or leave empty))
  endif
endif

# Unrecognized command line variable protection
ALLOWED_VARS := RS_RNPLIB RS_JSON_API RS_WEBUI RS_PLUGINS RS_FORUM_DEEP_INDEX RS_DEVELOPMENT_BUILD CMAKE_BUILD_TYPE CMAKE_GEN CMAKE_FLAGS NPROC

$(foreach var,$(.VARIABLES),\
  $(if $(filter command line,$(origin $(var))),\
    $(if $(filter $(var),$(ALLOWED_VARS)),,\
      $(error Unrecognized command line variable: '$(var)'. Allowed variables are: $(ALLOWED_VARS))\
    )\
  )\
)

# Global CMake flags (forces policy compatibility for old submodules like libbitdht)
CMAKE_FLAGS ?= -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Detect Windows MSYS2 / MinGW environment to force Makefile generation
ifeq ($(OS),Windows_NT)
    CMAKE_GEN ?= -G "MSYS Makefiles"
else
    CMAKE_GEN ?=
endif

# Dynamically detect number of CPUs (defaults to 4 if not found)
NPROC ?= $(shell nproc 2>/dev/null || echo 4)

# Global build directory at the root
BUILD_DIR = Build-cmake

.PHONY: all clean rnp libretroshare retroshare-service retroshare-friendserver retroshare-gui

all: rnp
	@echo ">>> Compiling all functional RetroShare modules..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_PLUGINS=$(RS_PLUGINS) \
		-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
		-DRS_GUI=ON \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=ON
	cmake --build $(BUILD_DIR) -j $(NPROC)

rnp:
	@echo ">>> Step 1: Compiling RNP..."
	cmake $(CMAKE_GEN) -B supportlibs/librnp/Build -S supportlibs/librnp $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCRYPTO_BACKEND=botan -DBUILD_TESTING=off
	cmake --build supportlibs/librnp/Build -j $(NPROC)

libretroshare: rnp
	@echo ">>> Step 2: Compiling libretroshare..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_PLUGINS=$(RS_PLUGINS) \
		-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare

retroshare-service: rnp
	@echo ">>> Step 3: Compiling retroshare-service..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_PLUGINS=$(RS_PLUGINS) \
		-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-service

retroshare-friendserver: rnp
	@echo ">>> Step 4: Compiling retroshare-friendserver..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_PLUGINS=$(RS_PLUGINS) \
		-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=ON
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-friendserver

retroshare-gui: rnp
	@echo ">>> Step 5: Compiling retroshare-gui (UI)..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DRS_RNPLIB=$(RS_RNPLIB) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_PLUGINS=$(RS_PLUGINS) \
		-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
		-DRS_GUI=ON \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-gui

clean:
	@echo "Cleaning up all build directories..."
	rm -rf supportlibs/librnp/Build
	rm -rf $(BUILD_DIR)
