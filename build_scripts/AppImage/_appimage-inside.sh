#!/usr/bin/env bash
#
# Runs INSIDE the Debian 11 container (see Dockerfile.bullseye), invoked by
# build-appimage-docker.sh. Bind-mounts: the repo at /src.
#
# Builds RetroShare (Qt5, full feature set — same flags as the current-glibc
# builds) fresh with CMake into a bullseye-specific build dir (so it never
# clobbers the host's Build-cmake), assembles the AppDir and emits a glibc-2.31
# AppImage into /src/Build-appimage. This is meant for old Linux installs, so it
# is NOT stripped down: it follows the same rules as the other builds.
#
set -euo pipefail

SRC=/src
BUILD="$SRC/Build-cmake-bullseye"          # separate from host Build-cmake
OUT="$SRC/Build-appimage"
APPDIR="$OUT/AppDir-bullseye"

export APPIMAGE_EXTRACT_AND_RUN=1          # no FUSE in containers
export QMAKE=/usr/bin/qmake                # bullseye qmake = Qt5
export PATH=/opt/linuxdeploy:/usr/local/bin:$PATH

# Everything in ONE echo on purpose: a standalone `ldd --version | head -1`
# under `set -o pipefail` returns 141 (SIGPIPE: head closes the pipe, ldd dies
# writing to it) and `set -e` would abort the whole build right here, before
# cmake even runs. Inside a command substitution the failure doesn't reach
# `set -e`, so the toolchain banner can't kill the build.
echo ">>> toolchain: $(cmake --version | head -1) | gcc $(gcc -dumpversion) | glibc $(ldd --version 2>&1 | head -1)"

# --- 1. configure + build (Qt5, same feature flags as the current-glibc builds)-
# Incremental by default: reuse objects from a previous run (set CLEAN=1 to wipe).
# RS_NO_LTO=ON: GCC 10's LTO plugin segfaults (lto1 ICE) linking libretroshare on
# bullseye; LTO is only an optimization, so we turn it off for this build.
# Feature flags mirror build-all-appimages.sh's current-glibc Qt5 build: Qt5 =>
# plugins ON, service + friendserver ON (wanted in every build). FORUM_DEEP_INDEX
# stays OFF here and is flipped to ON for combo by build-all-appimages.sh (via a
# throwaway sed on this file) — so leave that exact flag token untouched below.
[ -n "${CLEAN:-}" ] && rm -rf "$BUILD"
cmake -G Ninja -B "$BUILD" -S "$SRC" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    -DRS_NO_LTO=ON \
    -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
    -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
    -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
    -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON
cmake --build "$BUILD" -j"$(nproc)"

BIN="$BUILD/retroshare-gui/retroshare-gui"
[ -x "$BIN" ] || { echo "ERROR: build produced no binary at $BIN" >&2; exit 1; }

# --- 2. assemble a clean FHS AppDir -------------------------------------------
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/scalable/apps"

install -m 0755 "$BIN" "$APPDIR/usr/bin/retroshare-gui"

for sz in 24x24 48x48 64x64 128x128; do
    mkdir -p "$APPDIR/usr/share/icons/hicolor/$sz/apps"
    cp "$SRC/data/$sz/apps/retroshare.png" \
       "$APPDIR/usr/share/icons/hicolor/$sz/apps/retroshare.png"
done
cp "$SRC/data/retroshare.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/retroshare.svg"

DESKTOP="$APPDIR/usr/share/applications/retroshare.desktop"
cp "$SRC/data/retroshare.desktop" "$DESKTOP"
sed -i 's/^Icon=.*/Icon=retroshare/' "$DESKTOP"
sed -i 's|^Exec=/usr/bin/retroshare|Exec=retroshare-gui|' "$DESKTOP"

# --- 3. bundle Qt + libs, emit the AppImage -----------------------------------
# VERSION is passed in from the host for the output filename.
cd "$OUT"
linuxdeploy-x86_64.AppImage --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/retroshare-gui" \
    --desktop-file "$DESKTOP" \
    --icon-file "$SRC/data/128x128/apps/retroshare.png" --icon-filename retroshare \
    --plugin qt \
    --output appimage

echo
echo ">>> AppImage(s) in Build-appimage:"
ls -1 "$OUT"/*.AppImage 2>/dev/null || echo "    (none — check the log above)"
