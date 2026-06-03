#!/bin/bash

# Exit immediately if any command fails
set -e

# Clean build directories if the "clean" argument is passed
if [ "$1" == "clean" ]; then
    echo "=== Cleaning build directories ==="
    rm -rf libretroshare/Build
    rm -rf retroshare-service/Build
    rm -rf retroshare-friendserver/Build
    rm -rf retroshare-gui/Build
fi

# Detect number of CPU cores for fast parallel compilation
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# Detect Qt5 path on macOS if Homebrew is installed
QT_PREFIX=""
if command -v brew &> /dev/null; then
    QT_PREFIX=$(brew --prefix qt@5)
fi

echo "=== Starting RetroShare Compilation ==="

# 1. Compile libretroshare (core library) and librnp automatically
echo ">>> Step 1: Compiling libretroshare..."
mkdir -p libretroshare/Build && cd libretroshare/Build
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DRS_LIBRETROSHARE_STATIC=OFF -DRS_LIBRETROSHARE_SHARED=ON ..
cmake --build . -j${NPROC}
cd ../..

# 2. Compile retroshare-service (headless daemon)
echo ">>> Step 2: Compiling retroshare-service..."
mkdir -p retroshare-service/Build && cd retroshare-service/Build
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
cmake --build . -j${NPROC}
cd ../..

# 3. Compile retroshare-friendserver
echo ">>> Step 3: Compiling retroshare-friendserver..."
mkdir -p retroshare-friendserver/Build && cd retroshare-friendserver/Build
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
cmake --build . -j${NPROC}
cd ../..

# 4. Compile retroshare-gui (graphical user interface)
echo ">>> Step 4: Compiling retroshare-gui..."
mkdir -p retroshare-gui/Build && cd retroshare-gui/Build
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DCMAKE_PREFIX_PATH="${QT_PREFIX}" \
      -DENABLE_GUI=ON \
      -DENABLE_QT_TRANSLATIONS=ON ..
cmake --build . -j${NPROC}
cd ../..

echo "=== Compilation Successfully Completed! ==="
