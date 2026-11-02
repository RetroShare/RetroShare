# Building RetroShare AppImages (Linux)

Two ways to produce a RetroShare GUI AppImage. Both **package the CMake build**
(never qmake) with `linuxdeploy` — they differ only in *where* the binary is
compiled, which fixes the glibc floor of the result.

| Script | Compiles? | Needs a local CMake build first? | Result runs on |
|---|---|---|---|
| `build-appimage-local.sh` | no — packages `Build-cmake/retroshare-gui/retroshare-gui` | **yes** | only distros with glibc ≥ the build host (≈2.39 on Ubuntu 24.04) |
| `build-appimage-docker.sh` | yes — inside a Debian 11 container | **no** | any distro with **glibc ≥ 2.31** (MX 21, Slackware 15, Debian 11+, Ubuntu 20.04+, Fedora 34+…) |

**Rule of thumb:** use `docker` for anything you hand to testers/users; use
`local` only for a quick check on a recent machine.

> Whatever you run, **the source tree must already contain the code you want to
> ship.** These scripts don't change code: `local` repackages whatever is in
> `Build-cmake/`, `docker` recompiles the mounted source. After switching
> branches (e.g. to `combo` for the GXS signature-verification fixes) you must
> rebuild.

---

## 1. Local build — `build-appimage-local.sh`

Fast, but the AppImage inherits **this machine's glibc**, so it only runs on
equally-recent or newer distros.

```bash
# 1. compile with CMake first (see ../../BUILD-cmake.md) — Qt5, GUI:
cmake -G Ninja -B Build-cmake -S . \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DRS_GUI=ON -DRS_SERVICE=OFF -DRS_FRIENDSERVER=OFF -DRS_PLUGINS=OFF \
  -DRS_JSON_API=ON -DRS_WEBUI=ON -DRS_FORUM_DEEP_INDEX=OFF
cmake --build Build-cmake -j$(nproc)

# 2. package it:
build_scripts/AppImage/build-appimage-local.sh
```

Output: `Build-appimage/RetroShare-<version>-x86_64.AppImage`.

It downloads `linuxdeploy` + `linuxdeploy-plugin-qt` into
`build_scripts/AppImage/tools/` (once), lays out a clean `AppDir`, and bundles
Qt + libs.

**Qt5 or Qt6:** the script reads the Qt major from the compiled binary and picks
the matching `qmake` automatically (`qmake6` for a Qt6 build, `qmake`/`qmake-qt5`
for Qt5), and aborts early on a qmake/binary Qt mismatch. So to ship a Qt6
AppImage, just compile the GUI against Qt6 — no script change. Override the
choice with `QMAKE=/path/to/qmake` if needed.

---

## 2. Portable build — `build-appimage-docker.sh`

Compiles inside a **Debian 11 (glibc 2.31)** container, so the AppImage runs
almost everywhere. No prior local build needed — it recompiles the mounted
source itself into `Build-cmake-bullseye/` (separate from your `Build-cmake/`).

> **Prerequisite (glibc 2.31 build only):** the libretroshare you compile must
> carry the `RS_NO_LTO` opt-out, i.e. **libretroshare PR #316 must be merged**
> (or its branch checked out in the `libretroshare` submodule). Debian 11's
> GCC 10 crashes while linking otherwise — see [**LTO**](#lto) below. The
> `local` build does not need it.

```bash
build_scripts/AppImage/build-appimage-docker.sh
```

Prerequisite: **Docker** (or Podman). One-time setup on Ubuntu:

```bash
sudo apt install -y docker.io
sudo usermod -aG docker $USER
```

Then **log out and back in once** (every new terminal then has Docker + your
normal group). `newgrp docker` is only a "activate it in this terminal without
logging out" shortcut; its side effect is that files you create in that shell
get group `docker` — harmless, but avoid it by just re-logging.

### From scratch vs incremental

Incremental by default (reuses `Build-cmake-bullseye/`, fast). To recompile
everything from zero:

```bash
CLEAN=1 build_scripts/AppImage/build-appimage-docker.sh
```

The output `AppDir` and `.AppImage` are rebuilt every run regardless. The Docker
**image** is never touched by `CLEAN` — rebuild it from scratch only when you
change dependencies, with `docker build --no-cache -f Dockerfile.bullseye .`.

---

## Files in this directory

| File | Role |
|---|---|
| `build-appimage-local.sh` | package the local `Build-cmake` binary (host glibc) |
| `build-appimage-docker.sh` | host-side driver: build image, run the container build (glibc 2.31) |
| `Dockerfile.bullseye` | Debian 11 toolchain + deps + AppImage tools |
| `_appimage-inside.sh` | runs **inside** the container: cmake → AppDir → linuxdeploy |
| `tools/` | cached `linuxdeploy` + `linuxdeploy-plugin-qt` |
| `TESTERS.txt` | ship this **with** the AppImage — how testers run it |
| `Recipe`, `retroshare.yml`, `makeAppImage*.sh` | **obsolete** (old probonopd Qt4/trusty recipe) — ignore |

---

## How the packaging works

Both scripts build the same clean FHS `AppDir` by hand (the project's own
`install()` rules aren't AppImage-friendly — `RS_SERVICE_DESKTOP` is OFF by
default and would put the `.desktop` under `prefix/data/`):

```
AppDir/usr/bin/retroshare-gui                    <- the CMake binary
AppDir/usr/share/applications/retroshare.desktop <- Icon=retroshare, Exec=retroshare-gui
AppDir/usr/share/icons/hicolor/{24,48,64,128,scalable}/apps/retroshare.*
```

then `linuxdeploy --plugin qt --output appimage` bundles Qt + all non-system libs.

**We use `linuxdeploy`, not `linuxdeployqt`.** The probonopd `linuxdeployqt`
"continuous" build 107 fails silently right after the icon stage on this
toolchain. `linuxdeploy` + `linuxdeploy-plugin-qt` work and give clear errors.
`APPIMAGE_EXTRACT_AND_RUN=1` is set because containers/Ubuntu 24.04 lack libfuse2.

---

## Adding a missing build dependency

If the container build fails on a missing header / `pkg_check_modules … not
found`, add the Debian package to **`Dockerfile.bullseye`** — in the **small
trailing `RUN apt-get install` layer**, *not* the big apt line. That keeps the
big toolchain layer a cache hit so only the new package installs (editing the
big line reinstalls the whole toolchain).

Known one already applied: `libxss-dev` (provides `xscrnsaver.pc`, needed by the
GUI's idle detection at `retroshare-gui/CMakeLists.txt`).

---

## LTO

The container build passes `-DRS_NO_LTO=ON`. Debian 11's GCC 10 has an LTO
plugin that segfaults (`lto1: internal compiler error: Segmentation fault`, IPA
`icf` pass) when linking libretroshare, so LTO **must** be off for the glibc
2.31 build to link at all.

`RS_NO_LTO` is an opt-out in `libretroshare/CMakeLists.txt` (default builds keep
LTO on). It is **not in libretroshare master yet** — it comes from libretroshare
**PR #316** (`CMakeLtoOptOut`). The glibc 2.31 build therefore only works once
that option is present in the libretroshare you compile:

- **ideally:** PR #316 is merged into libretroshare master — a plain
  `git submodule update --init` then picks it up, nothing else to do;
- **until then:** check out the #316 branch inside the `libretroshare` submodule
  before running the Docker build.

Without it, CMake silently ignores `-DRS_NO_LTO=ON` (`unused variable` warning),
LTO stays on, and the container link dies with the `lto1` ICE.

The local build (recent host GCC) is unaffected — LTO links fine there, so the
flag is a harmless no-op. `_appimage-inside.sh` passes it automatically.

---

## Versioning / filename gotcha

The version in the filename comes from `git describe` of the **super-project**
(retroshare-gui), **not** libretroshare. So a build with a fixed/different
libretroshare (e.g. `combo`) can carry the **same filename** as an older buggy
one. Rename the output to mark the real variant, e.g.:

```bash
mv Build-appimage/RetroShare-v0.6.7.3-1081-g8943c7306-x86_64.AppImage \
   Build-appimage/RetroShare-combo-gxsfix-x86_64-glibc_2.31.AppImage
```

---

## glibc portability (why the Docker build exists)

A binary requires the **highest `GLIBC_x.y` symbol** it was linked against, and
an AppImage **never bundles glibc**. Built on Ubuntu 24.04 the binary needs
`GLIBC_2.38` → fails on Debian 12 (2.36), Slackware 15 (2.33), MX 21 (2.31).
Built on Debian 11 it needs only 2.31 → runs on ~everything since 2020.

| Distro | glibc | local (2.39) | docker (2.31) |
|---|---|---|---|
| Ubuntu 24.04 | 2.39 | ✅ | ✅ |
| Debian 13 / Fedora 40 | 2.41 / 2.39 | ✅ | ✅ |
| Debian 12 | 2.36 | ❌ | ✅ |
| Slackware 15 | 2.33 | ❌ | ✅ |
| MX 21 / Debian 11 / Ubuntu 20.04 | 2.31 | ❌ | ✅ |
| Ubuntu 18.04 / Debian 10 / RHEL 8 | ≤2.28 | ❌ | ❌ |

Check any machine with `ldd --version`.

---

## Troubleshooting

**Docker build "finishes in ~1 second" / exit code 141** — a diagnostic line
`ldd --version | head -1` was aborting the script via SIGPIPE under
`set -o pipefail` before cmake ran. Fixed (`_appimage-inside.sh` now folds the
toolchain banner into a single `echo`). If a build ever exits fast again, check
the very last command before it stopped.

**Build didn't actually recompile** — check timestamps:
`ls -la Build-cmake-bullseye/retroshare-gui/retroshare-gui`. If it's old, the
compile was skipped/aborted. `CLEAN=1` forces a full rebuild.

**"Text file busy" when launching** — the AppImage sits inside a folder a
running RetroShare is *sharing*; RS holds the file open. Run it from a normal
folder (see TESTERS.txt).

**"libfuse.so.2" missing on the target** — `sudo apt install libfuse2`, or run
with `APPIMAGE_EXTRACT_AND_RUN=1 ./RetroShare-*.AppImage`.

**"GLIBC_2.xx not found" on the target** — the target's glibc is older than the
build host's. Use the Docker (2.31) build, or an even older base if needed.

---

## For testers

Ship `TESTERS.txt` alongside the `.AppImage`. It covers: glibc requirement, how
to run, the FUSE fallback, `~/.retroshare` behavior (existing profile is reused;
`HOME=/tmp/rs-test ./…` to isolate), and the shared-folder caveat.
