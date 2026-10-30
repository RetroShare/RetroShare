# RetroShare — CMake build (Linux / macOS / Windows)

How to build RetroShare from source: the GUI (`retroshare-gui`), the headless
daemon with JSON API / WebUI (`retroshare-service`), the friend server and the
plugins.

The steps, in order:

1. [Get the source](#1-get-the-source) — clone, init the submodules.
2. [Pick Qt5 or Qt6](#2-qt5-or-qt6).
3. Install the dependencies and build, for your platform:
   [Linux](#3-linux-ubuntudebian) · [macOS](#4-macos-homebrew-apple-silicon) ·
   [Windows](#5-windows-msys2--ucrt64-shell).
4. [Run what you built](#6-running-what-you-built).

Every command is run from the **root of the clone**. Every configure command is
prefixed with `rm -rf Build-cmake`, so the build directory is recreated from
scratch (this keeps a switch between Qt5 and Qt6 clean); drop the `rm -rf` for an
incremental rebuild.

Requirements common to all platforms: **CMake ≥ 3.18**, a C++17 compiler, and
**network access during the first configure** — `cmark` (GUI) and, on Windows,
`restbed` are downloaded by CMake at configure time.

---

## 1. Get the source

You need `git` first. It is not installed by default on a fresh Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y git
```

On macOS it comes with the Xcode Command Line Tools (`xcode-select --install`,
see [§4](#4-macos-homebrew-apple-silicon)). **On Windows, skip this section:**
[§5](#5-windows-msys2--ucrt64-shell) covers every step in order.

```bash
git clone https://github.com/RetroShare/RetroShare.git
cd RetroShare
```

Initialise the submodules **without `--remote`** — `--remote` moves libretroshare
off its pinned commit and breaks the build. librnp needs `--recursive` for its
nested `src/libsexpp`:

```bash
git submodule update --init libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/
git submodule update --init --recursive supportlibs/librnp
git submodule update --init supportlibs/restbed supportlibs/libsam3 \
                              supportlibs/udp-discovery-cpp supportlibs/rapidjson
```

### Fix rapidjson (all platforms)

The rapidjson commit pinned by the super-project (`f54b0e47`) carries a broken
assignment operator that recent compilers reject: with **GCC 14 or newer**
(Debian 13, current MSYS2) the build fails with *"assignment of read-only member
'length'"*, and recent Clang may reject it too (GCC 13, e.g. Ubuntu 24.04, still
accepts it). Check out the upstream fix (Tencent/rapidjson #719), which only
removes that operator — it is harmless everywhere, so do it whatever your
compiler:

```bash
git -C supportlibs/rapidjson fetch origin
git -C supportlibs/rapidjson checkout 9bd618f5
```

Side effect: `git status` now shows `supportlibs/rapidjson` as modified, and the
version stamped into the build gets a `-dirty` suffix (see
[Version numbering](#version-numbering)). Both are harmless.

---

## 2. Qt5 or Qt6

The same tree builds against **either** Qt5 or Qt6. When both are installed,
**Qt6 is picked**; the Qt5 commands below force Qt5 with
`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`.

A Qt6 build differs in two ways only:
- different Qt packages (listed per platform below);
- **`-DRS_PLUGINS=OFF`** — the VOIP and FeedReader plugins are not ported to Qt6
  yet in this tree.

If you want the plugins, build with Qt5.

---

## 3. Linux (Ubuntu/Debian)

The CMake CI builds Qt5 on Ubuntu 22.04 and Qt6 on Ubuntu 24.04. Ubuntu 20.04
ships CMake 3.16, which is too old.

Dependencies — **Qt5**:
```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential g++ cmake ninja-build pkg-config git \
  qtbase5-dev qtbase5-dev-tools qtmultimedia5-dev libqt5svg5-dev \
  libqt5x11extras5-dev qttools5-dev qttools5-dev-tools \
  libssl-dev zlib1g-dev libbz2-dev libbotan-2-dev libjson-c-dev \
  libasio-dev libsqlcipher-dev \
  libspeex-dev libspeexdsp-dev libminiupnpc-dev \
  libavcodec-dev libavutil-dev \
  libx11-dev libxss-dev \
  libxml2-dev libxslt1-dev libcurl4-openssl-dev \
  doxygen python3
```

Dependencies — **Qt6** (only the `qt*` packages differ from the Qt5 list):
```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential g++ cmake ninja-build pkg-config git \
  qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qt6-svg-dev \
  qt6-5compat-dev qt6-tools-dev qt6-tools-dev-tools \
  libssl-dev zlib1g-dev libbz2-dev libbotan-2-dev libjson-c-dev \
  libasio-dev libsqlcipher-dev \
  libspeex-dev libspeexdsp-dev libminiupnpc-dev \
  libavcodec-dev libavutil-dev \
  libx11-dev libxss-dev \
  libxml2-dev libxslt1-dev libcurl4-openssl-dev \
  doxygen python3
```
> On Ubuntu 24.04 the Core5Compat dev headers come from `qt6-5compat-dev` (there
> is no real `libqt6core5compat6-dev`). X11Extras does not exist in Qt6, so it
> has no counterpart in that list. `qt6-svg-dev` pulls in the Qt6 SVG plugins:
> without them the GUI's SVG icons stay blank (the Qt6 CI installs it too). `libx11-dev` /
> `libxss-dev` are needed by both: the GUI uses XScreenSaver for idle detection.

Build — **Qt5**:
```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON
cmake --build Build-cmake -j$(nproc)
```

Build — **Qt6**:
```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=OFF -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON
cmake --build Build-cmake -j$(nproc)
```

> `-j$(nproc)` runs one compile job per core. The largest GUI and library files
> take 1–2 GB of RAM each; on a machine with less than ~2 GB per core, use a
> smaller number (e.g. `-j4`) to avoid running out of memory.

Then see [§6](#6-running-what-you-built) to run it, and
[Debug builds](#7-debug-builds-linux) to investigate a crash.

---

## 4. macOS (Homebrew, Apple Silicon)

Prerequisites: the Xcode Command Line Tools (compiler, `git`) and
[Homebrew](https://brew.sh):
```bash
xcode-select --install
```

Dependencies — **Qt5**:
```bash
brew install cmake ninja pkgconf qt@5 openssl@3 botan@2 sqlcipher miniupnpc \
  json-c asio speex speexdsp ffmpeg libxml2 libxslt bzip2 zlib rapidjson doxygen
```
Dependencies — **Qt6** (only the Qt package differs). Homebrew ships Qt6 as a
**single `qt` formula** that already bundles qt5compat / svg / multimedia /
tools:
```bash
brew install cmake ninja pkgconf qt openssl@3 botan@2 sqlcipher miniupnpc \
  json-c asio speex speexdsp ffmpeg libxml2 libxslt bzip2 zlib rapidjson doxygen
```

> **If `openssl@4` is installed** (`brew list openssl@4` prints files),
> unlink it before configuring, or the build fails in `sslfns.cc` /
> `authssl.cc` with `ASN1_STRING_data` errors:
> ```bash
> brew unlink openssl@4
> ```
> If a build already failed this way, delete `Build-cmake` before configuring
> again. Once RetroShare is built, `brew link openssl@4` restores it.

> **If `brew install qt` fails with** `Cannot install qtbase because conflicting
> formulae are installed … qt@5: because both link conflicting binaries`, your
> qt@5 is (force-)linked. Unlink it, then re-run the install:
> ```bash
> brew unlink qt@5
> brew install qt
> ```
> Both Qt versions then coexist: the Qt6 build finds Qt6 via `Qt6_DIR`, and the
> Qt5 build still finds qt@5 via `Qt5_DIR` (the keg path works whether or not
> qt@5 is linked).

> **Deprecation warnings are expected.** Homebrew prints *"qt@5 has been
> deprecated"* and *"botan@2 has been deprecated … It was disabled on
> 2025-12-31"*: both still install, as warnings only. Homebrew ships ready-made
> botan@2 packages for macOS 14 to 26 on Apple Silicon (macOS 14 only on Intel);
> on a newer macOS, use Botan 3 instead (see the next note). The Botan 3 route is
> not covered by the CI on macOS.

> **Botan:** with several Botan versions installed (`botan` = v3, `botan@2`,
> `botan@3`), a fresh configure can mix Botan 3 headers with the Botan 2 library
> → `Undefined symbols … Botan::…` linker errors in librnp. Both commands below
> pin it with `-DBOTAN_ROOT_DIR="$(brew --prefix botan@2)"` so headers and
> library come from the **same** keg. To build against Botan 3 instead, install
> `botan` in place of `botan@2` and replace `$(brew --prefix botan@2)` with
> `$(brew --prefix botan)` in the command; librnp then links `libbotan-3`.

> **Which macOS versions the build runs on:** Homebrew installs libraries built
> for the macOS of the machine, so what you build runs on that macOS version and
> later ones, not on older ones. The commands below set
> `CMAKE_OSX_DEPLOYMENT_TARGET` to that same version; without it, the linker warns
> `building for macOS-X, but linking with dylib … built for newer version Y`
> whenever the Command Line Tools are older than the system.

The commands below first collect the include/lib directories of every Homebrew
package into compiler and linker flags. The loop skips every `qt*` package on
purpose: Qt is found through `Qt5_DIR` / `Qt6_DIR`, and pulling the other Qt's
headers in would mix Qt5 and Qt6.

Build — **Qt5**:
```bash
HB="$(brew --prefix)"; QT5="$(brew --prefix qt@5)"
IFLAGS="-I$HB/include"; LFLAGS="-L$HB/lib"; PREFIX="$QT5;$HB"
for d in "$HB"/opt/*; do
  case "${d##*/}" in qt*) continue;; esac
  [ -d "$d/include" ] && IFLAGS="$IFLAGS -I$d/include"
  [ -d "$d/lib" ]     && LFLAGS="$LFLAGS -L$d/lib"
  PREFIX="$PREFIX;$d"
done

rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$(sw_vers -productVersion | cut -d. -f1).0" \
  -DCMAKE_PREFIX_PATH="$PREFIX" -DQt5_DIR="$QT5/lib/cmake/Qt5" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DBOTAN_ROOT_DIR="$(brew --prefix botan@2)" \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON \
  -DCMAKE_C_FLAGS="$IFLAGS" -DCMAKE_CXX_FLAGS="$IFLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$LFLAGS" \
  -DCMAKE_SHARED_LINKER_FLAGS="$LFLAGS" \
  -DCMAKE_MODULE_LINKER_FLAGS="$LFLAGS"
cmake --build Build-cmake -j$(sysctl -n hw.ncpu)
```

Build — **Qt6**:
```bash
HB="$(brew --prefix)"; QT6="$(brew --prefix qt)"
IFLAGS="-I$HB/include"; LFLAGS="-L$HB/lib"; PREFIX="$QT6;$HB"
for d in "$HB"/opt/*; do
  case "${d##*/}" in qt*) continue;; esac
  [ -d "$d/include" ] && IFLAGS="$IFLAGS -I$d/include"
  [ -d "$d/lib" ]     && LFLAGS="$LFLAGS -L$d/lib"
  PREFIX="$PREFIX;$d"
done

rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$(sw_vers -productVersion | cut -d. -f1).0" \
  -DCMAKE_PREFIX_PATH="$PREFIX" -DQt6_DIR="$QT6/lib/cmake/Qt6" \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DBOTAN_ROOT_DIR="$(brew --prefix botan@2)" \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=OFF -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON \
  -DCMAKE_C_FLAGS="$IFLAGS" -DCMAKE_CXX_FLAGS="$IFLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$LFLAGS" \
  -DCMAKE_SHARED_LINKER_FLAGS="$LFLAGS" \
  -DCMAKE_MODULE_LINKER_FLAGS="$LFLAGS"
cmake --build Build-cmake -j$(sysctl -n hw.ncpu)
```

---

## 5. Windows (MSYS2 — UCRT64 shell)

This section is self-contained: follow it from top to bottom, it repeats the
steps of [§1](#1-get-the-source) where they belong.

### 5.1 Set up MSYS2

1. Install [MSYS2](https://www.msys2.org) (default location `C:\msys64`).
2. From the Start menu, open **"MSYS2 UCRT64"** — not "MSYS2 MSYS" nor
   "MSYS2 MINGW64". Every command in this section runs in that shell.
3. Update the package database and base system. If the shell asks to close,
   reopen "MSYS2 UCRT64" and run the same command again until it reports
   nothing to do:
   ```bash
   pacman -Syu
   ```

### 5.2 Install the dependencies

Install **one** of the two lists below, not both: with Qt5 and Qt6 installed
side by side, the packaging script of [§5.5](#55-package-it) picks Qt6's
`windeployqt` even for a Qt5 build and bundles the wrong Qt. Pick Qt5 if you
want the plugins (see [§2](#2-qt5-or-qt6)). Use the UCRT64 doxygen listed here, not the base MSYS one (it mishandles `D:/`
paths).

**Qt5**:
```bash
pacman -S --needed base-devel git perl ruby \
  mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-doxygen mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-multimedia \
  mingw-w64-ucrt-x86_64-qt5-svg mingw-w64-ucrt-x86_64-qt5-tools \
  mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-libbotan mingw-w64-ucrt-x86_64-sqlcipher \
  mingw-w64-ucrt-x86_64-miniupnpc mingw-w64-ucrt-x86_64-json-c mingw-w64-ucrt-x86_64-rapidjson \
  mingw-w64-ucrt-x86_64-asio mingw-w64-ucrt-x86_64-libxml2 mingw-w64-ucrt-x86_64-libxslt \
  mingw-w64-ucrt-x86_64-speex mingw-w64-ucrt-x86_64-speexdsp mingw-w64-ucrt-x86_64-ffmpeg \
  mingw-w64-ucrt-x86_64-bzip2 mingw-w64-ucrt-x86_64-zlib
```
**Qt6**:
```bash
pacman -S --needed base-devel git perl ruby \
  mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-doxygen mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-multimedia \
  mingw-w64-ucrt-x86_64-qt6-svg mingw-w64-ucrt-x86_64-qt6-5compat mingw-w64-ucrt-x86_64-qt6-tools \
  mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-libbotan mingw-w64-ucrt-x86_64-sqlcipher \
  mingw-w64-ucrt-x86_64-miniupnpc mingw-w64-ucrt-x86_64-json-c mingw-w64-ucrt-x86_64-rapidjson \
  mingw-w64-ucrt-x86_64-asio mingw-w64-ucrt-x86_64-libxml2 mingw-w64-ucrt-x86_64-libxslt \
  mingw-w64-ucrt-x86_64-speex mingw-w64-ucrt-x86_64-speexdsp mingw-w64-ucrt-x86_64-ffmpeg \
  mingw-w64-ucrt-x86_64-bzip2 mingw-w64-ucrt-x86_64-zlib
```

Optional, for a compressed `.7z` archive in [§5.5](#55-package-it). Without it
the script falls back to `zip` if installed, and otherwise leaves just the
folder:
```bash
pacman -S --needed p7zip
```

### 5.3 Get the source

The clone lands in your MSYS2 home directory, i.e.
`C:\msys64\home\<your user name>\RetroShare` in the Windows Explorer.

```bash
cd ~
git clone https://github.com/RetroShare/RetroShare.git
cd RetroShare
git submodule update --init libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/
git submodule update --init --recursive supportlibs/librnp
git submodule update --init supportlibs/restbed supportlibs/libsam3 \
                              supportlibs/udp-discovery-cpp supportlibs/rapidjson
```

Then apply the rapidjson fix of [§1](#fix-rapidjson-all-platforms) — MSYS2
ships GCC ≥ 13, so it is required here:
```bash
git -C supportlibs/rapidjson fetch origin
git -C supportlibs/rapidjson checkout 9bd618f5
```

### 5.4 Build

Notes on the commands below:
- libretroshare is built as a **shared** library (`libretroshare.dll`) on Windows.
- **`RS_USE_NATIVE_DIALOGS=ON`** — Qt's own (non-native) file/directory dialogs
  hang on Windows; the OS native dialogs avoid that.
- The `-include` / `-Wno-template-body` flags are required with GCC ≥ 16 — which
  is what MSYS2 ships today — and harmless on older GCC.
- MinGW compilation is RAM-hungry (~1.5 GB per job for RNP/rapidjson). On a
  low-memory machine replace `-j$(nproc)` with e.g. `-j4`.
- The first configure downloads cmark and restbed: it needs network access.

Build — **Qt5**:
```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DRS_LIBRETROSHARE_STATIC=OFF -DRS_LIBRETROSHARE_SHARED=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DRS_USE_NATIVE_DIALOGS=ON \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON \
  -DCMAKE_C_FLAGS="-include string.h -include stdint.h" \
  -DCMAKE_CXX_FLAGS="-include cstring -include cstdint -Wno-template-body"
cmake --build Build-cmake -j$(nproc)
```

Build — **Qt6**:
```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DRS_LIBRETROSHARE_STATIC=OFF -DRS_LIBRETROSHARE_SHARED=ON \
  -DRS_USE_NATIVE_DIALOGS=ON \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=OFF -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON \
  -DCMAKE_C_FLAGS="-include string.h -include stdint.h" \
  -DCMAKE_CXX_FLAGS="-include cstring -include cstdint -Wno-template-body"
cmake --build Build-cmake -j$(nproc)
```

### 5.5 Package it

The executables in `Build-cmake/` cannot be started as they are: they need
`libretroshare.dll`, the Qt DLLs and the compiler runtime next to them. The
deployment script gathers everything (plugins included, under
`Data\extensions6`) into a standalone portable folder and compresses it:

```bash
./build_scripts/Windows-msys2/deploy-windows.sh
```

The folder is created inside `Build-cmake/`, named `RetroShare-<version>-…`; the
script prints its exact path, and the archive's, on its last `SUCCESS` line.
Start `retroshare-gui.exe` from that folder, or copy the folder or the archive to
any Windows machine. It runs in **portable mode** (the script drops a `portable`
marker file next to the executable): the profile, keys and settings are stored
in the folder's `Data\` subdirectory, not in `%APPDATA%`.

The folder does **not** contain the WebUI yet: the script looks for the web
files in `retroshare-webui/src`, which does not exist, and prints `WebUI source
folder not found. Skipping WebUI packaging.` The GUI, the service and the
plugins are not affected.

The script reads the build from `Build-cmake/`; set `BUILD_DIR` to use another
directory:

```bash
BUILD_DIR=MyCustomBuildDir ./build_scripts/Windows-msys2/deploy-windows.sh
```

---

## 6. Running what you built

The binaries land in the build directory:

| Program | Linux | macOS | Windows |
|---|---|---|---|
| GUI | `Build-cmake/retroshare-gui/retroshare-gui` | `Build-cmake/retroshare-gui/retroshare-gui.app` | `Build-cmake/retroshare-gui/retroshare-gui.exe` |
| Service (JSON API / WebUI) | `Build-cmake/retroshare-service/retroshare-service` | same path | same path, `.exe` |
| Friend server | `Build-cmake/retroshare-friendserver/retroshare-friendserver` | same path | same path, `.exe` |
| Plugins | `Build-cmake/plugins/` | same | same |

Start the GUI on Linux:
```bash
./Build-cmake/retroshare-gui/retroshare-gui
```
and on macOS:
```bash
open Build-cmake/retroshare-gui/retroshare-gui.app
```
On Windows, the binaries in `Build-cmake/` do not start on their own: package them
first ([§5.5](#55-package-it)) and start `retroshare-gui.exe` from the folder the
script produces.

**Plugins (Linux / macOS).** RetroShare does not load plugins from the build
directory. Copy them into the per-user extensions folder, then enable them in
*Options ▸ Plugins* and restart:
```bash
mkdir -p ~/.retroshare/extensions6
cp Build-cmake/plugins/*.so ~/.retroshare/extensions6/
```
Plugins must come from the **same build** as the GUI: an old plugin left in
`extensions6/` after a rebuild can crash RetroShare. Re-copy them after each
rebuild.

**WebUI.** `retroshare-service --help` lists the options; `-W` asks for a WebUI
password on the console and enables the WebUI. This requires the
`-DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON` used in every command above — with
`RS_WEBUI=ON` alone the WebUI is built but can never be switched on. Run from the
build tree (Linux / macOS), the service looks for the web files where
`cmake --install` would put them (`/usr/local/share/retroshare/webui/` on
Linux); point it at the generated ones with `-B` instead:
```bash
./Build-cmake/retroshare-service/retroshare-service -W -B retroshare-webui/webui/
```
On Windows,
the portable folder does not ship the WebUI files yet (see [§5.5](#55-package-it)).

---

## 7. Debug builds (Linux)

The commands above build `Release` (`-O3 -DNDEBUG`): fine for daily use, wrong
under a debugger — backtraces land on misattributed lines and locals read
`<optimized out>`. To step through code or investigate a crash with `gdb`, build
`Debug` with explicit flags: `-g3` emits full debug info including macros, `-O0`
disables inlining so line numbers are exact, `-fno-omit-frame-pointer` keeps
stack unwinding reliable.

```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS_DEBUG="-g3 -O0 -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS_DEBUG="-g3 -O0 -fno-omit-frame-pointer" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON
cmake --build Build-cmake -j$(nproc)
```

> For a **Qt6** debug build, drop `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` and set
> `-DRS_PLUGINS=OFF`; the debug flags are identical.

> A Debug build compiles slower and produces a much larger binary — expected.
> `RelWithDebInfo` (`-O2 -g`) is a middle ground, but because it still optimizes
> it gives approximate backtraces; prefer `Debug` when chasing a crash.

Run the GUI under gdb:

```bash
gdb ./Build-cmake/retroshare-gui/retroshare-gui
```

then `run`, reproduce the issue, and `bt` at the crash for a full symbolized stack.

### AddressSanitizer (use-after-free / heap bugs)

For memory-corruption bugs (use-after-free, heap overflow, double free), an ASan
build reports the fault **at the moment of the bad access**, with three stacks —
allocation, free, and the offending access — instead of a bare `SIGSEGV` after the
fact. Install the ASan runtime:

```bash
sudo apt-get install -y libasan8
```

then add `-fsanitize=address` to both the compile and the link flags:

```bash
rm -rf Build-cmake
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS_DEBUG="-g3 -O1 -fno-omit-frame-pointer -fsanitize=address" \
  -DCMAKE_C_FLAGS_DEBUG="-g3 -O1 -fno-omit-frame-pointer -fsanitize=address" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address" \
  -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=address" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DRS_RNPLIB=ON -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON \
  -DRS_GUI=ON -DRS_SERVICE=ON -DRS_FRIENDSERVER=ON -DRS_PLUGINS=ON -DRS_FORUM_DEEP_INDEX=OFF \
  -DRS_USE_I2P_SAM3=ON -DRS_BITDHT=ON -DRS_MINIUPNPC=ON \
  -DRS_BRODCAST_DISCOVERY=ON -DRS_SQLCIPHER=ON
cmake --build Build-cmake -j$(nproc)
```

Run it with leak detection **off** — RetroShare allocates plenty at startup that
would otherwise bury the report:
```bash
ASAN_OPTIONS=abort_on_error=1:detect_leaks=0 ./Build-cmake/retroshare-gui/retroshare-gui
```
`-O1` (not `-O0`) is deliberate: ASan needs light optimization for acceptable
runtime and cleaner reports, and the sanitizer preserves accurate line numbers
regardless.

---

## Background

### Build layout and options

The top-level `CMakeLists.txt` only *aggregates* independent component projects
(libretroshare, GUI, service, friendserver, plugins); each is a standalone CMake
project, buildable on its own. Ninja is used on all three platforms for
consistency.

- `RS_FORUM_DEEP_INDEX=OFF` — the Xapian-based forum index is not used on desktop.
- `RS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON` is what gives `retroshare-service` its
  `-W`/`-B` flags; it is a `cmake_dependent_option` requiring `RS_WEBUI=ON`.
  Without it `enableWebUI` stays false and the WebUI never serves.

The CMake CI (`.github/workflows/{ubuntu,macos,windows}-cmake.yml` for Qt5, the
matching `…-cmake-qt6.yml` for Qt6) builds the same dependency sets and the same
core options as this document. It does not pass the run-time-only options added
here (`RS_SERVICE_TERMINAL_WEBUI_PASSWORD`, `RS_USE_NATIVE_DIALOGS`), nor the
macOS `BOTAN_ROOT_DIR` pin.

### Why Qt6 is probed explicitly

`retroshare-gui/CMakeLists.txt` probes Qt6 first
(`find_package(Qt6 QUIET COMPONENTS Core)`) and falls back to Qt5, rather than
using `find_package(QT NAMES Qt6 Qt5 …)`. On Debian/Ubuntu multiarch (Qt5 and Qt6
both under `/usr/lib/<arch>/cmake`) the `NAMES` order is **not** honoured — CMake
globs the sibling `Qt5*`/`Qt6*` config dirs and stops at whichever
`…Config.cmake` it reaches first, which depends on filesystem order and
routinely lands on Qt5 even with Qt6 listed first. The explicit probe makes the
preference deterministic, and is also what makes
`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` actually force a Qt5 build.

### Version numbering

Versions are **not** hard-coded — CMake derives them at configure time from
`git describe` run in each component's working tree, so what a build reports is
exactly what your local checkout resolves to. Three independent strings are
stamped in:

| String | Source command | CMake file |
|---|---|---|
| `RS_GUI_VERSION` — the GUI's own version (shown in *Help ▸ About*) | `git describe --tags --always --dirty` in the super-project, leading `v` stripped | `retroshare-gui/CMakeLists.txt` |
| libRetroShare version (`RS_MAJOR`/`RS_MINOR`/`RS_MINI` + `RS_EXTRA_VERSION`) | `git describe --tags --long --match 'v*.*.*'` in `libretroshare/`, parsed into `major.minor.mini` + trailing extra | `libretroshare/CMakeLists.txt` |
| `RS_LIB_VERSION_HASH` — engine build hash | `git describe --tags --always --dirty` in `libretroshare/`, leading `v` stripped | `libretroshare/CMakeLists.txt` |

The super-project's `describe` resolves to a `v0.6.7.x` tag; libretroshare's own
tags are `v0.6.7`. So a GUI version reads like `0.6.7.3-1112-gd6047eb5a` and the
engine like `0.6.7-548-g25845f94`, i.e. `<tag>-<commits since tag>-g<short hash>`.

To read the values a build **would** stamp, without building:

```bash
git describe --tags --always --dirty
git -C libretroshare describe --tags --long --match 'v*.*.*'
```

The first line is `RS_GUI_VERSION` (drop the leading `v`); the second is the
libRetroShare version.

Notes:
- **`-dirty`** is appended whenever the working tree has *tracked* modifications
  at configure time. Modified **submodule pointers count**, so the rapidjson fix
  of [§1](#fix-rapidjson-all-platforms) alone makes the GUI version
  `-dirty`. Untracked files never trigger it.
- libretroshare uses `--long`, which **always** prints the `-<N>-g<hash>` suffix —
  even on a tagged commit you get `v0.6.7-0-g…`, never a bare `0.6.7`.
- If git metadata is unavailable (tarball / distro build, or the git call fails),
  the GUI falls back to the default in `retroshare-gui/src/rsguiversion.h`
  (`"version not available"`), and libretroshare must be given the version on the
  CMake command line: `-DRS_MAJOR_VERSION=… -DRS_MINOR_VERSION=… -DRS_MINI_VERSION=…`.
