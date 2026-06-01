# ==============================================================================
# RetroShare Clean Workspace Orchestrator Makefile
# ==============================================================================

# Default build options (uses cached values from Build-cmake/CMakeCache.txt if present)
CACHE_FILE := Build-cmake/CMakeCache.txt
GET_CACHE_VAL = $(shell [ -f $(CACHE_FILE) ] && grep -E "^$(1):" $(CACHE_FILE) | head -n1 | cut -d= -f2)

CACHE_RS_RNPLIB            := $(call GET_CACHE_VAL,RS_RNPLIB)
CACHE_RS_JSON_API          := $(call GET_CACHE_VAL,RS_JSON_API)
CACHE_RS_WEBUI             := $(call GET_CACHE_VAL,RS_WEBUI)
CACHE_RS_PLUGINS           := $(call GET_CACHE_VAL,RS_PLUGINS)
CACHE_RS_FORUM_DEEP_INDEX  := $(call GET_CACHE_VAL,RS_FORUM_DEEP_INDEX)
CACHE_RS_DEVELOPMENT_BUILD := $(call GET_CACHE_VAL,RS_DEVELOPMENT_BUILD)
CACHE_CMAKE_BUILD_TYPE     := $(call GET_CACHE_VAL,CMAKE_BUILD_TYPE)
CACHE_RS_USE_I2P_SAM3      := $(call GET_CACHE_VAL,RS_USE_I2P_SAM3)

RS_RNPLIB            ?= $(if $(CACHE_RS_RNPLIB),$(CACHE_RS_RNPLIB),ON)
RS_JSON_API          ?= $(if $(CACHE_RS_JSON_API),$(CACHE_RS_JSON_API),ON)
RS_WEBUI             ?= $(if $(CACHE_RS_WEBUI),$(CACHE_RS_WEBUI),ON)
RS_PLUGINS           ?= $(if $(CACHE_RS_PLUGINS),$(CACHE_RS_PLUGINS),OFF)
RS_FORUM_DEEP_INDEX  ?= $(if $(CACHE_RS_FORUM_DEEP_INDEX),$(CACHE_RS_FORUM_DEEP_INDEX),OFF)
RS_DEVELOPMENT_BUILD ?= $(if $(CACHE_RS_DEVELOPMENT_BUILD),$(CACHE_RS_DEVELOPMENT_BUILD),OFF)
CMAKE_BUILD_TYPE     ?= $(if $(CACHE_CMAKE_BUILD_TYPE),$(CACHE_CMAKE_BUILD_TYPE),RelWithDebInfo)
RS_USE_I2P_SAM3      ?= $(if $(CACHE_RS_USE_I2P_SAM3),$(CACHE_RS_USE_I2P_SAM3),ON)

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
$(eval $(call VALIDATE_OPTION,RS_USE_I2P_SAM3))

# Specific validation for CMAKE_BUILD_TYPE (if not empty)
ifneq ($(CMAKE_BUILD_TYPE),)
  ifeq ($(filter $(CMAKE_BUILD_TYPE),$(VALID_BUILD_TYPES)),)
    $(error Invalid option value for CMAKE_BUILD_TYPE: '$(CMAKE_BUILD_TYPE)'. Valid values are: $(VALID_BUILD_TYPES) (or leave empty))
  endif
endif

# Unrecognized command line variable protection
ALLOWED_VARS := RS_RNPLIB RS_JSON_API RS_WEBUI RS_PLUGINS RS_FORUM_DEEP_INDEX RS_DEVELOPMENT_BUILD CMAKE_BUILD_TYPE RS_USE_I2P_SAM3 CMAKE_GEN CMAKE_FLAGS NPROC

$(foreach var,$(.VARIABLES),\
  $(if $(filter command line,$(origin $(var))),\
    $(if $(filter $(var),$(ALLOWED_VARS)),,\
      $(error Unrecognized command line variable: '$(var)'. Allowed variables are: $(ALLOWED_VARS))\
    )\
  )\
)

# Global CMake flags (forces policy compatibility for old submodules like libbitdht)
CMAKE_FLAGS ?= -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Detect macOS to automatically inject Homebrew and Qt5 paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # Dynamically find Homebrew root path (works on both Intel and Apple Silicon)
    BREW_ROOT := $(shell brew --prefix 2>/dev/null)
    ifneq ($(BREW_ROOT),)
        # Prioritize Qt5 prefix path over Homebrew root to bypass Homebrew's Qt5 symlink bug
        CMAKE_FLAGS += -DCMAKE_PREFIX_PATH="$(shell brew --prefix qt@5 2>/dev/null);$(BREW_ROOT)"
    endif
    # Dynamically detect number of CPUs on macOS
    NPROC ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    # Dynamically detect number of CPUs on Linux (defaults to 4 if not found)
    NPROC ?= $(shell nproc 2>/dev/null || echo 4)
endif

# Detect Windows MSYS2 / MinGW environment to force Makefile generation
ifeq ($(OS),Windows_NT)
    CMAKE_GEN ?= -G "MSYS Makefiles"
else
    CMAKE_GEN ?=
endif

# Global build directory at the root
BUILD_DIR = Build-cmake

# Common CMake options shared by all targets (DRY factorization)
RS_CMAKE_COMMON = \
	-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	-DRS_RNPLIB=$(RS_RNPLIB) \
	-DRS_PLUGINS=$(RS_PLUGINS) \
	-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
	-DRS_DEVELOPMENT_BUILD=$(RS_DEVELOPMENT_BUILD) \
	-DRS_USE_I2P_SAM3=$(RS_USE_I2P_SAM3)

.PHONY: all clean rnp libretroshare retroshare-service retroshare-friendserver retroshare-gui configure show-config help

configure: rnp
	@echo ">>> Configuring RetroShare (CMake only, no compilation)..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=ON \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=ON
	@echo ">>> Configuration complete. Run 'make all' to compile."

all: rnp
	@echo ">>> Compiling all functional RetroShare modules..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
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
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare

retroshare-service: rnp
	@echo ">>> Step 3: Compiling retroshare-service..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=OFF \
		-DRS_SERVICE=ON \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-service

retroshare-friendserver: rnp
	@echo ">>> Step 4: Compiling retroshare-friendserver..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=OFF \
		-DRS_WEBUI=OFF \
		-DRS_GUI=OFF \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=ON
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-friendserver

retroshare-gui: rnp
	@echo ">>> Step 5: Compiling retroshare-gui (UI)..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_COMMON) \
		-DRS_JSON_API=$(RS_JSON_API) \
		-DRS_WEBUI=$(RS_WEBUI) \
		-DRS_GUI=ON \
		-DRS_SERVICE=OFF \
		-DRS_FRIENDSERVER=OFF
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-gui
	@if [ "$(RS_PLUGINS)" = "ON" ]; then \
		echo ">>> Step 5b: Compiling GUI plugins (FeedReader, VOIP)..."; \
		cmake --build $(BUILD_DIR) -j $(NPROC) --target FeedReader --target VOIP; \
	fi

clean:
	@echo "Cleaning up all build directories..."
	rm -rf supportlibs/librnp/Build
	rm -rf $(BUILD_DIR)

# Helper macro to dynamically detect the status of configuration options:
# 1. First checks if overridden via command line
# 2. Then checks if configured inside CMakeCache.txt
# 3. Otherwise falls back to the Makefile default
define GET_VAR_STATUS
$(shell \
	if [ "$(origin $(1))" = "command line" ]; then \
		echo "$($(1)) (command line override)"; \
	elif [ -f $(BUILD_DIR)/CMakeCache.txt ] && grep -q -E "^$(1):" $(BUILD_DIR)/CMakeCache.txt; then \
		val=$$(grep -E "^$(1):" $(BUILD_DIR)/CMakeCache.txt | head -n1 | cut -d= -f2); \
		if [ "$$val" = "$($(1))" ]; then \
			echo "$$val (default)"; \
		else \
			echo "$$val (configured)"; \
		fi; \
	else \
		echo "$($(1)) (default)"; \
	fi \
)
endef

show-config:
	@echo "=============================================================================="
	@echo "          CURRENT BUILD CONFIGURATION"
	@echo "=============================================================================="
	@echo "  CMAKE_BUILD_TYPE      = $(call GET_VAR_STATUS,CMAKE_BUILD_TYPE)"
	@echo "  RS_RNPLIB             = $(call GET_VAR_STATUS,RS_RNPLIB)"
	@echo "  RS_JSON_API           = $(call GET_VAR_STATUS,RS_JSON_API)"
	@echo "  RS_WEBUI              = $(call GET_VAR_STATUS,RS_WEBUI)"
	@echo "  RS_PLUGINS            = $(call GET_VAR_STATUS,RS_PLUGINS)"
	@echo "  RS_FORUM_DEEP_INDEX   = $(call GET_VAR_STATUS,RS_FORUM_DEEP_INDEX)"
	@echo "  RS_DEVELOPMENT_BUILD  = $(call GET_VAR_STATUS,RS_DEVELOPMENT_BUILD)"
	@echo "  RS_USE_I2P_SAM3       = $(call GET_VAR_STATUS,RS_USE_I2P_SAM3)"
	@echo "  NPROC                 = $(NPROC)"
	@echo "  BUILD_DIR             = $(BUILD_DIR)"
	@echo '  CMAKE_GEN             = $(CMAKE_GEN)'
	@echo '  CMAKE_FLAGS           = $(CMAKE_FLAGS)'
	@echo "=============================================================================="

help:
	@echo "=============================================================================="
	@echo "          RETROSHARE ORCHESTRATOR HELP & OPTIONS"
	@echo "=============================================================================="
	@echo "Usage:"
	@echo "  make <target> [OPTION=value]"
	@echo ""
	@echo "Available targets:"
	@echo "  all                      Compile all RetroShare modules (default)"
	@echo "  rnp                      Compile RNP cryptography library"
	@echo "  libretroshare            Compile libretroshare core library"
	@echo "  retroshare-service       Compile retroshare-service (headless daemon)"
	@echo "  retroshare-friendserver  Compile retroshare-friendserver"
	@echo "  retroshare-gui           Compile retroshare-gui (graphical user interface)"
	@echo "  configure                Compile RNP and run CMake configuration"
	@echo "  show-config              Display current build configuration"
	@echo "  clean                    Clean all build directories"
	@echo "  help                     Show this help message"
	@echo ""
	@echo "Available build options (use OPTION=ON/OFF or value):"
	@echo "  RS_RNPLIB             Use RNP library for PGP (ON/OFF, default: ON)"
	@echo "  RS_JSON_API           Enable HTTP JSON API service (ON/OFF, default: ON)"
	@echo "  RS_WEBUI              Deploy RetroShare Web UI assets (ON/OFF, default: ON)"
	@echo "  RS_PLUGINS            Build RetroShare plugins (ON/OFF, default: OFF)"
	@echo "  RS_FORUM_DEEP_INDEX   Xapian/FTS5 forum full text search (ON/OFF, default: OFF)"
	@echo "  RS_DEVELOPMENT_BUILD  Disable optimisations for fast builds (ON/OFF, default: OFF)"
	@echo "  RS_USE_I2P_SAM3       Enable I2P SAMv3 support (ON/OFF, default: ON)"
	@echo "  CMAKE_BUILD_TYPE      CMake build type (Debug/Release/RelWithDebInfo/MinSizeRel)"
	@echo "  NPROC                 Number of build threads (default: detected CPUs)"
	@echo "=============================================================================="
