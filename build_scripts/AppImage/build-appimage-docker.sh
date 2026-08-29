#!/usr/bin/env bash
#
# Portable RetroShare AppImage — built inside a Debian 11 (glibc 2.31) container.
#
# Produces an AppImage that runs on any distro with glibc >= 2.31 (MX 21,
# Slackware 15, Debian 11/12, Ubuntu 20.04+, Fedora 34+, ...), unlike
# build-appimage-local.sh which bakes in the host's (too-new) glibc.
#
# Prerequisite: Docker (or Podman). Install on Ubuntu with:
#     sudo apt install -y docker.io
#
# Usage:
#     build_scripts/AppImage/build-appimage-docker.sh
#
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
IMAGE=retroshare-appimage-bullseye

# --- pick a working container engine ------------------------------------------
if command -v podman >/dev/null 2>&1; then
    ENGINE="podman"
elif docker info >/dev/null 2>&1; then
    ENGINE="docker"
elif sudo -n docker info >/dev/null 2>&1 || sudo docker info >/dev/null 2>&1; then
    ENGINE="sudo docker"
else
    cat >&2 <<EOF
ERROR: no usable container engine.

Install Docker and retry:
    sudo apt install -y docker.io
    sudo usermod -aG docker \$USER      (then log out/in, or use: newgrp docker)

Or install Podman (rootless, no daemon):
    sudo apt install -y podman
EOF
    exit 1
fi
echo ">>> using container engine: $ENGINE"

# --- 1. build the toolchain image (Debian 11 + deps + AppImage tools) ---------
echo ">>> building image $IMAGE (Debian 11 / glibc 2.31) ..."
$ENGINE build -t "$IMAGE" -f "$HERE/Dockerfile.bullseye" "$HERE"

# --- 2. compile RS + package the AppImage inside the container ----------------
VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"
echo ">>> building + packaging RetroShare $VERSION ..."

# --user + HOME=/tmp so build artifacts land owned by us, not root.
$ENGINE run --rm \
    -v "$ROOT":/src \
    -e VERSION="$VERSION" \
    -e HOME=/tmp \
    -e CLEAN="${CLEAN:-}" \
    --user "$(id -u):$(id -g)" \
    "$IMAGE" bash /src/build_scripts/AppImage/_appimage-inside.sh

# --- 3. rename to the descriptive scheme (host side) --------------------------
#   RetroShare-v<ver>-<YYYYMMDD>-<count>-g<hash>-Qt-<qtver>-glibc-<min>-x86_64.AppImage
# The container leaves its AppDir (AppDir-bullseye) and the AppImage in
# Build-appimage/, so we compute everything here: git describe (version + count +
# hash), the build date, the exact Qt version read from the bundled libQt5Core
# (the legacy build is Qt5-only; its qmake lives only inside the container), and
# the highest GLIBC_2.x symbol across all bundled ELFs (the minimum glibc to run).
OUT="$ROOT/Build-appimage"
APPDIR="$OUT/AppDir-bullseye"
produced="$(ls -1t "$OUT"/*.AppImage 2>/dev/null | head -1 || true)"
if [ -n "$produced" ]; then
    desc="$(git -C "$ROOT" describe --tags --always 2>/dev/null | sed 's/^v//' || echo dev)"
    ver="${desc%%-*}"                                            # 0.6.7.3
    suffix="${desc#*-}"; [ "$suffix" = "$desc" ] && suffix=""    # 1121-ga87999bd2 (empty on a tag)
    date="$(date +%Y%m%d)"
    qtver="$(strings "$APPDIR/usr/lib/libQt5Core.so.5" 2>/dev/null \
             | grep -oE 'Qt 5\.[0-9]+\.[0-9]+' | grep -oE '5\.[0-9]+\.[0-9]+' | head -1 || true)"
    [ -n "$qtver" ] || qtver="unknown"
    glibc="$(find "$APPDIR" -type f \( -name '*.so*' -o -path '*/bin/*' -o -path '*/plugins/*' \) 2>/dev/null \
             | while read -r f; do objdump -T "$f" 2>/dev/null | grep -oE 'GLIBC_2\.[0-9]+(\.[0-9]+)?'; done \
             | sort -uV | tail -1 | sed 's/^GLIBC_//')"
    [ -n "$glibc" ] || glibc="unknown"
    name="RetroShare-v${ver}-${date}${suffix:+-${suffix}}-Qt-${qtver}-glibc-${glibc}-x86_64.AppImage"
    mv -f "$produced" "$OUT/$name"
    echo ">>> renamed -> $name"
fi

echo
echo ">>> done. Portable AppImage(s):"
ls -1 "$ROOT/Build-appimage"/*.AppImage 2>/dev/null || echo "    (none — check the log above)"
