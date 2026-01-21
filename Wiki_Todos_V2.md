# Wiki Todos V2 - Issue #3107

**Issue Link:** https://github.com/RetroShare/RetroShare/issues/3107

## Todos:

### 1. Remove RsGxsUpdateBroadcastPage & old TokenQueue method
Replace by loading data with `RsThread::async`
- **Example:** [GxsChannelPostFilesModel.cpp](https://github.com/RetroShare/RetroShare/blob/master/retroshare-gui/src/gui/gxschannels/GxsChannelPostFilesModel.cpp)

### 2. Add rsEvents for RsWiki
Add if required more rsEvents for RsWiki:
```
NEW_SNAPSHOT
NEW_COLLECTION 
SUBSCRIBE_STATUS_CHANGED
NEW_COMMENT
```

### 3. Implement Republish & Merging for WikiEdits
Not done yet, it shows only first edited ones, for Admins/Mods

### 4. Add UserNotify
When you subscribed a Wiki it has new updates or pushes
- Count on MainWindow/Wiki Button (when you viewed the Wiki Page or Wiki Group clear the counts)
- **Example code:** 
  - [GxsChannelUserNotify.cpp](https://github.com/RetroShare/RetroShare/blob/master/retroshare-gui/src/gui/gxschannels/GxsChannelUserNotify.cpp)
  - [GxsChannelUserNotify.h](https://github.com/RetroShare/RetroShare/blob/master/retroshare-gui/src/gui/gxschannels/GxsChannelUserNotify.h)

### 5. Add Search Filter
Add Search Filter for the Page (at the moment not functional)

### 6. Add Moderators Feature
Like how on forums

### 7. Implement Commenting Feature
Implement the commenting feature & a CommentsViewer for the comments of a Page (maybe as new tab on the main page of a Page)

### 8. Implement Required Features
Implement the features which we need for wiki for lib & gui part:

```cpp
/* Specific Service Data */
virtual bool getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections) = 0;
virtual bool getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots) = 0;
virtual bool getComments(const uint32_t &token, std::vector<RsWikiComment> &comments) = 0;

virtual bool getRelatedSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots) = 0;

virtual bool submitCollection(uint32_t &token, RsWikiCollection &collection) = 0;
virtual bool submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot) = 0;
virtual bool submitComment(uint32_t &token, RsWikiComment &comment) = 0;

virtual bool updateCollection(uint32_t &token, RsWikiCollection &collection) = 0;

// Blocking Interfaces.
virtual bool createCollection(RsWikiCollection &collection) = 0;
virtual bool updateCollection(const RsWikiCollection &collection) = 0;
virtual bool getCollections(const std::list<RsGxsGroupId> groupIds, std::vector<RsWikiCollection> &groups) = 0;
```

---

## Basic Functionality of RS Wiki

- **New Group:** Creates a collection of Wiki Pages. The creator of group and anyone they share the publish keys with is the moderator
- **New Page:** Create a new Wiki Page, only Group Moderators can do this
- **Edit:** Anyone can edit the Wiki Page, and the changes form a tree under the original
- **RePublish:** This is used by the Moderators to accept and reject Edits. The republished page becomes a new version of the Root Page, allowing more edits to be done

---

## Ideas

- **Formatting:** No HTML, lets use Markdown or something similar
- **Diffs:** Lots of edits will lead to complex merges - a robust merge tool is essential
- **Read Mode:** Hide all the Edits, and only show the most recently published versions
- **Easy Duplication:** To take over an Abandoned or badly moderated Wiki. Copies all base versions to a new group
- **WikiLinks:** A generic Wiki Cross Linking system. This should be combined with Easy Duplication option to allow easy replacement of groups if necessary. A good design here is critical to a successful Wiki ecosystem
- **Media:** Work out how to include media (photos, audio, video, etc) without embedding in pages. This would leverage the turtle transfer system somehow - maybe like channels
- **Comments:** Reviews etc can be incorporated - ideas here are welcome

---

## Source Code Paths

### libretroshare/service:
- [p3wiki.cc](https://github.com/RetroShare/libretroshare/blob/96e249a06d8f30c2aace38beecc8fb7271159a88/src/services/p3wiki.cc)
- [p3wiki.h](https://github.com/RetroShare/libretroshare/blob/96e249a06d8f30c2aace38beecc8fb7271159a88/src/services/p3wiki.h)

### rsitems:
- [rswikiitems.cc](https://github.com/RetroShare/libretroshare/blob/7643654403b5779e56dd20c5e73e4e47583f27e6/src/rsitems/rswikiitems.cc)
- [rswikiitems.h](https://github.com/RetroShare/libretroshare/blob/7643654403b5779e56dd20c5e73e4e47583f27e6/src/rsitems/rswikiitems.h)

### Interface:
- [rswiki.h](https://github.com/RetroShare/libretroshare/blob/master/src/retroshare/rswiki.h)

### GUI Part:
- [WikiPoos Directory](https://github.com/RetroShare/RetroShare/tree/master/retroshare-gui/src/gui/WikiPoos)

Files:
```
WikiAddDialog.cpp
WikiAddDialog.h
WikiAddDialog.ui
WikiDialog.cpp
WikiDialog.h
WikiDialog.ui
WikiEditDialog.cpp
WikiEditDialog.h
WikiEditDialog.ui
gui/gxs/WikiGroupDialog.cpp
gui/gxs/WikiGroupDialog.h
```

---

## Enable RS Wiki

Add to retroshare.pri:
```
CONFIG *= wikipoos
```

Then:
```bash
cd retroshare
qmake
make
```

---

## How to Compile

### Linux:
[Linux_InstallGuide.md](https://github.com/RetroShare/RetroShare/blob/master/build_scripts/Debian+Ubuntu/Linux_InstallGuide.md)

### Windows:
[WindowsMSys2_InstallGuide.md](https://github.com/RetroShare/RetroShare/blob/master/build_scripts/Windows-msys2/WindowsMSys2_InstallGuide.md)

### MacOS:
[MacOS_X_InstallGuide.md](https://github.com/RetroShare/RetroShare/blob/master/build_scripts/OSX/MacOS_X_InstallGuide.md)

---

## UI Editing

To Edit or Create UI files you can use Qt Creator or Qt Designer

---

## Key Discussion Points (from comments)

### Authentication/Moderation Architecture

The wiki authentication flags are currently the same as channels:
- Group modifications (admin level) and new messages require the publish key
- Child messages (e.g. edits or comments) don't require publish key
- The moderator list cannot be different from the list of people who can post root messages

**Proposed Improvement (Forums-style moderation):**
- Admin edits the group data to allow/remove moderators
- Moderators can edit any existing message (not only their own)
- When removing a moderator, it is kept on the list with a termination date
- Edits that do not respect the moderation rule are discarded network-wide

**Mitigation of Wiki Data Corruption:**
- Include in the group data (admin-controlled) a publish list that only allows some users to publish
- Force a contact author for the wiki (flag `GRP_OPTION_AUTHEN_AUTHOR_SIGN`)
- Let admins add/remove editors easily

---

## Implementation Status (as of latest commits)

### ✅ Completed (7 of 8 todos):
1. **Todos 1 & 2** - Async loading + rsEvents (commit 0bc989a)
2. **Todo 4** - User notifications (commit 911e13d)
3. **Todo 5** - Search filter (commit 121d56d)
4. **Todo 3** - Edit merging placeholder (commit 3d62095)
5. **Todo 7** - Comments backend + UI (commit b5db8c4)
6. **Todo 8** - Blocking APIs (already exist)

### ❌ Pending:
- **Todo 6** - Share publish key: Requires forums-style moderator architecture (separate from publish rights)
- **Todo 3** - Actual content merging: Current implementation is placeholder (tracks edits, adds attribution)
- **Todo 7** - Advanced comment UI features: Basic integration complete, advanced features pending

---

*Last updated: 2026-01-19*
