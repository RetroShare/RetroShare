#!/bin/bash
# ==============================================================================
# RetroShare Windows Portability & Deployment Packaging Script
# Designed for MSYS2 MinGW64 Shell environment
# ------------------------------------------------------------------------------
# Why this is more than a one-liner: windeployqt IS the standard tool and does the
# heavy lifting (bundling Qt DLLs/plugins). The extra steps around it handle MSYS2
# /MinGW + RetroShare specifics that windeployqt does not reliably cover:
#   - Manual fallback copy of the Qt 'platforms'/'styles'/'imageformats' plugins,
#     which windeployqt sometimes misses under MSYS2.
#   - Recursive ldd-based resolution of MinGW runtime DLLs (compiler runtime,
#     3rd-party libs) into the portable folder.
#   - RetroShare's portable layout (Data/extensions6 for plugins, qss/stylesheets/
#     sounds/translations) and the 'portable' marker file.
# ==============================================================================
set -e

# Always operate from the repository root (this script lives under
# build_scripts/Windows-msys2/); otherwise the relative paths (retroshare-gui/src,
# libbitdht, ...) resolve to nothing and silently produce an incomplete package.
cd "$(dirname "$0")/../.." || exit 1

# Build directory (override with: BUILD_DIR=mydir ./build_scripts/Windows-msys2/deploy-windows.sh)
BUILD_DIR="${BUILD_DIR:-Build-cmake}"

# Prefix of the MSYS2 environment used for the build
# (mingw64 / ucrt64 / clang64). Required to locate the compiler runtime
# and the Qt plugins whatever environment is used.
MINGW_PREFIX="${MSYSTEM_PREFIX:-/mingw64}"

# 1. Initial checks on the build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory '$BUILD_DIR' not found. Please compile the project first!"
    exit 1
fi

echo "================================================================================"
echo "          RETROSHARE WINDOWS DEPLOYMENT GENERATOR"
echo "================================================================================"

# 2. Dynamically determine the version variables (inspired by qmake)
echo ">>> Extracting versioning info from Git and environment..."

# Date in YYYYMMDD format
DATE_STR=$(date +%Y%m%d)

# Get the Git describe (e.g. v0.6.7.2-892-g5341b777d-dirty)
GIT_DESC=$(git describe --tags --always --dirty 2>/dev/null || echo "0.6.7-unknown")
CLEAN_DESC=${GIT_DESC#v} # Strip the leading 'v'

# Default values
VERSION="$CLEAN_DESC"
GIT_SUFFIX=""

if [[ "$CLEAN_DESC" == *-* ]]; then
    # Format: tag-commits-hash[-dirty] (e.g. 0.6.7.2-892-g5341b777d-dirty)
    VERSION=$(echo "$CLEAN_DESC" | cut -d'-' -f1)
    COMMIT_INFO=$(echo "$CLEAN_DESC" | cut -d'-' -f2-)
    GIT_SUFFIX="-${DATE_STR}-${COMMIT_INFO}"
else
    # Exactly on a tag
    GIT_SUFFIX="-${DATE_STR}"
fi

# ------------------------------------------------------------------------------
# Detect the MAJOR Qt version actually used by the build. We extract it from the
# compiled binary's imports (objdump), NOT from whichever qmake happens to be
# first in PATH: on an MSYS2 install where Qt5 AND Qt6 coexist, `qmake` may point
# to Qt6 while the binary is linked against Qt5 (or vice versa). If deployment is
# not locked to the right version, windeployqt and the fallback plugins
# (platforms/qwindows.dll...) are taken from the wrong Qt, causing the
# "no Qt platform plugin could be initialized" crash at startup.
# ------------------------------------------------------------------------------
QT_MAJOR=""
GUI_EXE_SRC=$(find "$BUILD_DIR" -path '*Portable*' -prune -o -name "retroshare-gui.exe" -print 2>/dev/null | head -n 1)
if [ -n "$GUI_EXE_SRC" ] && command -v objdump &> /dev/null; then
    if objdump -p "$GUI_EXE_SRC" 2>/dev/null | grep -qiE 'Qt6(Core|Gui|Widgets)\.dll'; then
        QT_MAJOR=6
    elif objdump -p "$GUI_EXE_SRC" 2>/dev/null | grep -qiE 'Qt5(Core|Gui|Widgets)\.dll'; then
        QT_MAJOR=5
    fi
fi
if [ -z "$QT_MAJOR" ]; then
    echo "  WARNING: Could not detect the Qt major version from the binary; defaulting to 6."
    QT_MAJOR=6
fi
echo "  Qt major version linked by the build: Qt${QT_MAJOR}"

# Find the windeployqt matching THIS major version. We validate the version
# returned by --version: the bare `windeployqt` may belong to the other Qt on a
# mixed install.
if [ "$QT_MAJOR" = "6" ]; then
    WINDEPLOYQT_CANDIDATES="windeployqt6 windeployqt-qt6 windeployqt"
else
    WINDEPLOYQT_CANDIDATES="windeployqt-qt5 windeployqt"
fi
WINDEPLOYQT_CMD=""
for cmd in $WINDEPLOYQT_CANDIDATES; do
    if command -v "$cmd" &> /dev/null; then
        cand_ver=$("$cmd" --version 2>/dev/null | grep -o -E '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1)
        if [[ "$cand_ver" == "${QT_MAJOR}."* ]]; then
            WINDEPLOYQT_CMD="$cmd"
            break
        fi
    fi
done

# Detect the full Qt version via the qmake of the RIGHT major version.
QT_VERSION=""
if [ "$QT_MAJOR" = "6" ]; then
    QMAKE_CANDIDATES="qmake6 qmake-qt6 qmake"
else
    QMAKE_CANDIDATES="qmake-qt5 qmake"
fi
for q in $QMAKE_CANDIDATES; do
    if command -v "$q" &> /dev/null; then
        v=$("$q" -query QT_VERSION 2>/dev/null)
        if [[ "$v" == "${QT_MAJOR}."* ]]; then
            QT_VERSION="$v"
            break
        fi
    fi
done

# If qmake returned nothing, fall back to windeployqt (already filtered by version).
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] && [ -n "$WINDEPLOYQT_CMD" ]; then
    QT_VERSION=$("$WINDEPLOYQT_CMD" --version 2>/dev/null | grep -o -E '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || echo "")
fi

# Fallback value consistent with the detected major version.
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    if [ "$QT_MAJOR" = "6" ]; then QT_VERSION="6.0.0"; else QT_VERSION="5.15.18"; fi
fi

# Base name of the final archive. The environment tag reflects the MSYS2 shell
# actually used (ucrt64 / mingw64 / clang64) instead of a hard-coded 'mingw64'
# that lied on UCRT64/CLANG64 builds.
ENV_TAG=$(echo "${MSYSTEM:-mingw64}" | tr '[:upper:]' '[:lower:]')
ARCHIVE_BASE="RetroShare-${VERSION}-Windows-Portable${GIT_SUFFIX}-Qt-${QT_VERSION}-${ENV_TAG}-msys2"
DEPLOY_DIR="${BUILD_DIR}/${ARCHIVE_BASE}"

echo "  Target Package Name: ${ARCHIVE_BASE}"
echo "  Deploy Directory:    ${DEPLOY_DIR}"

# Clean up previous deployment generations
echo ">>> Cleaning up previous deployment directories..."
# Only remove old deployment DIRECTORIES, not the .7z / .zip archives produced
# by previous runs (which match the same pattern).
find "$BUILD_DIR" -maxdepth 1 -type d -name "RetroShare-*-Windows-Portable*" -exec rm -rf {} + 2>/dev/null || true
mkdir -p "$DEPLOY_DIR"

# 3. Create the full directory tree expected by RetroShare
echo ">>> Creating target directory layout..."
mkdir -p "$DEPLOY_DIR/Data/extensions6" # Directory for RetroShare plugins
mkdir -p "$DEPLOY_DIR/qss"               # UI stylesheets
mkdir -p "$DEPLOY_DIR/stylesheets"       # Chat stylesheets
mkdir -p "$DEPLOY_DIR/sounds"            # UI system sounds
mkdir -p "$DEPLOY_DIR/translations"      # Translation files (RetroShare + Qt)
mkdir -p "$DEPLOY_DIR/license"           # License files
mkdir -p "$DEPLOY_DIR/log"               # Runtime log directory

# 4. Create the 'portable' marker file
echo ">>> Creating 'portable' mode indicator file..."
touch "$DEPLOY_DIR/portable"

# 5. Copy the executables and DLLs produced by the build
echo ">>> Copying compiled binaries..."
for exe in retroshare-gui.exe retroshare-service.exe retroshare-friendserver.exe; do
    found_exe=$(find "$BUILD_DIR" -path "$DEPLOY_DIR" -prune -o -name "$exe" -print | head -n 1)
    if [ -n "$found_exe" ]; then
        echo "  Found and copying: $found_exe"
        cp "$found_exe" "$DEPLOY_DIR/"
    else
        echo "  WARNING: Executable '$exe' not found. Skipping."
    fi
done

echo ">>> Copying local build libraries (DLLs)..."
# Find and copy the DLLs built internally by the project (RNP, CMark, RetroShare, Restbed)
# NB: we prune "$DEPLOY_DIR" (located under "$BUILD_DIR") to avoid re-finding the
# DLLs already copied and mistakenly deploying a stale variant.
find "$BUILD_DIR" -path "$DEPLOY_DIR" -prune -o -name "libcmark.dll" -exec cp {} "$DEPLOY_DIR/" \; 2>/dev/null || true
find "$BUILD_DIR" -path "$DEPLOY_DIR" -prune -o -name "*retroshare.dll" -exec cp {} "$DEPLOY_DIR/" \; 2>/dev/null || true
find "$BUILD_DIR" -path "$DEPLOY_DIR" -prune -o -name "*restbed*.dll" -exec cp {} "$DEPLOY_DIR/" \; 2>/dev/null || true
if [ -d "supportlibs/librnp/Build" ]; then
    find "supportlibs/librnp/Build" -name "librnp.dll" -exec cp {} "$DEPLOY_DIR/" \; 2>/dev/null || true
fi
find "$BUILD_DIR" -path "$DEPLOY_DIR" -prune -o -name "librnp.dll" -exec cp {} "$DEPLOY_DIR/" \; 2>/dev/null || true

echo ">>> Copying RetroShare plugins..."
if [ -d "$BUILD_DIR/plugins" ]; then
    find "$BUILD_DIR/plugins" -name "*.dll" -exec cp {} "$DEPLOY_DIR/Data/extensions6/" \; 2>/dev/null || true
    echo "  Plugins copied to Data/extensions6."
else
    echo "  WARNING: Plugins directory not found. Skipping plugins."
fi

# 6. Copy RetroShare's static assets (QSS, sounds, etc.)
echo ">>> Copying RetroShare static assets..."

if [ -d "retroshare-gui/src/qss" ]; then
    cp -r retroshare-gui/src/qss/* "$DEPLOY_DIR/qss/" 2>/dev/null || true
fi

if [ -d "retroshare-gui/src/gui/qss/chat" ]; then
    cp -r retroshare-gui/src/gui/qss/chat/* "$DEPLOY_DIR/stylesheets/" 2>/dev/null || true
    # Remove the useless directories, following the historical pack.bat
    rm -rf "$DEPLOY_DIR/stylesheets/compact" "$DEPLOY_DIR/stylesheets/standard"
fi

if [ -d "retroshare-gui/src/sounds" ]; then
    cp -r retroshare-gui/src/sounds/* "$DEPLOY_DIR/sounds/" 2>/dev/null || true
fi

if [ -d "retroshare-gui/src/license" ]; then
    cp -r retroshare-gui/src/license/* "$DEPLOY_DIR/license/" 2>/dev/null || true
fi

if [ -f "libbitdht/src/bitdht/bdboot.txt" ]; then
    cp "libbitdht/src/bitdht/bdboot.txt" "$DEPLOY_DIR/"
    echo "  Copied bdboot.txt"
fi

# 7. Copy the translation files
echo ">>> Copying translations (RetroShare + Qt system)..."
if [ -d "retroshare-gui/src/translations" ]; then
    find "retroshare-gui/src/translations" -name "*.qm" -exec cp {} "$DEPLOY_DIR/translations/" \; 2>/dev/null || true
fi

# Qt translations of the RIGHT major version only (do not mix Qt5 and Qt6 .qm
# files in the same package).
for tdir in \
    "$MINGW_PREFIX/share/qt${QT_MAJOR}/translations" \
    "$MINGW_PREFIX/lib/qt${QT_MAJOR}/translations" \
    "$MINGW_PREFIX/qt${QT_MAJOR}/translations"; do
    if [ -d "$tdir" ]; then
        cp "$tdir"/qt_*.qm "$DEPLOY_DIR/translations/" 2>/dev/null || true
        cp "$tdir"/qtbase_*.qm "$DEPLOY_DIR/translations/" 2>/dev/null || true
        break
    fi
done

# 8. Deploy the Qt dependencies via windeployqt
if [ -f "$DEPLOY_DIR/retroshare-gui.exe" ]; then
    if [ -n "$WINDEPLOYQT_CMD" ]; then
        echo ">>> Running $WINDEPLOYQT_CMD on retroshare-gui.exe..."
        # Convert path to Windows-style for the native windeployqt tool if cygpath is available
        if command -v cygpath &> /dev/null; then
            WIN_EXE_PATH=$(cygpath -w "$DEPLOY_DIR/retroshare-gui.exe")
            echo "  Using Windows path: $WIN_EXE_PATH"
            "$WINDEPLOYQT_CMD" --no-compiler-runtime "$WIN_EXE_PATH" || true
        else
            "$WINDEPLOYQT_CMD" --no-compiler-runtime "$DEPLOY_DIR/retroshare-gui.exe" || true
        fi
    else
        echo "  WARNING: windeployqt tool not found in PATH! Make sure Qt package is installed."
    fi
fi

# Manual fallback for the essential Qt plugins (such as 'platforms' and 'styles').
# Required if windeployqt failed or was not present.
# NB: every fallback plugin below is taken EXCLUSIVELY from the directory of the
# detected major version (qt${QT_MAJOR}). Mixing a Qt6 qwindows.dll with Qt5*.dll
# (or vice versa) causes the "no Qt platform plugin could be initialized" crash.
QT_PLUGIN_DIRS=( \
    "$MINGW_PREFIX/share/qt${QT_MAJOR}/plugins" \
    "$MINGW_PREFIX/lib/qt${QT_MAJOR}/plugins" \
    "$MINGW_PREFIX/qt${QT_MAJOR}/plugins" )

if [ ! -d "$DEPLOY_DIR/platforms" ] || [ ! -f "$DEPLOY_DIR/platforms/qwindows.dll" ]; then
    echo ">>> Manual fallback: Copying Qt${QT_MAJOR} 'platforms' plugin (qwindows.dll)..."
    mkdir -p "$DEPLOY_DIR/platforms"
    COPIED_PLATFORMS=false
    for base in "${QT_PLUGIN_DIRS[@]}"; do
        if [ -f "$base/platforms/qwindows.dll" ]; then
            echo "  Found platforms plugin at: $base/platforms/qwindows.dll"
            cp "$base/platforms/qwindows.dll" "$DEPLOY_DIR/platforms/"
            COPIED_PLATFORMS=true
            break
        fi
    done
    if [ "$COPIED_PLATFORMS" = false ]; then
        echo "  WARNING: Could not find Qt${QT_MAJOR} qwindows.dll in standard MSYS2 paths!"
    fi
fi

echo ">>> Deploying Qt${QT_MAJOR} 'styles' plugins..."
mkdir -p "$DEPLOY_DIR/styles"
for base in "${QT_PLUGIN_DIRS[@]}"; do
    if [ -d "$base/styles" ]; then
        echo "  Found styles plugin directory at: $base/styles"
        cp -r "$base/styles"/* "$DEPLOY_DIR/styles/" 2>/dev/null || true
        break
    fi
done

echo ">>> Deploying Qt${QT_MAJOR} 'imageformats' plugins (essential for SVG/ICO icons)..."
mkdir -p "$DEPLOY_DIR/imageformats"
COPIED_IMAGEFORMATS=false
for base in "${QT_PLUGIN_DIRS[@]}"; do
    if [ -d "$base/imageformats" ]; then
        echo "  Found imageformats plugin directory at: $base/imageformats"
        cp -r "$base/imageformats"/* "$DEPLOY_DIR/imageformats/" 2>/dev/null || true
        COPIED_IMAGEFORMATS=true
        break
    fi
done
if [ "$COPIED_IMAGEFORMATS" = false ]; then
    echo "  WARNING: Could not find Qt${QT_MAJOR} imageformats directory in standard MSYS2 paths!"
fi


# 9. Dynamically resolve MinGW dependencies (ldd)
echo ">>> Resolving system and 3rd party DLL dependencies using ldd..."
TEMP_DEPENDENCY_LIST=$(mktemp)
trap 'rm -f "$TEMP_DEPENDENCY_LIST"' EXIT

PREV_COUNT=0
while true; do
    # Scan every exe and dll present in the deployment folder
    find "$DEPLOY_DIR" \( -name "*.exe" -o -name "*.dll" \) -exec ldd {} \; 2>/dev/null \
        | grep -i "$MINGW_PREFIX/bin" | awk '{print $3}' | sort -u > "$TEMP_DEPENDENCY_LIST"

    CURR_COUNT=$(wc -l < "$TEMP_DEPENDENCY_LIST")
    if [ "$CURR_COUNT" -eq "$PREV_COUNT" ]; then
        break
    fi
    PREV_COUNT=$CURR_COUNT

    while read -r dll_path; do
        if [ -f "$dll_path" ]; then
            dll_name=$(basename "$dll_path")
            # Do not overwrite DLLs already present (Qt or locally built)
            if [ ! -f "$DEPLOY_DIR/$dll_name" ]; then
                echo "  Deploying dependency: $dll_name"
                cp "$dll_path" "$DEPLOY_DIR/"
            fi
        fi
    done < "$TEMP_DEPENDENCY_LIST"
done
rm -f "$TEMP_DEPENDENCY_LIST"

# Detect if we should strip debug symbols (only in explicit Release or MinSizeRel builds)
IS_RELEASE_BUILD=false
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    if grep -q "CMAKE_BUILD_TYPE:STRING=Release" "$BUILD_DIR/CMakeCache.txt" || grep -q "CMAKE_BUILD_TYPE:STRING=MinSizeRel" "$BUILD_DIR/CMakeCache.txt"; then
        IS_RELEASE_BUILD=true
    fi
fi

# 9.5 Stripping debug symbols to reduce file sizes (shrinks exe from 91MB down to ~22MB)
if [ "$IS_RELEASE_BUILD" = false ]; then
    echo ">>> Non-Release build detected (CMakeCache.txt). Skipping stripping of debug symbols to preserve GDB compatibility!"
else
    echo ">>> Stripping debug symbols from our compiled binaries..."
    if command -v strip &> /dev/null; then
        # Strip main executables and core DLLs
        for binary in \
            "$DEPLOY_DIR"/retroshare-gui.exe \
            "$DEPLOY_DIR"/retroshare-service.exe \
            "$DEPLOY_DIR"/retroshare-friendserver.exe \
            "$DEPLOY_DIR"/retroshare.dll \
            "$DEPLOY_DIR"/libretroshare.dll \
            "$DEPLOY_DIR"/libcmark.dll \
            "$DEPLOY_DIR"/librestbed-shared.dll \
            "$DEPLOY_DIR"/librestbed.dll \
            "$DEPLOY_DIR"/librnp.dll; do
            if [ -f "$binary" ]; then
                echo "  Stripping: $(basename "$binary")"
                strip "$binary" || true
            fi
        done
        # Strip plugins
        if [ -d "$DEPLOY_DIR/Data/extensions6" ]; then
            find "$DEPLOY_DIR/Data/extensions6" -name "*.dll" | while read -r plugin; do
                echo "  Stripping plugin: $(basename "$plugin")"
                strip "$plugin" || true
            done
        fi
    else
        echo "  WARNING: 'strip' tool not found. Skipping stripping of debug symbols."
    fi
fi

# 10. Deploy the WebUI (if present)
echo ">>> Deploying WebUI static assets..."
if [ -d "retroshare-webui/src" ]; then
    mkdir -p "$DEPLOY_DIR/webui"
    cp -r retroshare-webui/src/* "$DEPLOY_DIR/webui/" 2>/dev/null || true
    echo "  WebUI copied to deployment folder."
else
    echo "  WebUI source folder not found. Skipping WebUI packaging."
fi

# 11. Create the final archive (7z preferred, zip as fallback)
echo ">>> Packaging final compressed archive..."
(
    cd "$BUILD_DIR" || exit 1

    if command -v 7z &> /dev/null; then
        echo "  Using 7z for ultra high compression..."
        7z a -t7z -mx=9 "${ARCHIVE_BASE}.7z" "${ARCHIVE_BASE}"
        echo "================================================================================"
        echo "SUCCESS: Standalone package created at: ${BUILD_DIR}/${ARCHIVE_BASE}.7z"
        echo "================================================================================"
    elif command -v zip &> /dev/null; then
        echo "  7z not found. Falling back to standard zip..."
        zip -r "${ARCHIVE_BASE}.zip" "${ARCHIVE_BASE}"
        echo "================================================================================"
        echo "SUCCESS: Standalone package created at: ${BUILD_DIR}/${ARCHIVE_BASE}.zip"
        echo "================================================================================"
    else
        echo "================================================================================"
        echo "SUCCESS: Folder generated at: ${DEPLOY_DIR}"
        echo "NOTE: Neither '7z' nor 'zip' commands were found. Archive creation skipped."
        echo "================================================================================"
    fi
)
