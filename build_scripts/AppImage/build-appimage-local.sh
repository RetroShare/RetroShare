#!/usr/bin/env bash
#
# Local AppImage packager for the RetroShare GUI — CMake build, no qmake.
#
# It does NOT build anything: it packages the binary already produced by the
# CMake build documented in BUILD-cmake.md (Build-cmake/retroshare-gui/retroshare-gui).
# Build first with CMake, then run this to wrap the result into an .AppImage.
#
# Bundling tool: linuxdeploy + linuxdeploy-plugin-qt (actively maintained).
# (probonopd/linuxdeployqt "continuous" build 107 fails silently right after the
#  icon stage on this toolchain, so we do not use it.)
#
set -euo pipefail

# --- paths --------------------------------------------------------------------
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"          # repo root (RetroShare.clean)
BUILDDIR="$ROOT/Build-cmake"               # CMake build dir (see BUILD-cmake.md)
BIN="$BUILDDIR/retroshare-gui/retroshare-gui"
OUTDIR="$ROOT/Build-appimage"
APPDIR="$OUTDIR/AppDir"
TOOLS="$HERE/tools"
LD="$TOOLS/linuxdeploy-x86_64.AppImage"
LD_QT="$TOOLS/linuxdeploy-plugin-qt-x86_64.AppImage"

# Type-2 AppImages need libfuse2, which Ubuntu 24.04 doesn't ship by default.
# Extracting instead of FUSE-mounting sidesteps that dependency.
export APPIMAGE_EXTRACT_AND_RUN=1

# linuxdeploy-plugin-qt locates Qt through qmake. It must be the qmake of the
# SAME Qt major the binary links (Qt5 here — see the Qt5 build in BUILD-cmake.md).
export QMAKE="${QMAKE:-$(command -v qmake-qt5 || command -v qmake || true)}"

# --- 0. require the CMake build -----------------------------------------------
if [ ! -x "$BIN" ]; then
    cat >&2 <<EOF
ERROR: CMake build not found at:
    $BIN

Build it first (Qt5, GUI) from the repo root — see BUILD-cmake.md:

    rm -rf Build-cmake
    cmake -G Ninja -B Build-cmake -S . \\
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \\
      -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \\
      -DRS_GUI=ON -DRS_SERVICE=OFF -DRS_FRIENDSERVER=OFF -DRS_PLUGINS=OFF \\
      -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_FORUM_DEEP_INDEX=OFF
    cmake --build Build-cmake -j\$(nproc)

then re-run this script.
EOF
    exit 1
fi

# --- 1. fetch linuxdeploy + qt plugin -----------------------------------------
mkdir -p "$TOOLS"
[ -x "$LD" ] || { echo ">>> downloading linuxdeploy ..."; \
    wget -q --show-progress -O "$LD" \
      https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage; \
    chmod +x "$LD"; }
[ -x "$LD_QT" ] || { echo ">>> downloading linuxdeploy-plugin-qt ..."; \
    wget -q --show-progress -O "$LD_QT" \
      https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage; \
    chmod +x "$LD_QT"; }

# --- 2. assemble a clean FHS AppDir from the CMake build ----------------------
# (The project's own install() rules are not FHS-clean for AppImage — desktop
#  lands in prefix/data/ and RS_SERVICE_DESKTOP is OFF by default — so we lay
#  the tree out by hand instead of relying on `cmake --install`.)
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/scalable/apps"

install -m 0755 "$BIN" "$APPDIR/usr/bin/retroshare-gui"

# icons: hicolor png set + scalable svg
for sz in 24x24 48x48 64x64 128x128; do
    mkdir -p "$APPDIR/usr/share/icons/hicolor/$sz/apps"
    cp "$ROOT/data/$sz/apps/retroshare.png" \
       "$APPDIR/usr/share/icons/hicolor/$sz/apps/retroshare.png"
done
cp "$ROOT/data/retroshare.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/retroshare.svg"

# .desktop: take the shipped one, fix Icon (bare name) and Exec (CMake binary name)
DESKTOP="$APPDIR/usr/share/applications/retroshare.desktop"
cp "$ROOT/data/retroshare.desktop" "$DESKTOP"
sed -i 's/^Icon=.*/Icon=retroshare/' "$DESKTOP"
sed -i 's|^Exec=/usr/bin/retroshare|Exec=retroshare-gui|' "$DESKTOP"

# --- 3. bundle Qt + libs and emit the .AppImage -------------------------------
export VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"

cd "$OUTDIR"
"$LD" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/retroshare-gui" \
    --desktop-file "$DESKTOP" \
    --icon-file "$ROOT/data/128x128/apps/retroshare.png" --icon-filename retroshare \
    --plugin qt \
    --output appimage

echo
echo ">>> done. AppImage(s):"
ls -1 "$OUTDIR"/*.AppImage 2>/dev/null || echo "    (none produced — check the log above)"
