#!/bin/bash
# ==============================================================================
# RetroShare macOS Portability & Deployment Packaging Script
# Designed for macOS environment with CMake
# ==============================================================================
set -e

BUILD_DIR="Build-cmake"
APP_NAME="RetroShare"

# 1. Vérifications initiales du dossier de build
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory '$BUILD_DIR' not found. Please compile the project first!"
    exit 1
fi

# On cherche l'application compilée, mais on exclut les dossiers de déploiement précédents
APP_BUNDLE=$(find "$BUILD_DIR" -maxdepth 3 -type d -name "retroshare.app" ! -path "*/RetroShare-*" | head -n 1)

if [ -z "$APP_BUNDLE" ]; then
    echo "ERROR: retroshare.app not found in $BUILD_DIR. Please compile the GUI first."
    exit 1
fi

echo "================================================================================"
echo "          RETROSHARE MACOS DEPLOYMENT GENERATOR"
echo "================================================================================"

# 2. Détermination dynamique des variables de version
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

QT_VERSION=""
if command -v qmake &> /dev/null; then
    QT_VERSION=$(qmake -query QT_VERSION 2>/dev/null)
fi
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    QT_VERSION="UnknownQt"
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

# 3. Copie de l'application dans notre dossier de déploiement
echo ">>> Copying retroshare.app..."
cp -R "$APP_BUNDLE" "$DEPLOY_DIR/"
TARGET_APP="$DEPLOY_DIR/$(basename "$APP_BUNDLE")"

echo ">>> Creating target directory layout..."
APP_RESOURCES="$TARGET_APP/Contents/Resources"
APP_MACOS="$TARGET_APP/Contents/MacOS"

mkdir -p "$APP_MACOS/extensions6"
mkdir -p "$APP_RESOURCES/sounds"
mkdir -p "$APP_RESOURCES/translations"
mkdir -p "$APP_RESOURCES/webui"

# 4. Copie des exécutables annexes (service, friendserver) s'ils existent (non-bloquant)
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

# Copie des plugins
echo ">>> Copying RetroShare plugins..."
if [ -d "$BUILD_DIR/plugins" ]; then
    find "$BUILD_DIR/plugins" \( -name "*.dylib" -o -name "*.so" \) -exec cp {} "$APP_MACOS/extensions6/" \; 2>/dev/null || true
    echo "  Plugins copied to $APP_MACOS/extensions6/."
else
    echo "  INFO: Plugins directory not found. Skipping plugins."
fi

# 5. Copie des ressources statiques
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

# 6. Mise à jour du Info.plist
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

# 7. Déploiement Qt via macdeployqt
echo ">>> Running macdeployqt..."
if command -v macdeployqt &> /dev/null; then
    # macdeployqt copie les frameworks et ajuste les rpaths
    macdeployqt "$TARGET_APP" -always-overwrite
    
    echo ">>> Ad-hoc code signing (required for Apple Silicon)..."
    if command -v codesign &> /dev/null; then
        # On signe d'abord toutes les bibliothèques, plugins (.so/.dylib) et frameworks individuellement
        find "$TARGET_APP" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --sign - {} \; 2>/dev/null || true
        find "$TARGET_APP" -type d -name "*.framework" -exec codesign --force --sign - {} \; 2>/dev/null || true
        # Puis on signe l'application entière
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
