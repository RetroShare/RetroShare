# ==============================================================================
# RetroShare Clean Workspace Orchestrator Makefile
# ------------------------------------------------------------------------------
# Model:
#   * `configure` is the SINGLE source of truth. It writes ALL options into the
#     CMake cache (Build-cmake) exactly once. Pass options here, e.g.:
#         make configure RS_JSON_API=ON RS_GUI=OFF
#   * `show-config` reports the EFFECTIVE configuration, read from that cache.
#   * Build targets (all / libretroshare / retroshare-service /
#     retroshare-friendserver / retroshare-gui) are BUILD-ONLY: they never
#     re-run cmake with divergent -D flags, so they cannot poison the cache.
#   * deploy-windows / deploy-macos package the built tree via the deploy scripts
#     (both expect a fully built Build-cmake; run `make all` first).
# ==============================================================================

# ---- Hardcoded fallback defaults (used only when the cache has no value) ------
DEFAULT_RS_RNPLIB               := ON
DEFAULT_RS_JSON_API             := ON
DEFAULT_RS_WEBUI                := ON
DEFAULT_RS_PLUGINS              := OFF
DEFAULT_RS_GUI                  := ON
DEFAULT_RS_SERVICE              := ON
DEFAULT_RS_FRIENDSERVER         := ON
DEFAULT_RS_FORUM_DEEP_INDEX     := OFF
DEFAULT_RS_DEVELOPMENT_BUILD    := OFF
DEFAULT_RS_USE_I2P_SAM3         := ON
DEFAULT_RS_BITDHT               := ON
DEFAULT_RS_MINIUPNPC            := ON
DEFAULT_RS_BRODCAST_DISCOVERY   := ON
DEFAULT_RS_SQLCIPHER            := ON
DEFAULT_RS_V07_BREAKING_CHANGES := OFF
DEFAULT_CMAKE_BUILD_TYPE        := Release

BUILD_DIR  := Build-cmake
CACHE_FILE := $(BUILD_DIR)/CMakeCache.txt

# Read the current value of a CMake cache entry (empty string if absent).
GET_CACHE_VAL = $(shell [ -f $(CACHE_FILE) ] && grep -E "^$(1):" $(CACHE_FILE) | head -n1 | cut -d= -f2)

# All ON/OFF user-facing options.
RS_OPTIONS := \
	RS_RNPLIB RS_JSON_API RS_WEBUI RS_PLUGINS RS_GUI RS_SERVICE RS_FRIENDSERVER \
	RS_FORUM_DEEP_INDEX RS_DEVELOPMENT_BUILD RS_USE_I2P_SAM3 RS_BITDHT \
	RS_MINIUPNPC RS_BRODCAST_DISCOVERY RS_SQLCIPHER RS_V07_BREAKING_CHANGES

# Resolve each option: command-line value > cache value > hardcoded default.
define RESOLVE_OPTION
  CACHE_$(1) := $$(call GET_CACHE_VAL,$(1))
  $(1) ?= $$(if $$(CACHE_$(1)),$$(CACHE_$(1)),$$(DEFAULT_$(1)))
endef
$(foreach o,$(RS_OPTIONS),$(eval $(call RESOLVE_OPTION,$(o))))

# CMAKE_BUILD_TYPE is handled separately (may be intentionally empty).
CACHE_CMAKE_BUILD_TYPE := $(call GET_CACHE_VAL,CMAKE_BUILD_TYPE)
CMAKE_BUILD_TYPE ?= $(if $(CACHE_CMAKE_BUILD_TYPE),$(CACHE_CMAKE_BUILD_TYPE),$(DEFAULT_CMAKE_BUILD_TYPE))

# ==============================================================================
# Options validation (stops immediately if any option is invalid)
# ==============================================================================
VALID_OPTIONS     := ON OFF
VALID_BUILD_TYPES := Debug Release RelWithDebInfo MinSizeRel

define VALIDATE_OPTION
  ifeq ($$(filter $$($(1)),$$(VALID_OPTIONS)),)
    $$(error Invalid value for $(1): '$$($(1))'. Valid values are: $$(VALID_OPTIONS))
  endif
endef
$(foreach o,$(RS_OPTIONS),$(eval $(call VALIDATE_OPTION,$(o))))

ifneq ($(CMAKE_BUILD_TYPE),)
  ifeq ($(filter $(CMAKE_BUILD_TYPE),$(VALID_BUILD_TYPES)),)
    $(error Invalid CMAKE_BUILD_TYPE: '$(CMAKE_BUILD_TYPE)'. Valid: $(VALID_BUILD_TYPES) (or leave empty))
  endif
endif

# ---- Unrecognized command-line variable protection ---------------------------
ALLOWED_VARS := $(RS_OPTIONS) CMAKE_BUILD_TYPE CMAKE_GEN CMAKE_FLAGS NPROC VERBOSE

$(foreach var,$(.VARIABLES),\
  $(if $(filter command line,$(origin $(var))),\
    $(if $(filter $(var),$(ALLOWED_VARS)),,\
      $(error Unrecognized command line variable: '$(var)'. Allowed variables are: $(ALLOWED_VARS))\
    )\
  )\
)

# ==============================================================================
# Platform detection & global CMake flags
# ==============================================================================
# Mandatory policy flag for old submodules (e.g. libbitdht). Applied via
# `override +=` so it is NEVER dropped, even when the user passes CMAKE_FLAGS=...
override CMAKE_FLAGS += -DCMAKE_POLICY_VERSION_MINIMUM=3.5

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # Homebrew root (Intel & Apple Silicon).
    BREW_ROOT  := $(shell brew --prefix 2>/dev/null)
    QT5_PREFIX := $(shell brew --prefix qt@5 2>/dev/null)
    ifneq ($(QT5_PREFIX),)
        # Prioritize the Qt5 prefix to bypass Homebrew's Qt5 symlink bug.
        override CMAKE_FLAGS += -DCMAKE_PREFIX_PATH="$(QT5_PREFIX);$(BREW_ROOT)"
    else ifneq ($(BREW_ROOT),)
        override CMAKE_FLAGS += -DCMAKE_PREFIX_PATH="$(BREW_ROOT)"
    endif
    NPROC ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    NPROC ?= $(shell nproc 2>/dev/null || echo 4)
endif

# Windows MSYS2 / MinGW: force Makefile generation and a shared libretroshare
# (avoids multiple-definition issues with static linking).
ifeq ($(OS),Windows_NT)
    CMAKE_GEN    ?= -G "MSYS Makefiles"
    RS_WIN_FLAGS := -DRS_LIBRETROSHARE_STATIC=OFF -DRS_LIBRETROSHARE_SHARED=ON
else
    CMAKE_GEN    ?=
    RS_WIN_FLAGS :=
endif

# Only emit -DCMAKE_BUILD_TYPE when non-empty (avoids a meaningless empty flag).
BUILD_TYPE_FLAG := $(if $(CMAKE_BUILD_TYPE),-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE),)

# The full option set written by `configure` (the single source of truth).
RS_CMAKE_OPTS := \
	$(BUILD_TYPE_FLAG) \
	-DRS_RNPLIB=$(RS_RNPLIB) \
	-DRS_JSON_API=$(RS_JSON_API) \
	-DRS_WEBUI=$(RS_WEBUI) \
	-DRS_PLUGINS=$(RS_PLUGINS) \
	-DRS_GUI=$(RS_GUI) \
	-DRS_SERVICE=$(RS_SERVICE) \
	-DRS_FRIENDSERVER=$(RS_FRIENDSERVER) \
	-DRS_FORUM_DEEP_INDEX=$(RS_FORUM_DEEP_INDEX) \
	-DRS_DEVELOPMENT_BUILD=$(RS_DEVELOPMENT_BUILD) \
	-DRS_USE_I2P_SAM3=$(RS_USE_I2P_SAM3) \
	-DRS_BITDHT=$(RS_BITDHT) \
	-DRS_MINIUPNPC=$(RS_MINIUPNPC) \
	-DRS_BRODCAST_DISCOVERY=$(RS_BRODCAST_DISCOVERY) \
	-DRS_SQLCIPHER=$(RS_SQLCIPHER) \
	-DRS_V07_BREAKING_CHANGES=$(RS_V07_BREAKING_CHANGES) \
	$(RS_WIN_FLAGS)

# ==============================================================================
# Targets
# ==============================================================================
.DEFAULT_GOAL := all
# Build-only targets parallelise internally via `cmake --build -j`; forbid
# top-level parallelism so two recipes never reconfigure/build the same tree at
# once.
.NOTPARALLEL:
.PHONY: all configure show-config help clean \
        libretroshare retroshare-service retroshare-friendserver retroshare-gui \
        deploy-windows deploy-macos

# configure: single source of truth — writes every option into the CMake cache.
configure:
	@echo ">>> Configuring RetroShare into $(BUILD_DIR) (options -> CMake cache)..."
	cmake $(CMAKE_GEN) -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) $(RS_CMAKE_OPTS)
	@echo ">>> Configured. Run 'make all' to build, 'make show-config' to review."

# Auto-configure once (with current options) if the cache does not exist yet.
$(CACHE_FILE):
	@$(MAKE) --no-print-directory configure

# all: (re)configure with current options, then build everything.
all: configure
	@echo ">>> Building all configured RetroShare modules..."
	cmake --build $(BUILD_DIR) -j $(NPROC)

# Build-only component targets: configure once if needed (order-only prereq),
# then just build — they never pass -D flags, so the cache is never poisoned.
libretroshare: | $(CACHE_FILE)
	@echo ">>> Building libretroshare..."
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare

retroshare-service: | $(CACHE_FILE)
	@echo ">>> Building retroshare-service..."
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-service

retroshare-friendserver: | $(CACHE_FILE)
	@echo ">>> Building retroshare-friendserver..."
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-friendserver

retroshare-gui: | $(CACHE_FILE)
	@echo ">>> Building retroshare-gui..."
	cmake --build $(BUILD_DIR) -j $(NPROC) --target retroshare-gui
	@if [ "$(RS_PLUGINS)" = "ON" ]; then \
		echo ">>> Building GUI plugins (FeedReader, VOIP)..."; \
		cmake --build $(BUILD_DIR) -j $(NPROC) --target FeedReader --target VOIP; \
	fi

# ---- Deployment (package the already-built Build-cmake tree) ------------------
deploy-windows:
	@[ -f ./deploy-windows.sh ] || { echo "ERROR: deploy-windows.sh not found"; exit 1; }
	@echo ">>> Deploying (Windows)..."
	./deploy-windows.sh

deploy-macos:
	@[ -f ./deploy-macos.sh ] || { echo "ERROR: deploy-macos.sh not found"; exit 1; }
	@echo ">>> Deploying (macOS)..."
	./deploy-macos.sh

clean:
	@echo ">>> Removing build directory ($(BUILD_DIR))..."
	rm -rf $(BUILD_DIR)

# ==============================================================================
# show-config: report the EFFECTIVE configuration.
#   1. command-line override  -> "(command line override)"
#   2. value present in cache -> "(configured)" or "(configured = default)"
#   3. otherwise              -> "(default, not configured yet)"
# ==============================================================================
define GET_VAR_STATUS
$(shell \
	if [ "$(origin $(1))" = "command line" ]; then \
		echo "$($(1)) (command line override)"; \
	elif [ -f $(CACHE_FILE) ] && grep -q -E "^$(1):" $(CACHE_FILE); then \
		val=$$(grep -E "^$(1):" $(CACHE_FILE) | head -n1 | cut -d= -f2); \
		if [ "$$val" = "$(DEFAULT_$(1))" ]; then \
			echo "$$val (configured = default)"; \
		else \
			echo "$$val (configured)"; \
		fi; \
	else \
		echo "$($(1)) (default, not configured yet)"; \
	fi \
)
endef

show-config:
	@echo "=============================================================================="
	@echo "          CURRENT BUILD CONFIGURATION  (source: $(CACHE_FILE))"
	@echo "=============================================================================="
	@echo "  CMAKE_BUILD_TYPE        = $(call GET_VAR_STATUS,CMAKE_BUILD_TYPE)"
	@echo "  RS_GUI                  = $(call GET_VAR_STATUS,RS_GUI)"
	@echo "  RS_SERVICE              = $(call GET_VAR_STATUS,RS_SERVICE)"
	@echo "  RS_FRIENDSERVER         = $(call GET_VAR_STATUS,RS_FRIENDSERVER)"
	@echo "  RS_RNPLIB               = $(call GET_VAR_STATUS,RS_RNPLIB)"
	@echo "  RS_JSON_API             = $(call GET_VAR_STATUS,RS_JSON_API)"
	@echo "  RS_WEBUI                = $(call GET_VAR_STATUS,RS_WEBUI)"
	@echo "  RS_PLUGINS              = $(call GET_VAR_STATUS,RS_PLUGINS)"
	@echo "  RS_FORUM_DEEP_INDEX     = $(call GET_VAR_STATUS,RS_FORUM_DEEP_INDEX)"
	@echo "  RS_DEVELOPMENT_BUILD    = $(call GET_VAR_STATUS,RS_DEVELOPMENT_BUILD)"
	@echo "  RS_USE_I2P_SAM3         = $(call GET_VAR_STATUS,RS_USE_I2P_SAM3)"
	@echo "  RS_BITDHT               = $(call GET_VAR_STATUS,RS_BITDHT)"
	@echo "  RS_MINIUPNPC            = $(call GET_VAR_STATUS,RS_MINIUPNPC)"
	@echo "  RS_BRODCAST_DISCOVERY   = $(call GET_VAR_STATUS,RS_BRODCAST_DISCOVERY)"
	@echo "  RS_SQLCIPHER            = $(call GET_VAR_STATUS,RS_SQLCIPHER)"
	@echo "  RS_V07_BREAKING_CHANGES = $(call GET_VAR_STATUS,RS_V07_BREAKING_CHANGES)"
	@echo "  NPROC                   = $(NPROC)"
	@echo "  BUILD_DIR               = $(BUILD_DIR)"
	@echo '  CMAKE_GEN               = $(CMAKE_GEN)'
	@echo '  CMAKE_FLAGS             = $(CMAKE_FLAGS)'
	@echo "=============================================================================="

help:
	@echo "=============================================================================="
	@echo "          RETROSHARE ORCHESTRATOR HELP & OPTIONS"
	@echo "=============================================================================="
	@echo "Usage:"
	@echo "  make [target] [OPTION=value]"
	@echo ""
	@echo "Typical flow:"
	@echo "  make configure RS_JSON_API=ON       # set options once (-> CMake cache)"
	@echo "  make show-config                    # review the effective configuration"
	@echo "  make all                            # build everything into $(BUILD_DIR)"
	@echo "  make deploy-windows | deploy-macos  # package the built tree"
	@echo ""
	@echo "Targets:"
	@echo "  all                      (default) Configure if needed, then build all modules"
	@echo "  configure                Run CMake configuration only (writes options to cache)"
	@echo "  libretroshare            Build the libretroshare core (build-only)"
	@echo "  retroshare-service       Build retroshare-service, headless daemon (build-only)"
	@echo "  retroshare-friendserver  Build retroshare-friendserver (build-only)"
	@echo "  retroshare-gui           Build retroshare-gui, +plugins if RS_PLUGINS=ON (build-only)"
	@echo "  deploy-windows           Package a Windows build via deploy-windows.sh"
	@echo "  deploy-macos             Package a macOS build via deploy-macos.sh"
	@echo "  show-config              Display the effective build configuration"
	@echo "  clean                    Remove the build directory ($(BUILD_DIR))"
	@echo "  help                     Show this help message"
	@echo ""
	@echo "Note: build-only targets reuse the existing CMake configuration; run"
	@echo "      'make configure' (or 'make all') first to choose what gets built."
	@echo ""
	@echo "Options (OPTION=ON/OFF unless noted; persisted in the CMake cache):"
	@echo "  RS_GUI                Build the Qt GUI (default: ON)"
	@echo "  RS_SERVICE            Build retroshare-service (default: ON)"
	@echo "  RS_FRIENDSERVER       Build retroshare-friendserver (default: ON)"
	@echo "  RS_RNPLIB             Use RNP library for PGP (default: ON)"
	@echo "  RS_JSON_API           Enable HTTP JSON API service (default: ON)"
	@echo "  RS_WEBUI              Deploy RetroShare Web UI assets (default: ON; requires RS_JSON_API=ON)"
	@echo "  RS_PLUGINS            Build RetroShare GUI plugins (default: OFF)"
	@echo "  RS_FORUM_DEEP_INDEX   Xapian-based full-text index/search of GXS forums (default: OFF)"
	@echo "  RS_DEVELOPMENT_BUILD  Disable optimisations for fast builds (default: OFF)"
	@echo "  RS_USE_I2P_SAM3       Enable I2P SAMv3 support (default: ON)"
	@echo "  RS_BITDHT             BitTorrent-DHT peer discovery (default: ON)"
	@echo "  RS_MINIUPNPC          NAT/UPnP port forwarding via miniupnpc (default: ON)"
	@echo "  RS_BRODCAST_DISCOVERY LAN peer discovery (default: ON)"
	@echo "  RS_SQLCIPHER          SQLCipher encryption for the GXS database (default: ON)"
	@echo "  RS_V07_BREAKING_CHANGES  Enable 0.7.0 retro-incompatible changes (default: OFF)"
	@echo "  CMAKE_BUILD_TYPE      Debug/Release/RelWithDebInfo/MinSizeRel (default: Release)"
	@echo "  NPROC                 Number of build threads (default: detected CPUs)"
	@echo "  CMAKE_FLAGS           Extra flags appended to the cmake configure line"
	@echo "=============================================================================="
