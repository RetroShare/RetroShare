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

# ------------------------------------------------------------------------------
# Détection de la version MAJEURE de Qt réellement utilisée par la compilation.
# On l'extrait des imports du binaire compilé (objdump), et NON d'un qmake pris
# au hasard du PATH : sur une install MSYS2 où Qt5 ET Qt6 cohabitent, `qmake`
# peut pointer vers Qt6 alors que le binaire est lié à Qt5 (ou l'inverse). Si le
# déploiement n'est pas verrouillé sur la bonne version, windeployqt et les
# plugins de secours (platforms/qwindows.dll...) sont pris dans le mauvais Qt,
# d'où le crash "no Qt platform plugin could be initialized" au démarrage.
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

# Recherche de windeployqt correspondant À CETTE version majeure. On valide la
# version retournée par --version : le `windeployqt` nu peut appartenir à l'autre
# Qt sur une install mixte.
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

# Détection de la version complète de Qt via le qmake de la BONNE version majeure.
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

# Si qmake n'a rien donné, on retombe sur windeployqt (déjà filtré par version).
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] && [ -n "$WINDEPLOYQT_CMD" ]; then
    QT_VERSION=$("$WINDEPLOYQT_CMD" --version 2>/dev/null | grep -o -E '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || echo "")
fi

# Valeur de secours cohérente avec la version majeure détectée.
if [[ ! "$QT_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    if [ "$QT_MAJOR" = "6" ]; then QT_VERSION="6.0.0"; else QT_VERSION="5.15.18"; fi
fi

# Nom de base de l'archive finale. L'étiquette d'environnement reflète le shell
# MSYS2 réellement utilisé (ucrt64 / mingw64 / clang64) au lieu d'un 'mingw64'
# figé qui mentait sur les builds UCRT64/CLANG64.
ENV_TAG=$(echo "${MSYSTEM:-mingw64}" | tr '[:upper:]' '[:lower:]')
ARCHIVE_BASE="RetroShare-${VERSION}-Windows-Portable${GIT_SUFFIX}-Qt-${QT_VERSION}-${ENV_TAG}-msys2"
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

# Traductions Qt de la BONNE version majeure uniquement (ne pas mélanger des
# .qm Qt5 et Qt6 dans le même paquet).
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
# NB: tous les plugins de secours ci-dessous sont pris EXCLUSIVEMENT dans le
# répertoire de la version majeure détectée (qt${QT_MAJOR}). Mélanger un
# qwindows.dll Qt6 avec des Qt5*.dll (ou l'inverse) provoque le crash
# "no Qt platform plugin could be initialized".
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
