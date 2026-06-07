# Task: Implement Friend Requests Page (Issue #1794)

All new source files are already in place. Your job is to integrate them
into the existing build system and codebase, then commit and push.

---

## Files already added (do not modify their content)

- `libretroshare/src/retroshare/rsfriendrequest.h`
- `libretroshare/src/rsserver/p3friendrequest.h`
- `libretroshare/src/rsserver/p3friendrequest.cc`
- `retroshare-gui/src/gui/FriendRequests/FriendRequestsPage.h`
- `retroshare-gui/src/gui/FriendRequests/FriendRequestsPage.cpp`

---

## Step 1 — Add CONFIG_TYPE_FRIEND_REQUESTS to p3cfgmgr.h

File: `libretroshare/src/rsserver/p3cfgmgr.h`

Find the enum or set of `#define` constants that list config type IDs
(e.g. `CONFIG_TYPE_PEERS`, `CONFIG_TYPE_GENERAL`, …).
Find the highest existing value, add 1, and insert:

```cpp
#define CONFIG_TYPE_FRIEND_REQUESTS  <next_unused_value>
```

Then remove the fallback `#ifndef` block from `p3friendrequest.h` —
search for `#ifndef CONFIG_TYPE_FRIEND_REQUESTS` and delete those 3 lines.

---

## Step 2 — Register files in the build system

### `libretroshare/src/libretroshare.pro`

Add to the **HEADERS** list. Find the last item in HEADERS that ends
with a backslash continuation `\`, add `\` to that line if missing,
then append on a new line:

```
    retroshare/rsfriendrequest.h \
    rsserver/p3friendrequest.h \
```

Add to the **SOURCES** list in the same way:

```
    rsserver/p3friendrequest.cc \
```

### `retroshare-gui/src/retroshare-gui.pro`

Add to **HEADERS**:

```
    gui/FriendRequests/FriendRequestsPage.h \
```

Add to **SOURCES**:

```
    gui/FriendRequests/FriendRequestsPage.cpp \
```

> Note: every non-final entry in a qmake list must end with `\`.
> Check that the line immediately above your insertion already has `\`.

---

## Step 3 — Hook into AuthSSL (backend trigger)

File: `libretroshare/src/authssl/authssl.cc`

Add this include near the top of the file:

```cpp
#include "retroshare/rsfriendrequest.h"
```

Find the function `AuthSSLimpl::VerifyX509Callback` (or the equivalent
connection-rejection branch). Locate the point where a connection is denied
because the peer's PGP key is NOT in the friend list. Insert:

```cpp
// Notify the friend-request manager so the user can review and accept.
// onUnknownPeerConnectionAttempt is part of the public RsFriendRequest
// interface — no cast is needed.
if (rsFriendRequest)
    rsFriendRequest->onUnknownPeerConnectionAttempt(
            sslId,
            pgpId,
            pgpName,        // CN field parsed from the X.509 certificate
            pgpFingerprint  // hex fingerprint string
    );
```

The exact variable names (`sslId`, `pgpId`, …) may differ in the actual
source. Adapt to whatever names are used in that branch.

---

## Step 4 — Initialise p3FriendRequest in p3Server

File: `libretroshare/src/rsserver/p3server.cc`

Add include near the top:

```cpp
#include "rsserver/p3friendrequest.h"
```

In `p3Server::init()` (or `StartupRsThread()`), after `rsPeers` and
`mPeerMgr` are initialised, add:

```cpp
auto* frMgr = new p3FriendRequest(mPeerMgr);
rsFriendRequest = frMgr;
mConfigMgr->addConfiguration("friend_requests.cfg", frMgr);
```

---

## Step 5 — Add the page to MainWindow

### `retroshare-gui/src/gui/MainWindow.h`

Add include:

```cpp
#include "FriendRequests/FriendRequestsPage.h"
```

Add to **private members**:

```cpp
FriendRequestsPage* mFriendRequestsPage   = nullptr;
QAction*            mFriendRequestsAction = nullptr;  // returned by addPage()
```

Add to **private slots**:

```cpp
void updateFriendRequestBadge(int count);
```

### `retroshare-gui/src/gui/MainWindow.cpp`

In the constructor, after the other pages are added:

```cpp
mFriendRequestsPage = new FriendRequestsPage(this);
connect(mFriendRequestsPage, &FriendRequestsPage::pendingCountChanged,
        this,                &MainWindow::updateFriendRequestBadge);

// addPage() returns a QAction* for the navigation entry.
mFriendRequestsAction = addPage(
        mFriendRequestsPage,
        QIcon(":/images/friend_request.png"),
        tr("Friend Requests"));
```

Implement the slot:

```cpp
void MainWindow::updateFriendRequestBadge(int count)
{
    if (!mFriendRequestsAction) return;
    QString label = tr("Friend Requests");
    if (count > 0)
        label += QString(" (%1)").arg(count);
    mFriendRequestsAction->setText(label);
}
```

---

## Step 6 — Add a placeholder icon

Check whether `retroshare-gui/src/gui/images/friend_request.png` exists.
If not, copy any existing 24×24 icon as a placeholder:

```bash
cp retroshare-gui/src/gui/images/peers.png \
   retroshare-gui/src/gui/images/friend_request.png
```

Then register it in `retroshare-gui/src/gui/images/images.qrc`:

```xml
<file>images/friend_request.png</file>
```

---

## Step 7 — Build and fix errors

**Always run qmake before make** whenever .pro files change:

```bash
cd build   # or your configured build directory
qmake ..
make -j$(nproc) 2>&1 | tee /tmp/rs_build.log | head -80
```

If there are compilation errors, fix them by adapting include paths,
function signatures, or variable names to match the actual RS source.
Do **not** change the logic of the new files — only fix integration mismatches.

If the build succeeds but you see linker errors about missing symbols,
check that all SOURCES entries in both .pro files were saved correctly.

---

## Step 8 — Commit and push

```bash
git checkout -b feature/friend-requests-page-1794

git add libretroshare/src/retroshare/rsfriendrequest.h
git add libretroshare/src/rsserver/p3friendrequest.h
git add libretroshare/src/rsserver/p3friendrequest.cc
git add retroshare-gui/src/gui/FriendRequests/FriendRequestsPage.h
git add retroshare-gui/src/gui/FriendRequests/FriendRequestsPage.cpp
git add libretroshare/src/libretroshare.pro
git add retroshare-gui/src/retroshare-gui.pro
git add libretroshare/src/rsserver/p3cfgmgr.h
git add retroshare-gui/src/gui/MainWindow.h
git add retroshare-gui/src/gui/MainWindow.cpp
git add retroshare-gui/src/gui/images/friend_request.png
git add retroshare-gui/src/gui/images/images.qrc

git commit -m "gui: add Friend Requests page (closes #1794)

- Add RsFriendRequest public interface with onUnknownPeerConnectionAttempt
- Implement p3FriendRequest with p3Config persistence (friend_requests.cfg)
- Accept via rsPeers->addSslOnlyFriend(); reject is local-only (no network msg)
- Qt Model/View page: search, badge count, reject/clear-rejected support
- Hook into AuthSSL unknown-peer connection callback"

git push origin feature/friend-requests-page-1794
```

---

## Success criteria

- Project compiles without errors or warnings related to new files
- New branch `feature/friend-requests-page-1794` is pushed to origin
- Commit message references `#1794`
