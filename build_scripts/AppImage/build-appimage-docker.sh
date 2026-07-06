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

echo
echo ">>> done. Portable AppImage(s):"
ls -1 "$ROOT/Build-appimage"/*.AppImage 2>/dev/null || echo "    (none — check the log above)"
