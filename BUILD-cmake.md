# RetroShare — CMake build (Linux / macOS / Windows)

Build commands for the whole tree from the root of the super-project. The
top-level `CMakeLists.txt` only *aggregates* the independent component projects
(libretroshare + GUI + service + friendserver + plugins) — each is a standalone
CMake project, buildable on its own. JSON API / WebUI enabled.

## Qt5 or Qt6

The build is **version-agnostic**: `retroshare-gui/CMakeLists.txt` probes Qt6
first (`find_package(Qt6 QUIET COMPONENTS Core)`) and falls back to Qt5, so the
same tree builds against **either** Qt5 or Qt6. When both are installed **Qt6 is
preferred**; force Qt5 with `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`.

> Why an explicit probe and not `find_package(QT NAMES Qt6 Qt5 …)`: on
> Debian/Ubuntu multiarch (Qt5 and Qt6 both under `/usr/lib/<arch>/cmake`) the
> `NAMES` order is **not** honoured — CMake globs the sibling `Qt5*`/`Qt6*`
> config dirs and stops at whichever `…Config.cmake` it reaches first, which is
> filesystem-order dependent and routinely lands on Qt5 even with Qt6 listed
> first. The explicit Qt6 probe makes the preference deterministic, and is also
> what makes `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` actually force a Qt5 build
> (with the versionless `QT` package name that switch had no effect).

Two deltas for a Qt6 build (everything else is identical):
- different Qt `-dev` packages (see each platform below);
- **`-DRS_PLUGINS=OFF`** — the VOIP plugin still uses the Qt5-era QtMultimedia API
  (`QAudioInput`/`QAudioOutput`/`QCameraInfo`) and isn't ported to Qt6 yet;
  FeedReader's CMake is also still Qt5-only.

Each configure below is prefixed with `rm -rf Build-cmake`, so the build dir is
recreated from scratch every time. This keeps switching between Qt5 and Qt6 in the
same `Build-cmake` clean (no stale `CMakeCache.txt` pinning the previous Qt
version). Drop the `rm -rf` if you want an incremental rebuild instead.

Validated by the CMake CI:
`.github/workflows/{ubuntu,macos,windows}-cmake.yml` (Qt5) and the matching
`…-cmake-qt6.yml` (Qt6).

Other notes:
- `RS_FORUM_DEEP_INDEX=OFF` on purpose (Xapian unused; re-enabled later via the
  ForumContentSearchFTS5 branch).
- Ninja is used on all three platforms for consistency.
- `RS_SERVICE_TERMINAL_WEBUI_PASSWORD=ON` is **required** for `retroshare-service`
  to expose the `-W`/`-B` flags that enable and serve the WebUI. `RS_WEBUI=ON`
  alone builds the WebUI machinery, but the service then has **no way to turn it
  on** — `enableWebUI` defaults to false and only `-W` ever sets it, so the WebUI
  never serves. It is a `cmake_dependent_option` requiring `RS_WEBUI=ON`.

## Submodules (init WITHOUT --remote)

`--remote` would move libretroshare off its pinned CMake commit and break the
build. librnp needs `--recursive` for its nested `src/libsexpp`.

```bash
git submodule update --init libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/
git submodule update --init --recursive supportlibs/librnp
git submodule update --init supportlibs/restbed supportlibs/libsam3 \
                              supportlibs/udp-discovery-cpp supportlibs/rapidjson
```

**rapidjson / GCC >= 13:** the commit pinned by the super-project (`f54b0e47`)
has a broken `GenericStringRef::operator=` that assigns to the `const` member
`length`, so it fails to compile on modern toolchains (e.g. Debian 13 / GCC 14)
with *"assignment of read-only member 'length'"*. Check out the upstream fix
commit (#719, on Tencent/rapidjson master):

```bash
git -C supportlibs/rapidjson fetch origin
git -C supportlibs/rapidjson checkout 9bd618f5
```

## Version numbering

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

To read the exact values a build **would** stamp, without building, run from the
super-project root:

```bash
git describe --tags --always --dirty
git -C libretroshare describe --tags --long --match 'v*.*.*'
```

The first line is `RS_GUI_VERSION` (drop the leading `v`); the second is the
libRetroShare version.

Notes:
- **`-dirty`** is appended whenever the working tree has *tracked* modifications
  at configure time. Modified **submodule pointers count as tracked changes**, so
  a tree that is otherwise clean apart from submodule drift still stamps `-dirty`
  on the GUI/engine version. Untracked files never trigger it.
- libretroshare uses `--long`, which **always** prints the `-<N>-g<hash>` suffix —
  even on a tagged commit you get `v0.6.7-0-g…`, never a bare `0.6.7`.
- Merging feature branches into an integration branch (e.g. `combo`) only grows
  the `-<N>-` commit count and changes the `-g<hash>`; the `v0.6.7.x` / `v0.6.7`
  tag prefix stays put. An octopus merge (one merge commit) yields a slightly
  lower `<N>` than the same branches merged one by one (one merge commit each).
- If git metadata is unavailable (tarball / distro build, or the git call fails),
  the GUI falls back to the default in `retroshare-gui/src/rsguiversion.h`
  (`"version not available"`), and libretroshare must be given the version on the
  CMake command line: `-DRS_MAJOR_VERSION=… -DRS_MINOR_VERSION=… -DRS_MINI_VERSION=…`.

---

## Linux (Ubuntu/Debian)

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
  libxml2-dev libxslt1-dev libcurl4-openssl-dev \
  doxygen python3
```

Dependencies — **Qt6** (self-contained; only the `qt*` packages differ from the
Qt5 list above):
```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential g++ cmake ninja-build pkg-config git \
  qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev \
  qt6-5compat-dev qt6-tools-dev qt6-tools-dev-tools \
  libssl-dev zlib1g-dev libbz2-dev libbotan-2-dev libjson-c-dev \
  libasio-dev libsqlcipher-dev \
  libspeex-dev libspeexdsp-dev libminiupnpc-dev \
  libavcodec-dev libavutil-dev \
  libxml2-dev libxslt1-dev libcurl4-openssl-dev \
  doxygen python3
```
> On Ubuntu 24.04 the Core5Compat dev headers come from `qt6-5compat-dev` (there
> is no `libqt6core5compat6-dev`). Svg/X11Extras dev packages aren't needed on
> Linux for Qt6.

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

Build — **Qt6** (auto-selected once the qt6 `-dev` packages are installed):
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

### Debug build (Linux)

The commands above build `Release` (`-O3 -DNDEBUG`): aggressive inlining, assertions
stripped. Fine for daily use, wrong under a debugger — backtraces land on
misattributed lines and locals read `<optimized out>`. To step through code or
investigate a crash with `gdb`, build `Debug` instead.

Two changes to the Qt5 configure above:
- swap `-DCMAKE_BUILD_TYPE=Release` for `-DCMAKE_BUILD_TYPE=Debug` (gives `-g -O0`);
- pin explicit debug flags so every symbol is emitted and nothing is inlined:
  `-DCMAKE_CXX_FLAGS_DEBUG` / `-DCMAKE_C_FLAGS_DEBUG` = `-g3 -O0 -fno-omit-frame-pointer`.

`-g3` emits full debug info **including macro definitions**; `-O0` disables inlining so
backtrace line numbers are exact; `-fno-omit-frame-pointer` keeps stack unwinding
reliable.

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
> `-DRS_PLUGINS=OFF` (as in the Release Qt6 command); the debug flags are identical.

> A Debug build compiles slower and produces a much larger binary — expected.
> `RelWithDebInfo` (`-O2 -g`) is a middle ground with usable symbols, but because it
> still optimizes it gives approximate backtraces; prefer `Debug` when chasing a crash.

Run the GUI under gdb (the binary lands at `Build-cmake/retroshare-gui/retroshare`):

```bash
gdb ./Build-cmake/retroshare-gui/retroshare
```

then `run`, reproduce the issue, and `bt` at the crash for a full symbolized stack.

#### AddressSanitizer (use-after-free / heap bugs)

For memory-corruption bugs (use-after-free, heap overflow, double free), an ASan build
reports the fault **at the moment of the bad access**, with three stacks — allocation,
free, and the offending access — instead of a bare `SIGSEGV` after the fact. Add
`-fsanitize=address` to both the compile and the link flags:

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

> Needs the ASan runtime: `sudo apt-get install -y libasan8`. Run with leak detection
> **off** — RetroShare allocates plenty at startup that would otherwise bury the report:
> ```bash
> ASAN_OPTIONS=abort_on_error=1:detect_leaks=0 ./Build-cmake/retroshare-gui/retroshare
> ```
> `-O1` (not `-O0`) is deliberate: ASan needs light optimization for acceptable runtime
> and cleaner reports, and the sanitizer preserves accurate line numbers regardless.

---

## macOS (Homebrew, Apple Silicon)

Dependencies — **Qt5**:
```bash
brew install ninja qt@5 openssl@3 botan@2 sqlcipher miniupnpc \
  json-c asio speex speexdsp ffmpeg libxml2 libxslt bzip2 zlib rapidjson doxygen
```
Dependencies — **Qt6** (self-contained; only the Qt package differs from the Qt5
list above). Unlike Linux's split `qt6-*` packages, Homebrew ships Qt6 as a
**single monolithic `qt` formula** that already bundles qt5compat / svg /
multimedia / tools — so installing `qt` is all you need:
```bash
brew install ninja qt openssl@3 botan@2 sqlcipher miniupnpc \
  json-c asio speex speexdsp ffmpeg libxml2 libxslt bzip2 zlib rapidjson doxygen
```
> `qt5compat` (bundled in `qt`) provides `Qt6::Core5Compat`, required by the GUI
> (QRegExp / QTextCodec).
>
> **If `brew install qt` fails with** `Cannot install qtbase because conflicting
> formulae are installed … qt@5: because both link conflicting binaries`, your
> qt@5 is (force-)linked. Run `brew unlink qt@5` first, then re-run
> `brew install qt`:
> ```bash
> brew unlink qt@5
> brew install qt
> ```
> Both Qt versions then coexist: the Qt6 build finds Qt6 via `Qt6_DIR`, and the
> Qt5 build still finds qt@5 via `-DQt5_DIR="$(brew --prefix qt@5)/lib/cmake/Qt5"`
> (the keg path works whether or not qt@5 is linked).

> **Botan:** with several Botan versions installed (`botan`=v3, `botan@2`,
> `botan@3`), a fresh configure can mismatch — Botan 3 headers + Botan 2 lib →
> `Undefined symbols … Botan::…` linker errors in librnp. Both macOS commands
> below pin it with `-DBOTAN_ROOT_DIR="$(brew --prefix botan@2)"` so headers and
> lib come from the **same** keg ([FindBotan.cmake] searches `BOTAN_ROOT_DIR`
> only, via `NO_DEFAULT_PATH`). To build against Botan 3 instead, point it at
> `$(brew --prefix botan)` — RNP then detects v3 and links `libbotan-3`.

> The harvest loop in both macOS commands skips every `qt*` keg on purpose: Qt is
> provided via `Qt6_DIR` / `Qt5_DIR`, and pulling a second Qt's headers off
> `/opt/homebrew/opt/qt*` would mix Qt5/Qt6 and break the build.

Build — **Qt5** (harvest Homebrew include/lib dirs; point find_package(Qt5) at keg-only qt@5):
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

Build — **Qt6** (use the `qt` prefix + `Qt6_DIR`; plugins OFF):
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

## Windows (MSYS2 — UCRT64 shell)

> The build commands use `-j$(nproc)` (all cores; `nproc` is available in MSYS2).
> MinGW C++ compilation is RAM-hungry (RNP/rapidjson can use ~1.5 GB per job), so
> on a low-memory machine lower it manually, e.g. `-j4` — that's why it used to be
> hard-capped at `-j3`.

> **`RS_USE_NATIVE_DIALOGS=ON`** — Qt's own (non-native) file/directory dialogs
> hang on Windows. Using the OS native dialogs (`RS_USE_NATIVE_DIALOGS=ON`)
> avoids the problem. On Linux/macOS the default `OFF` works fine.

Dependencies — common + **Qt5** (use the UCRT64 doxygen, not the base MSYS one —
it mishandles `D:/` paths):
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
Dependencies — **Qt6** (self-contained; only the `qt*` packages differ from the
Qt5 list above):
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

Build — **Qt5** (the `-include` / `-Wno-template-body` flags are only needed for
GCC >= 16 — RNP/rapidjson; harmless on older GCC):
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

Build — **Qt6** (auto-selected once the qt6 packages are installed; plugins OFF):
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
