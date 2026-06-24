#!/bin/bash
# ==============================================================================
# RetroShare macOS Portability & Deployment Packaging Script
# Designed for macOS environment with CMake
# ------------------------------------------------------------------------------
# Why this is more than a one-liner: macdeployqt IS the standard tool and does the
# heavy lifting (bundling Qt frameworks, fixing rpaths). The extra steps around it
# handle RetroShare specifics that macdeployqt does not cover:
#   - RetroShare refuses to load plugins on macOS unless they end in ".dylib",
#     so built plugins are copied/renamed accordingly.
#   - macdeployqt skips .so plugins, so each plugin is passed via -executable=.
#   - Qt imageformats (SVG) are force-copied (macdeployqt sometimes misses them).
#   - Ad-hoc codesign is mandatory on Apple Silicon, otherwise the app won't run.
# ==============================================================================
set -e

# Always operate from the repository root, regardless of where this script lives
# (it resides under build_scripts/OSX/). All paths below are relative to the root.
cd "$(dirname "$0")/../.." || exit 1

# Build directory (override with: BUILD_DIR=mydir ./build_scripts/OSX/deploy-macos.sh)
BUILD_DIR="${BUILD_DIR:-Build-cmake}"
APP_NAME="RetroShare"

# 1. Initial checks on the build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory '$BUILD_DIR' not found. Please compile the project first!"
    exit 1
fi

# Find the compiled app, excluding previous deployment directories
APP_BUNDLE=$(find "$BUILD_DIR" -maxdepth 3 -type d -name "retroshare-gui.app" ! -path "*/RetroShare-*" | head -n 1)

if [ -z "$APP_BUNDLE" ]; then
    echo "ERROR: retroshare-gui.app not found in $BUILD_DIR. Please compile the GUI first."
    exit 1
fi

echo "================================================================================"
echo "          RETROSHARE MACOS DEPLOYMENT GENERATOR"
echo "================================================================================"

# 2. Dynamically determine the version variables
echo ">>> Extracting versioning info from Git and environment..."

DATE_STR=$(date +%Y%m%d)
GIT_DESC=$(git describe --tags --always --dirty 2>/dev/null || echo "0.6.7-unknown")
CLEAN_DESC=${GIT_DESC#v}
VERSION="$CLEAN_DESC"
GIT_SUFFIX=""

if [[ "$CLEAN_DESC" == *-* ]]; then
    VERSION=$(echo "$CLEAN_DESC" | cut -d'-' -f1)
    COMMIT_INFO=$(echo "$CLEAN_DESC" | cut -d'-' -f2-)
    GIT_SUFFIX="-${DATE_STR}-${COMMIT_INFO}"
else
    GIT_SUFFIX="-${DATE_STR}"
fi

# Detect the Qt version for the archive name from the ACTUAL built binary, NOT
# from whichever qmake is first in PATH. With both qt (Qt6) and qt@5 installed,
# `qmake` resolves to one of them regardless of what the app was built against,
# mislabelling the archive (a Qt6 build packaged as "Qt-5..."). Read the QtCore
# the app links - the same signal used for plugin bundling further below - and
# query the matching Homebrew keg's qmake.
GUI_BIN="$APP_BUNDLE/Contents/MacOS/retroshare-gui"
if otool -L "$GUI_BIN" 2>/dev/null | grep -q "QtCore.framework/Versions/5"; then
    QT_MAJOR=5
    QMAKE_BIN="$(brew --prefix qt@5 2>/dev/null)/bin/qmake"
else
    QT_MAJOR=6
    QMAKE_BIN="$(brew --prefix qt 2>/dev/null)/bin/qmake"
fi
echo "  Qt major linked by the app (otool): Qt${QT_MAJOR}"

QT_VERSION=""
[ -x "$QMAKE_BIN" ] || QMAKE_BIN="$(command -v qmake)"
if [ -x "$QMAKE_BIN" ]; then
    v=$("$QMAKE_BIN" -query QT_VERSION 2>/dev/null)
    [[ "$v" == "${QT_MAJOR}."* ]] && QT_VERSION="$v"
fi
# Fallback: at least keep the major version right even if qmake is unavailable.
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    QT_VERSION="${QT_MAJOR}.x"
fi

MACVERSION=$(sw_vers -productVersion 2>/dev/null || echo "UnknownOS")
ARCH=$(uname -m)

ARCHIVE_BASE="RetroShare-${VERSION}-macOS-${ARCH}${GIT_SUFFIX}-Qt-${QT_VERSION}"
DEPLOY_DIR="${BUILD_DIR}/${ARCHIVE_BASE}"

echo "  Target Package Name: ${ARCHIVE_BASE}"
echo "  Deploy Directory:    ${DEPLOY_DIR}"

echo ">>> Cleaning up previous deployment directories..."
rm -rf "${BUILD_DIR}/RetroShare-"*"-macOS-"*
mkdir -p "$DEPLOY_DIR"

# 3. Copy the app into our deployment directory
echo ">>> Copying retroshare-gui.app..."
cp -R "$APP_BUNDLE" "$DEPLOY_DIR/"
TARGET_APP="$DEPLOY_DIR/$(basename "$APP_BUNDLE")"

echo ">>> Creating target directory layout..."
APP_RESOURCES="$TARGET_APP/Contents/Resources"
APP_MACOS="$TARGET_APP/Contents/MacOS"

mkdir -p "$APP_MACOS/extensions6"
mkdir -p "$APP_RESOURCES/sounds"
mkdir -p "$APP_RESOURCES/translations"
mkdir -p "$APP_RESOURCES/webui"

# 4. Copy the extra executables (service, friendserver) if present (non-blocking)
echo ">>> Copying additional executables if they exist..."
for exe in retroshare-service retroshare-friendserver; do
    found_exe=$(find "$BUILD_DIR" -type f -name "$exe" -perm -0111 | grep -v "CMakeFiles" | head -n 1)
    if [ -n "$found_exe" ]; then
        echo "  Found and copying: $found_exe"
        cp "$found_exe" "$APP_MACOS/"
    else
        echo "  INFO: Executable '$exe' not found. Skipping."
    fi
done

# Copy the plugins
echo ">>> Copying RetroShare plugins..."
if [ -d "$BUILD_DIR/plugins" ]; then
    mkdir -p "$APP_RESOURCES/extensions6"
    for plugin in $(find "$BUILD_DIR/plugins" -type f \( -name "*.dylib" -o -name "*.so" \)); do
        filename=$(basename "$plugin")
        # RetroShare on macOS refuses to load plugins unless they end in .dylib!
        newname="${filename%.*}.dylib"
        cp "$plugin" "$APP_RESOURCES/$newname"
        cp "$plugin" "$APP_RESOURCES/extensions6/$newname"
    done
    echo "  Plugins copied and renamed to .dylib in $APP_RESOURCES/ and $APP_RESOURCES/extensions6/."
else
    echo "  INFO: Plugins directory not found. Skipping plugins."
fi

# 5. Copy the static assets
echo ">>> Copying RetroShare static assets..."

if [ -d "retroshare-gui/src/sounds" ]; then
    cp -r retroshare-gui/src/sounds/* "$APP_RESOURCES/sounds/" 2>/dev/null || true
fi

if [ -f "libbitdht/src/bitdht/bdboot.txt" ]; then
    cp "libbitdht/src/bitdht/bdboot.txt" "$APP_RESOURCES/"
    echo "  Copied bdboot.txt"
fi

if [ -d "retroshare-gui/src/translations" ]; then
    find "retroshare-gui/src/translations" -name "*.qm" -exec cp {} "$APP_RESOURCES/translations/" \; 2>/dev/null || true
fi

if [ -d "retroshare-webui/src" ]; then
    cp -r retroshare-webui/src/* "$APP_RESOURCES/webui/" 2>/dev/null || true
    echo "  WebUI copied to app resources."
fi

# 6. Update Info.plist
echo ">>> Updating Info.plist..."
PLIST_PATH="$TARGET_APP/Contents/Info.plist"
if command -v /usr/libexec/PlistBuddy &> /dev/null; then
    /usr/libexec/PlistBuddy -c "Delete :CFBundleGetInfoString" "$PLIST_PATH" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Delete :CFBundleVersion" "$PLIST_PATH" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :CFBundleVersion string $VERSION" "$PLIST_PATH" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Delete :CFBundleShortVersionString" "$PLIST_PATH" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :CFBundleShortVersionString string $VERSION" "$PLIST_PATH" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Delete :NSRequiresAquaSystemAppearance" "$PLIST_PATH" 2>/dev/null || true
else
    echo "  WARNING: PlistBuddy not found, skipping Info.plist update."
fi

# 7. Qt deployment via macdeployqt
echo ">>> Running macdeployqt..."
# Use the SAME Qt the app was built against — NOT whatever macdeployqt happens to
# be first in PATH. With both qt (Qt6) and qt@5 installed, PATH often points at
# qt@5, which copies Qt5 plugins into a Qt6 app; the Qt5 cocoa plugin then can't
# load ("Could not find the Qt platform plugin cocoa"). Detect the major version
# from the app's QtCore link and pick the matching keg + its macdeployqt/plugins.
if otool -L "$TARGET_APP/Contents/MacOS/retroshare-gui" 2>/dev/null | grep -q "QtCore.framework/Versions/5"; then
    QT_PREFIX="$(brew --prefix qt@5 2>/dev/null)"
else
    QT_PREFIX="$(brew --prefix qt 2>/dev/null)"
fi
MACDEPLOY_BIN="$QT_PREFIX/bin/macdeployqt"
[ -x "$MACDEPLOY_BIN" ] || MACDEPLOY_BIN="$(command -v macdeployqt)"
QT_PLUGINS_DIR="$QT_PREFIX/share/qt/plugins"
[ -d "$QT_PLUGINS_DIR" ] || QT_PLUGINS_DIR="$QT_PREFIX/plugins"

if [ -x "$MACDEPLOY_BIN" ]; then
    echo ">>> Using Qt toolchain: $QT_PREFIX (macdeployqt: $MACDEPLOY_BIN)"

    # Force-copy the essential Qt plugin categories. Without "platforms" (the
    # mandatory cocoa plugin) the app aborts at launch with "Could not find the Qt
    # platform plugin cocoa" — macdeployqt has been seen to skip it here. styles/tls
    # are needed for native look and HTTPS; imageformats = SVG icons.
    echo ">>> Forcing copy of essential Qt plugins from $QT_PLUGINS_DIR..."
    for cat in platforms styles imageformats iconengines tls networkinformation; do
        if [ -d "$QT_PLUGINS_DIR/$cat" ]; then
            mkdir -p "$TARGET_APP/Contents/PlugIns/$cat"
            cp "$QT_PLUGINS_DIR/$cat/"*.dylib "$TARGET_APP/Contents/PlugIns/$cat/" 2>/dev/null || true
            echo "    + $cat"
        fi
    done

    # Tell Qt where the bundled plugins live (macdeployqt normally writes this).
    mkdir -p "$TARGET_APP/Contents/Resources"
    [ -f "$TARGET_APP/Contents/Resources/qt.conf" ] || printf '[Paths]\nPlugins = PlugIns\n' > "$TARGET_APP/Contents/Resources/qt.conf"

    # By default macdeployqt ignores .so files; force them with -executable
    EXTRA_EXECS=""
    for plugin in "$TARGET_APP"/Contents/Resources/*.so "$TARGET_APP"/Contents/Resources/*.dylib "$TARGET_APP"/Contents/Resources/extensions6/*.so "$TARGET_APP"/Contents/Resources/extensions6/*.dylib "$TARGET_APP"/Contents/PlugIns/*/*.dylib; do
        if [ -f "$plugin" ]; then
            EXTRA_EXECS="$EXTRA_EXECS -executable=$plugin"
        fi
    done
    
    # macdeployqt copies the frameworks and fixes the rpaths
    "$MACDEPLOY_BIN" "$TARGET_APP" -always-overwrite $EXTRA_EXECS

    # --------------------------------------------------------------------------
    # Bundle the transitive third-party Homebrew dependencies that macdeployqt
    # misses (the ffmpeg chain: libavcodec -> libswresample, etc.).
    #
    # macdeployqt copies each plugin's DIRECT dependencies (e.g. VOIP ->
    # libavcodec, libavutil, libspeex) but does NOT recurse into them: libavcodec
    # keeps an absolute reference to /opt/homebrew/.../libswresample.6.dylib.
    # On the build Mac that file exists (Homebrew) so the bundle "works"; on a
    # clean Mac it is missing and the plugin fails to load.
    #
    # DELIBERATELY NARROW SCOPE: only the flat third-party dylibs in Frameworks/
    # and the RetroShare plugins in Resources/ are processed. Qt is NOT touched:
    # neither the .frameworks (binary with no extension) nor Contents/PlugIns/ —
    # Qt is already handled by macdeployqt, and sweeping it here would re-copy all
    # of Qt in bulk. References are rewritten to @executable_path/../Frameworks,
    # which points to Contents/Frameworks for ANY binary in the bundle.
    # --------------------------------------------------------------------------
    echo ">>> Bundling transitive third-party (ffmpeg) dependencies missed by macdeployqt..."
    FRAMEWORKS_DIR="$TARGET_APP/Contents/Frameworks"
    mkdir -p "$FRAMEWORKS_DIR"

    # Mach-O files to make self-contained: flat third-party dylibs in Frameworks/
    # (ffmpeg, speex, openssl...) + RetroShare plugins in Resources/. No PlugIns/
    # and no Qt .frameworks (their binary has no .dylib extension, so never listed).
    list_macho() {
        {
            find "$FRAMEWORKS_DIR" -maxdepth 1 -type f -name "*.dylib"
            find "$TARGET_APP/Contents/Resources" -maxdepth 2 -type f -name "*.dylib"
        } | sort -u
    }
    # Homebrew/MacPorts dependencies of a Mach-O, EXCLUDING its own install id
    # (first line of otool -L = LC_ID_DYLIB, not a dependency) and all of Qt.
    homebrew_deps() {
        local id
        id=$(otool -D "$1" 2>/dev/null | sed -n '2p')
        otool -L "$1" 2>/dev/null | awk 'NR>1{print $1}' \
            | grep -E '^(/opt/homebrew|/usr/local|/opt/local)/' \
            | grep -vE '/[Qq]t@?[0-9]*/' \
            | grep -vxF "${id:-@@none@@}" \
            || true
    }

    # 1) Pull in the missing libs until no new one appears.
    changed=1
    while [ "$changed" -eq 1 ]; do
        changed=0
        while IFS= read -r macho; do
            for dep in $(homebrew_deps "$macho"); do
                base=$(basename "$dep")
                if [ ! -f "$FRAMEWORKS_DIR/$base" ]; then
                    echo "    + bundling $base (needed by $(basename "$macho"))"
                    cp -L "$dep" "$FRAMEWORKS_DIR/$base"
                    chmod u+w "$FRAMEWORKS_DIR/$base"
                    install_name_tool -id "@rpath/$base" "$FRAMEWORKS_DIR/$base" 2>/dev/null
                    changed=1
                fi
            done
        done < <(list_macho)
    done

    # 2) Rewrite every remaining Homebrew reference to point inside the bundle.
    while IFS= read -r macho; do
        for dep in $(homebrew_deps "$macho"); do
            install_name_tool -change "$dep" \
                "@executable_path/../Frameworks/$(basename "$dep")" "$macho" 2>/dev/null
        done
    done < <(list_macho)

    # 3) Verify: NO non-bundled third-party reference must remain.
    LEAKS=$(while IFS= read -r m; do homebrew_deps "$m"; done < <(list_macho) | sort -u)
    if [ -n "$LEAKS" ]; then
        echo "  WARNING: residual non-bundled references remain (bundle NOT self-contained):"
        echo "$LEAKS" | sed 's/^/    /'
    else
        echo "  OK: third-party libs self-contained (no /opt/homebrew or /usr/local references left)."
    fi


    echo ">>> Ad-hoc code signing (required for Apple Silicon)..."
    if command -v codesign &> /dev/null; then
        # First sign every library, plugin (.so/.dylib) and framework individually
        find "$TARGET_APP" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --sign - {} \; 2>/dev/null || true
        find "$TARGET_APP" -type d -name "*.framework" -exec codesign --force --sign - {} \; 2>/dev/null || true
        # Then sign the whole application
        codesign --force --deep --sign - "$TARGET_APP"
    fi

    echo ">>> Creating dmg archive using hdiutil..."
    DMG_FILE="${BUILD_DIR}/${ARCHIVE_BASE}.dmg"
    rm -f "$DMG_FILE"
    if command -v hdiutil &> /dev/null; then
        hdiutil create -volname "${APP_NAME}" -srcfolder "$TARGET_APP" -ov -format UDZO "$DMG_FILE"
        
        if [ -f "$DMG_FILE" ]; then
            echo "================================================================================"
            echo "SUCCESS: Standalone package created at: $DMG_FILE"
            echo "================================================================================"
        else
            echo "WARNING: .dmg not generated."
        fi
    else
        echo "WARNING: hdiutil not found, cannot create .dmg."
    fi
else
    echo "  WARNING: macdeployqt tool not found in PATH! Make sure Qt is installed and its bin folder is in PATH."
    echo "  Cannot bundle Qt dependencies or create DMG automatically."
    echo "================================================================================"
    echo "SUCCESS: App bundle generated at: ${TARGET_APP}"
    echo "================================================================================"
fi
