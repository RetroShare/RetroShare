# libretroshare Implementation Prompt for Wiki Modernization

This prompt provides complete implementation guidance for Wiki modernization work that requires changes to the libretroshare submodule.

## Context

This implementation is for **Issue #3107: Wiki Modernization** in the RetroShare repository. The GUI-level work has been completed in the main RetroShare repository. This prompt covers the backend changes needed in libretroshare.

## Required Implementation: Todo 6 - Forums-Style Moderator System

### Overview

Implement a forums-style moderator system for Wiki that is **independent from publish rights**. This allows admins to grant/revoke moderator permissions without sharing publish keys.

### Requirements (from Issue #3107 discussion)

**Authentication Architecture:**
- Keep existing auth policy flags unchanged (MSG_AUTHEN_ROOT_PUBLISH_SIGN | MSG_AUTHEN_CHILD_AUTHOR_SIGN)
- Add moderator validation ON TOP of existing authentication
- Moderator list is admin-controlled and separate from publish rights
- Moderators can edit ANY message (not just their own)
- Moderator permissions are revocable (with termination dates)
- Validation enforced network-wide (all nodes check moderator list)

### Files to Modify

#### 1. Data Structures: `libretroshare/src/rsitems/rswikiitems.h`

Add moderator list to RsWikiCollection:

```cpp
// Add to RsWikiCollection class
class RsWikiCollection
{
public:
    // ... existing fields ...
    
    // Moderator list (forums-style)
    std::list<RsGxsId> mModeratorList;  // List of moderator GxsIds
    std::map<RsGxsId, rstime_t> mModeratorTerminationDates;  // Termination dates for removed moderators
    
    // ... rest of class ...
};
```

#### 2. Serialization: `libretroshare/src/rsitems/rswikiitems.cc`

Update RsWikiCollectionItem serialization:

```cpp
// In RsWikiCollectionItem::serial_process()
void RsWikiCollectionItem::serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_COMMENT, mDescription, "mDescription");
    RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_HASH_TAG, mHashTags, "mHashTags");
    
    // Add moderator list serialization
    RsTypeSerializer::serial_process<RsGxsId>(j, ctx, mModeratorList, "mModeratorList");
    RsTypeSerializer::serial_process(j, ctx, mModeratorTerminationDates, "mModeratorTerminationDates");
}
```

#### 3. Interface: `libretroshare/src/retroshare/rswiki.h`

Add moderator management methods:

```cpp
class RsWiki : public RsGxsIfaceHelper
{
public:
    // ... existing methods ...
    
    // Moderator management (admin only)
    virtual bool addModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId) = 0;
    virtual bool removeModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId) = 0;
    virtual bool getModerators(const RsGxsGroupId& grpId, std::list<RsGxsId>& moderators) = 0;
    virtual bool isActiveModerator(const RsGxsGroupId& grpId, const RsGxsId& authorId, rstime_t editTime) = 0;
    
    // ... rest of class ...
};
```

#### 4. Implementation: `libretroshare/src/services/p3wiki.h`

Declare methods in p3Wiki class:

```cpp
class p3Wiki : public p3GxsCommentService, public RsWiki
{
public:
    // ... existing methods ...
    
    // Moderator management
    virtual bool addModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId) override;
    virtual bool removeModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId) override;
    virtual bool getModerators(const RsGxsGroupId& grpId, std::list<RsGxsId>& moderators) override;
    virtual bool isActiveModerator(const RsGxsGroupId& grpId, const RsGxsId& authorId, rstime_t editTime) override;
    
private:
    // Helper for moderator validation
    bool checkModeratorPermission(const RsGxsGroupId& grpId, const RsGxsId& authorId, const RsGxsId& originalAuthorId, rstime_t editTime);
};
```

#### 5. Implementation: `libretroshare/src/services/p3wiki.cc`

Implement moderator methods:

```cpp
bool p3Wiki::addModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId)
{
    // Get group metadata
    std::vector<RsWikiCollection> collections;
    if (!getCollections({grpId}, collections) || collections.empty())
        return false;
    
    RsWikiCollection& collection = collections[0];
    
    // Check if caller is admin (has publish rights)
    // ... admin check logic ...
    
    // Add moderator to list
    collection.mModeratorList.push_back(moderatorId);
    collection.mModeratorList.sort();
    collection.mModeratorList.unique();
    
    // Update group metadata
    uint32_t token;
    return updateGroup(token, collection);
}

bool p3Wiki::removeModerator(const RsGxsGroupId& grpId, const RsGxsId& moderatorId)
{
    std::vector<RsWikiCollection> collections;
    if (!getCollections({grpId}, collections) || collections.empty())
        return false;
    
    RsWikiCollection& collection = collections[0];
    
    // Remove from active list
    collection.mModeratorList.remove(moderatorId);
    
    // Add termination date
    collection.mModeratorTerminationDates[moderatorId] = time(nullptr);
    
    // Update group
    uint32_t token;
    return updateGroup(token, collection);
}

bool p3Wiki::getModerators(const RsGxsGroupId& grpId, std::list<RsGxsId>& moderators)
{
    std::vector<RsWikiCollection> collections;
    if (!getCollections({grpId}, collections) || collections.empty())
        return false;
    
    moderators = collections[0].mModeratorList;
    return true;
}

bool p3Wiki::isActiveModerator(const RsGxsGroupId& grpId, const RsGxsId& authorId, rstime_t editTime)
{
    std::vector<RsWikiCollection> collections;
    if (!getCollections({grpId}, collections) || collections.empty())
        return false;
    
    const RsWikiCollection& collection = collections[0];
    
    // Check if in active moderator list
    if (std::find(collection.mModeratorList.begin(), collection.mModeratorList.end(), authorId) == collection.mModeratorList.end())
        return false;
    
    // Check if terminated before edit time
    auto it = collection.mModeratorTerminationDates.find(authorId);
    if (it != collection.mModeratorTerminationDates.end() && editTime >= it->second)
        return false;
    
    return true;
}

bool p3Wiki::checkModeratorPermission(const RsGxsGroupId& grpId, const RsGxsId& authorId, const RsGxsId& originalAuthorId, rstime_t editTime)
{
    // Self-edits are always allowed
    if (authorId == originalAuthorId)
        return true;
    
    // Check if author is an active moderator
    return isActiveModerator(grpId, authorId, editTime);
}
```

#### 6. Message Validation: Update `p3wiki.cc` message processing

Add moderator validation in message reception/validation:

```cpp
// In p3Wiki::notifyChanges() or wherever messages are validated
void p3Wiki::validateWikiMessage(const RsGxsGroupId& grpId, const RsGxsMessageId& msgId, const RsGxsId& authorId, rstime_t timestamp)
{
    // Get original message author (for edits)
    RsGxsId originalAuthorId = getOriginalMessageAuthor(grpId, msgId);
    
    // Validate moderator permission for non-self edits
    if (authorId != originalAuthorId)
    {
        if (!checkModeratorPermission(grpId, authorId, originalAuthorId, timestamp))
        {
            // Reject message - author is not an active moderator
            std::cerr << "p3Wiki: Rejecting edit from non-moderator " << authorId << " on message by " << originalAuthorId << std::endl;
            // Mark message as invalid/rejected
            return;
        }
    }
    
    // Message passes validation
}
```

### Testing Requirements

1. **Backward Compatibility:**
   - Old groups without moderator list should work (empty list)
   - Serialization should handle missing fields gracefully

2. **Moderator Management:**
   - Test adding moderators
   - Test removing moderators
   - Test moderator list retrieval

3. **Validation:**
   - Self-edits always allowed
   - Active moderators can edit any message
   - Non-moderators cannot edit others' messages
   - Terminated moderators cannot edit after termination date

4. **Network-wide Enforcement:**
   - Validation happens on all nodes
   - Invalid edits are rejected consistently

### Build and Test

```bash
cd libretroshare
qmake
make
# Run tests
```

---

## Optional Future Enhancement: Todo 3 - Content Merging

### Overview

Todo 3 currently has a placeholder UI for merge tracking. Full implementation would require backend APIs to fetch snapshot content efficiently.

### Required API Addition

Add to `libretroshare/src/retroshare/rswiki.h`:

```cpp
// Fetch snapshot content for merging
virtual bool getSnapshotContent(const RsGxsMessageId& snapshotId, std::string& content) = 0;

// Bulk fetch for efficient merge operations
virtual bool getSnapshotsContent(const std::vector<RsGxsMessageId>& snapshotIds, 
                                 std::map<RsGxsMessageId, std::string>& contents) = 0;
```

Implementation in `p3wiki.cc`:

```cpp
bool p3Wiki::getSnapshotContent(const RsGxsMessageId& snapshotId, std::string& content)
{
    // Fetch snapshot message
    std::vector<RsWikiSnapshot> snapshots;
    if (!getSnapshots({snapshotId}, snapshots) || snapshots.empty())
        return false;
    
    content = snapshots[0].mPage;
    return true;
}

bool p3Wiki::getSnapshotsContent(const std::vector<RsGxsMessageId>& snapshotIds,
                                 std::map<RsGxsMessageId, std::string>& contents)
{
    std::vector<RsWikiSnapshot> snapshots;
    if (!getSnapshots(snapshotIds, snapshots))
        return false;
    
    for (const auto& snapshot : snapshots)
        contents[snapshot.mMsgId] = snapshot.mPage;
    
    return true;
}
```

### GUI Implementation (in main RetroShare repo)

Once API exists, update `WikiEditDialog.cpp` to:
1. Call `getSnapshotsContent()` for selected edits
2. Implement diff-based merging algorithm
3. Show merged content with section markers
4. Detect and highlight conflicts

**Note:** This is optional future work, not required for current PR.

---

## Summary

**Required Now:**
- Todo 6: Forums-style moderator system (complete specification above)

**Optional Future:**
- Todo 3: Content merging APIs (specification above, but not required)

**Already Complete:**
- Todo 8: Blocking APIs (getCollections, etc.) - implemented 5+ years ago, working

---

## Implementation Checklist

- [ ] Update RsWikiCollection data structure
- [ ] Add serialization for moderator fields
- [ ] Implement interface methods in rswiki.h
- [ ] Implement p3Wiki moderator management
- [ ] Add message validation logic
- [ ] Test backward compatibility
- [ ] Test moderator add/remove
- [ ] Test validation enforcement
- [ ] Update GUI (WikiGroupDialog) to use new APIs
- [ ] Document changes

---

**Created for Issue #3107 Wiki Modernization**
**Main Repository PR: samuel-asleep/RetroShare#5**
