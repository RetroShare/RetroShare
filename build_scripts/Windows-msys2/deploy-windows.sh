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

# Toujours travailler depuis la racine du dépôt (ce script vit sous
# build_scripts/Windows-msys2/), sinon les chemins relatifs (retroshare-gui/src,
# libbitdht, ...) sont muets et produisent un paquet incomplet sans erreur.
cd "$(dirname "$0")/../.." || exit 1

# Build directory (override with: BUILD_DIR=mydir ./build_scripts/Windows-msys2/deploy-windows.sh)
BUILD_DIR="${BUILD_DIR:-Build-cmake}"

# Préfixe de l'environnement MSYS2 utilisé pour la compilation
# (mingw64 / ucrt64 / clang64). Indispensable pour retrouver le runtime
# compilateur et les plugins Qt quel que soit l'environnement employé.
MINGW_PREFIX="${MSYSTEM_PREFIX:-/mingw64}"

# 1. Vérifications initiales du dossier de build
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory '$BUILD_DIR' not found. Please compile the project first!"
    exit 1
fi

echo "================================================================================"
echo "          RETROSHARE WINDOWS DEPLOYMENT GENERATOR"
echo "================================================================================"

# 2. Détermination dynamique des variables de version (inspiré de qmake)
echo ">>> Extracting versioning info from Git and environment..."

# Date au format YYYYMMDD
DATE_STR=$(date +%Y%m%d)

# Récupération du describe de Git (ex: v0.6.7.2-892-g5341b777d-dirty)
GIT_DESC=$(git describe --tags --always --dirty 2>/dev/null || echo "0.6.7-unknown")
CLEAN_DESC=${GIT_DESC#v} # Enlève le 'v' initial

# Valeurs par défaut
VERSION="$CLEAN_DESC"
GIT_SUFFIX=""

if [[ "$CLEAN_DESC" == *-* ]]; then
    # Format: tag-commits-hash[-dirty] (ex: 0.6.7.2-892-g5341b777d-dirty)
    VERSION=$(echo "$CLEAN_DESC" | cut -d'-' -f1)
    COMMIT_INFO=$(echo "$CLEAN_DESC" | cut -d'-' -f2-)
    GIT_SUFFIX="-${DATE_STR}-${COMMIT_INFO}"
else
    # Si on est pile sur un tag
    GIT_SUFFIX="-${DATE_STR}"
fi

# Recherche de l'exécutable windeployqt adéquat
WINDEPLOYQT_CMD=""
for cmd in windeployqt windeployqt-qt5 windeployqt-qt6; do
    if command -v "$cmd" &> /dev/null; then
        WINDEPLOYQT_CMD="$cmd"
        break
    fi
done

# Détection de la version de Qt
QT_VERSION=""
if command -v qmake &> /dev/null; then
    QT_VERSION=$(qmake -query QT_VERSION 2>/dev/null)
fi

# Si qmake n'a pas donné de version propre, on cherche avec windeployqt
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] && [ -n "$WINDEPLOYQT_CMD" ]; then
    QT_VERSION=$("$WINDEPLOYQT_CMD" --version 2>/dev/null | grep -o -E '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || echo "")
fi

# Valeur de secours si rien n'a été détecté ou si la version est invalide
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    QT_VERSION="5.15.18"
fi

# Nom de base de l'archive finale
ARCHIVE_BASE="RetroShare-${VERSION}-Windows-Portable${GIT_SUFFIX}-Qt-${QT_VERSION}-mingw64-msys2"
DEPLOY_DIR="${BUILD_DIR}/${ARCHIVE_BASE}"

echo "  Target Package Name: ${ARCHIVE_BASE}"
echo "  Deploy Directory:    ${DEPLOY_DIR}"

# Nettoyage des anciennes générations de déploiement
echo ">>> Cleaning up previous deployment directories..."
# Ne supprimer que les anciens RÉPERTOIRES de déploiement, pas les archives
# .7z / .zip produites lors des runs précédents (qui matchent le même motif).
find "$BUILD_DIR" -maxdepth 1 -type d -name "RetroShare-*-Windows-Portable*" -exec rm -rf {} + 2>/dev/null || true
mkdir -p "$DEPLOY_DIR"

# 3. Création de l'arborescence complète attendue par RetroShare
echo ">>> Creating target directory layout..."
mkdir -p "$DEPLOY_DIR/Data/extensions6" # Répertoire pour les plugins de RetroShare
mkdir -p "$DEPLOY_DIR/qss"               # Feuilles de style de l'interface
mkdir -p "$DEPLOY_DIR/stylesheets"       # Feuilles de style de la messagerie
mkdir -p "$DEPLOY_DIR/sounds"            # Sons système de l'interface
mkdir -p "$DEPLOY_DIR/translations"      # Fichiers de traductions (RetroShare + Qt)
mkdir -p "$DEPLOY_DIR/license"           # Fichiers de licences
mkdir -p "$DEPLOY_DIR/log"               # Dossier de logs de fonctionnement

# 4. Création du fichier témoin 'portable'
echo ">>> Creating 'portable' mode indicator file..."
touch "$DEPLOY_DIR/portable"

# 5. Copie des exécutables et DLL générées par la compilation
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
# Recherche et copie des DLL compilées en interne dans le projet (RNP, CMark, RetroShare, Restbed)
# NB: on élague "$DEPLOY_DIR" (situé sous "$BUILD_DIR") pour éviter de re-trouver
# les DLL déjà copiées et de déployer par erreur une variante stale.
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

# 6. Copie des ressources statiques de RetroShare (QSS, sons, etc.)
echo ">>> Copying RetroShare static assets..."

if [ -d "retroshare-gui/src/qss" ]; then
    cp -r retroshare-gui/src/qss/* "$DEPLOY_DIR/qss/" 2>/dev/null || true
fi

if [ -d "retroshare-gui/src/gui/qss/chat" ]; then
    cp -r retroshare-gui/src/gui/qss/chat/* "$DEPLOY_DIR/stylesheets/" 2>/dev/null || true
    # Supprime les répertoires inutiles d'après le pack.bat historique
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

# 7. Copie des fichiers de traduction
echo ">>> Copying translations (RetroShare + Qt system)..."
if [ -d "retroshare-gui/src/translations" ]; then
    find "retroshare-gui/src/translations" -name "*.qm" -exec cp {} "$DEPLOY_DIR/translations/" \; 2>/dev/null || true
fi

# Copie des traductions Qt de l'environnement MSYS2 MinGW64
QT_TRANS_DIR="$MINGW_PREFIX/share/qt5/translations"
if [ -d "$QT_TRANS_DIR" ]; then
    cp "$QT_TRANS_DIR"/qt_*.qm "$DEPLOY_DIR/translations/" 2>/dev/null || true
fi
QT6_TRANS_DIR="$MINGW_PREFIX/share/qt6/translations"
if [ -d "$QT6_TRANS_DIR" ]; then
    cp "$QT6_TRANS_DIR"/qt_*.qm "$DEPLOY_DIR/translations/" 2>/dev/null || true
    cp "$QT6_TRANS_DIR"/qtbase_*.qm "$DEPLOY_DIR/translations/" 2>/dev/null || true
fi

# 8. Déploiement des dépendances Qt via windeployqt
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

# Fallback manuel pour les plugins Qt essentiels (comme 'platforms' et 'styles')
# Indispensable si windeployqt a échoué ou n'était pas présent.
if [ ! -d "$DEPLOY_DIR/platforms" ] || [ ! -f "$DEPLOY_DIR/platforms/qwindows.dll" ]; then
    echo ">>> Manual fallback: Copying Qt 'platforms' directory (qwindows.dll)..."
    mkdir -p "$DEPLOY_DIR/platforms"
    COPIED_PLATFORMS=false
    for path in \
        "$MINGW_PREFIX/share/qt6/plugins/platforms/qwindows.dll" \
        "$MINGW_PREFIX/share/qt5/plugins/platforms/qwindows.dll" \
        "$MINGW_PREFIX/lib/qt6/plugins/platforms/qwindows.dll" \
        "$MINGW_PREFIX/lib/qt5/plugins/platforms/qwindows.dll"; do
        if [ -f "$path" ]; then
            echo "  Found platforms plugin at: $path"
            cp "$path" "$DEPLOY_DIR/platforms/"
            COPIED_PLATFORMS=true
            break
        fi
    done
    if [ "$COPIED_PLATFORMS" = false ]; then
        echo "  WARNING: Could not find qwindows.dll in standard MSYS2 paths!"
    fi
fi

if [ ! -d "$DEPLOY_DIR/styles" ] || [ ! -f "$DEPLOY_DIR/styles/qwindowsvistastyle.dll" ]; then
    echo ">>> Manual fallback: Copying Qt 'styles' directory (qwindowsvistastyle.dll)..."
    mkdir -p "$DEPLOY_DIR/styles"
    for path in \
        "$MINGW_PREFIX/share/qt6/plugins/styles/qwindowsvistastyle.dll" \
        "$MINGW_PREFIX/share/qt5/plugins/styles/qwindowsvistastyle.dll" \
        "$MINGW_PREFIX/lib/qt6/plugins/styles/qwindowsvistastyle.dll" \
        "$MINGW_PREFIX/lib/qt5/plugins/styles/qwindowsvistastyle.dll"; do
        if [ -f "$path" ]; then
            echo "  Found styles plugin at: $path"
            cp "$path" "$DEPLOY_DIR/styles/"
            break
        fi
    done
fi

echo ">>> Deploying Qt 'imageformats' directory (essential for SVG/ICO icons)..."
mkdir -p "$DEPLOY_DIR/imageformats"
COPIED_IMAGEFORMATS=false
for path in \
    "$MINGW_PREFIX/share/qt6/plugins/imageformats" \
    "$MINGW_PREFIX/share/qt5/plugins/imageformats" \
    "$MINGW_PREFIX/lib/qt6/plugins/imageformats" \
    "$MINGW_PREFIX/lib/qt5/plugins/imageformats"; do
    if [ -d "$path" ]; then
        echo "  Found imageformats plugin directory at: $path"
        cp -r "$path"/* "$DEPLOY_DIR/imageformats/" 2>/dev/null || true
        COPIED_IMAGEFORMATS=true
        break
    fi
done
if [ "$COPIED_IMAGEFORMATS" = false ]; then
    echo "  WARNING: Could not find imageformats directory in standard MSYS2 paths!"
fi


# 9. Résolution dynamique des dépendances MinGW (ldd)
echo ">>> Resolving system and 3rd party DLL dependencies using ldd..."
TEMP_DEPENDENCY_LIST=$(mktemp)
trap 'rm -f "$TEMP_DEPENDENCY_LIST"' EXIT

PREV_COUNT=0
while true; do
    # Scanner tous les exe et dll présents dans le dossier de déploiement
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
            # Ne pas écraser les DLL déjà présentes (Qt ou compilées localement)
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

# 10. Déploiement de la WebUI (si présente)
echo ">>> Deploying WebUI static assets..."
if [ -d "retroshare-webui/src" ]; then
    mkdir -p "$DEPLOY_DIR/webui"
    cp -r retroshare-webui/src/* "$DEPLOY_DIR/webui/" 2>/dev/null || true
    echo "  WebUI copied to deployment folder."
else
    echo "  WebUI source folder not found. Skipping WebUI packaging."
fi

# 11. Création de l'archive finale (7z en priorité, zip en fallback)
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
