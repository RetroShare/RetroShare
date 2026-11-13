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

# web UI: RetroShare resolves its data directory as <exedir>/../share/retroshare
# when running from a relocated tree, so the web files must land exactly there.
if [ -d "$SRC/retroshare-webui/webui" ]; then
    mkdir -p "$APPDIR/usr/share/retroshare/webui"
    cp -r "$SRC/retroshare-webui/webui/." "$APPDIR/usr/share/retroshare/webui/"
    echo ">>> web UI bundled from retroshare-webui/webui"
else
    echo ">>> WARNING: retroshare-webui/webui is missing — no web interface bundled"
fi

# DHT bootstrap list: a new account copies <data dir>/bdboot.txt into its own
# directory at first start (rsinit.cc). A CMake install puts the file in
# share/retroshare, which the hand-built AppDir does not get, so ship it next to
# the web UI or a fresh account starts the DHT with no bootstrap peers.
if [ -f "$SRC/libbitdht/src/bitdht/bdboot.txt" ]; then
    mkdir -p "$APPDIR/usr/share/retroshare"
    cp "$SRC/libbitdht/src/bitdht/bdboot.txt" "$APPDIR/usr/share/retroshare/"
    echo ">>> DHT bootstrap list bundled from libbitdht"
else
    echo ">>> WARNING: libbitdht/src/bitdht/bdboot.txt is missing — a new account"
    echo "    will start the DHT with no bootstrap peers."
fi

# Sounds, chat styles and the extra GUI theme, which the GUI reads from the data
# directory (SoundManager, ChatStyle, Rshare). Only the standard and compact chat
# styles are compiled into the binary; Bubble and Bubble_Compact are not. The
# Windows package and the distribution packages ship the same set.
RSDATA="$APPDIR/usr/share/retroshare"
mkdir -p "$RSDATA/sounds" "$RSDATA/stylesheets" "$RSDATA/qss"
cp -r "$SRC/retroshare-gui/src/sounds/." "$RSDATA/sounds/"
cp -r "$SRC/retroshare-gui/src/qss/." "$RSDATA/qss/"
for style in "$SRC"/retroshare-gui/src/gui/qss/chat/*/; do
    style="${style%/}"
    case "$(basename "$style")" in standard|compact) continue ;; esac
    cp -r "$style" "$RSDATA/stylesheets/"
done
echo ">>> sounds, chat styles ($(ls "$RSDATA/stylesheets" | xargs)) and qss themes bundled"

# plugins (VOIP, FeedReader): RetroShare searches
# <exedir>/../lib/retroshare/extensions6/, i.e. usr/lib/retroshare/extensions6
# from usr/bin/retroshare-gui — the only search path that resolves inside an
# AppImage (PLUGIN_DIR is absolute and would point at the host, and
# ~/.retroshare/extensions6 is the user's, not ours). Their own dependencies are
# deployed in step 3 below.
PLUGINDIR="$APPDIR/usr/lib/retroshare/extensions6"
shopt -s nullglob
RS_PLUGINS=( "$BUILD"/plugins/*.so )
shopt -u nullglob
if [ ${#RS_PLUGINS[@]} -gt 0 ]; then
    mkdir -p "$PLUGINDIR"
    install -m 0644 "${RS_PLUGINS[@]}" "$PLUGINDIR/"
    echo ">>> ${#RS_PLUGINS[@]} plugin(s) bundled: $(basename -a "${RS_PLUGINS[@]}" | tr '\n' ' ')"
else
    echo ">>> WARNING: no plugin in $BUILD/plugins — the AppImage will ship none."
fi

# --- 3. bundle Qt + libs, emit the AppImage -----------------------------------
# VERSION is passed in from the host for the output filename.
# --deploy-deps-only: the plugins must STAY in extensions6 (RetroShare looks them
# up by path), but linuxdeploy still has to bundle what they link and patch their
# rpath. --library would move them into usr/lib, where RS never looks.
DEPS_ONLY=()
[ -d "$PLUGINDIR" ] && DEPS_ONLY=( --deploy-deps-only "$PLUGINDIR" )

cd "$OUT"
linuxdeploy-x86_64.AppImage --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/retroshare-gui" \
    --desktop-file "$DESKTOP" \
    --icon-file "$SRC/data/128x128/apps/retroshare.png" --icon-filename retroshare \
    ${DEPS_ONLY[@]+"${DEPS_ONLY[@]}"} \
    --plugin qt \
    --output appimage

echo
echo ">>> AppImage(s) in Build-appimage:"
ls -1 "$OUT"/*.AppImage 2>/dev/null || echo "    (none — check the log above)"
