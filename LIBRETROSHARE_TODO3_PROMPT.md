# Libretroshare Implementation: Todo 3 - Content Merging APIs

## Context

This prompt is for implementing content merging APIs in the **libretroshare** repository to enable full Todo 3 functionality (edit merging with actual content) for RetroShare Wiki.

**Current state:**
- Todo 3 has a placeholder UI in RetroShare GUI that tracks selected edits
- Missing: Backend APIs to fetch page content from wiki snapshots
- Goal: Enable real diff-based content merging in the GUI

**Repository:** https://github.com/samuel-asleep/libretroshare (or RetroShare/libretroshare)

---

## Implementation Steps

### Step 1: Add Interface Methods to rswiki.h

**File:** `libretroshare/src/retroshare/rswiki.h`

Add these pure virtual methods to the `RsWiki` interface class:

```cpp
/**
 * @brief Get page content from a single snapshot for merging
 * @param snapshotId The message ID of the snapshot
 * @param content Output parameter for page content
 * @return true if snapshot found and content retrieved
 */
virtual bool getSnapshotContent(const RsGxsMessageId& snapshotId, 
                                std::string& content) = 0;

/**
 * @brief Get page content from multiple snapshots efficiently (bulk fetch)
 * @param snapshotIds Vector of snapshot message IDs to fetch
 * @param contents Output map of snapshotId -> content
 * @return true if at least one snapshot was retrieved
 */
virtual bool getSnapshotsContent(const std::vector<RsGxsMessageId>& snapshotIds,
                                 std::map<RsGxsMessageId, std::string>& contents) = 0;
```

**Location:** Add after existing method declarations, before the closing `};` of the `RsWiki` class.

---

### Step 2: Implement Methods in p3wiki.h

**File:** `libretroshare/src/services/p3wiki.h`

Add these method declarations to the `p3Wiki` class:

```cpp
// Content fetching for merge operations (Todo 3)
virtual bool getSnapshotContent(const RsGxsMessageId& snapshotId, 
                                std::string& content) override;
virtual bool getSnapshotsContent(const std::vector<RsGxsMessageId>& snapshotIds,
                                 std::map<RsGxsMessageId, std::string>& contents) override;
```

**Location:** Add in the public section, grouped with other wiki-specific methods.

---

### Step 3: Implement Methods in p3wiki.cc

**File:** `libretroshare/src/services/p3wiki.cc`

Add these implementations:

```cpp
bool p3Wiki::getSnapshotContent(const RsGxsMessageId& snapshotId, std::string& content)
{
    // Use token-based request to fetch snapshot
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    
    std::list<RsGxsGroupId> grpIds; // Empty - we're searching by message ID
    std::vector<RsGxsGrpMsgIdPair> msgIds;
    msgIds.push_back(RsGxsGrpMsgIdPair(RsGxsGroupId(), snapshotId));
    
    if (!requestMsgInfo(token, opts, msgIds))
    {
        std::cerr << "p3Wiki::getSnapshotContent() requestMsgInfo failed" << std::endl;
        return false;
    }
    
    // Wait for request to complete
    RsTokenService::GxsRequestStatus status = RsTokenService::PENDING;
    while (status == RsTokenService::PENDING)
    {
        checkRequestStatus(token, status);
        if (status == RsTokenService::PENDING)
            rstime::rs_usleep(10000); // 10ms
    }
    
    if (status != RsTokenService::COMPLETE)
    {
        std::cerr << "p3Wiki::getSnapshotContent() request failed with status " 
                  << status << std::endl;
        return false;
    }
    
    // Get snapshot data
    std::vector<RsWikiSnapshot> snapshots;
    if (!getSnapshots(token, snapshots) || snapshots.empty())
    {
        std::cerr << "p3Wiki::getSnapshotContent() no snapshot found" << std::endl;
        return false;
    }
    
    content = snapshots[0].mPage;
    return true;
}

bool p3Wiki::getSnapshotsContent(const std::vector<RsGxsMessageId>& snapshotIds,
                                 std::map<RsGxsMessageId, std::string>& contents)
{
    if (snapshotIds.empty())
        return false;
    
    // Use token-based request for bulk fetch
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    
    std::vector<RsGxsGrpMsgIdPair> msgIds;
    for (const auto& msgId : snapshotIds)
        msgIds.push_back(RsGxsGrpMsgIdPair(RsGxsGroupId(), msgId));
    
    if (!requestMsgInfo(token, opts, msgIds))
    {
        std::cerr << "p3Wiki::getSnapshotsContent() requestMsgInfo failed" << std::endl;
        return false;
    }
    
    // Wait for request to complete
    RsTokenService::GxsRequestStatus status = RsTokenService::PENDING;
    while (status == RsTokenService::PENDING)
    {
        checkRequestStatus(token, status);
        if (status == RsTokenService::PENDING)
            rstime::rs_usleep(10000); // 10ms
    }
    
    if (status != RsTokenService::COMPLETE)
    {
        std::cerr << "p3Wiki::getSnapshotsContent() request failed with status " 
                  << status << std::endl;
        return false;
    }
    
    // Get snapshot data
    std::vector<RsWikiSnapshot> snapshots;
    if (!getSnapshots(token, snapshots))
    {
        std::cerr << "p3Wiki::getSnapshotsContent() no snapshots found" << std::endl;
        return false;
    }
    
    // Map snapshotId -> content
    for (const auto& snapshot : snapshots)
        contents[snapshot.mMsgId] = snapshot.mPage;
    
    return !contents.empty();
}
```

**Location:** Add near other wiki-specific methods, after the moderator methods if Todo 6 is already implemented.

---

### Step 4: Testing

**Test the new APIs:**

1. **Build the library:**
   ```bash
   cd libretroshare/src
   qmake
   make
   ```

2. **Verify compilation:** Ensure no errors

3. **Integration test (requires GUI):**
   - Once submodule updated in main RetroShare repo
   - Use APIs in WikiEditDialog to fetch content
   - Verify page content is correctly retrieved

---

## Usage in GUI (RetroShare Main Repo)

Once these APIs are implemented, update `WikiEditDialog.cpp` in the main RetroShare repo:

```cpp
// In generateMerge() or similar method:

// Get selected snapshot IDs
std::vector<RsGxsMessageId> snapshotIds;
for (QTreeWidgetItem *item : checkedItems)
{
    RsGxsMessageId msgId(item->text(WET_COL_PAGEID).toStdString());
    snapshotIds.push_back(msgId);
}

// Fetch content from all selected snapshots
std::map<RsGxsMessageId, std::string> contents;
if (rsWiki->getSnapshotsContent(snapshotIds, contents))
{
    // Build merged content with section markers
    QString mergedText;
    for (const auto& item : checkedItems)
    {
        RsGxsMessageId msgId(item->text(WET_COL_PAGEID).toStdString());
        QString author = item->text(WET_COL_AUTHOR);
        QString date = item->data(WET_COL_DATE, WET_ROLE_SORT).toString();
        
        if (contents.count(msgId) > 0)
        {
            mergedText += QString("\n\n===== Edit by %1 on %2 =====\n").arg(author, date);
            mergedText += QString::fromStdString(contents[msgId]);
        }
    }
    
    // Set merged content in edit field
    ui->textEdit->setPlainText(mergedText);
}
```

---

## Implementation Notes

### Error Handling
- Return `false` if snapshot not found
- Log errors to stderr for debugging
- Handle empty snapshot lists gracefully

### Performance
- `getSnapshotsContent()` is more efficient for multiple snapshots
- Single token request for bulk operations
- Reuses existing GXS infrastructure

### Backward Compatibility
- No changes to existing data structures
- Only adds new methods to interface
- Existing code continues to work

---

## Testing Checklist

- [ ] Code compiles without errors
- [ ] Methods added to rswiki.h interface
- [ ] Methods declared in p3wiki.h
- [ ] Methods implemented in p3wiki.cc
- [ ] Single snapshot fetch works (`getSnapshotContent`)
- [ ] Bulk snapshot fetch works (`getSnapshotsContent`)
- [ ] GUI integration (when submodule updated)
- [ ] Merge UI displays actual page content

---

## Commit Message

```
Wiki: Add content fetching APIs for edit merging (Todo 3)

Implement getSnapshotContent() and getSnapshotsContent() methods to
enable full content merging functionality in Wiki edit dialog.

Changes:
- Added content fetching methods to RsWiki interface (rswiki.h)
- Implemented in p3Wiki class (p3wiki.h/cc)
- Uses GXS token-based requests for efficient bulk fetching
- Returns page content mapped by snapshot message ID

These APIs enable the GUI to fetch actual page content from selected
edits for diff-based merging, completing Todo 3 implementation.

Related: RetroShare/RetroShare#3107 (Todo 3)
```

---

## Summary

This implementation adds content fetching capabilities to libretroshare, enabling the RetroShare GUI to:
1. Fetch page content from individual wiki snapshots
2. Bulk fetch content from multiple snapshots efficiently
3. Implement real diff-based merging with actual page content
4. Complete Todo 3 with full functionality (not just placeholder)

**Next steps after implementing:**
1. Build and test libretroshare
2. Update submodule reference in main RetroShare repo
3. Update WikiEditDialog.cpp to use new APIs
4. Implement diff/merge algorithm in GUI
5. Test complete merge workflow
