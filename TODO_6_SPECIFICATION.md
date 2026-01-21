# Todo 6: Forums-Style Moderator Implementation - Complete Specification

## Overview
Implement a forums-style moderator system for WikiPoos that allows admin-controlled, revocable moderator management independent from publish key sharing.

## Requirements

### 1. Data Model Changes (libretroshare)

#### File: `libretroshare/src/rsitems/rswikiitems.h`

Add to `RsGxsWikiGroupItem`:
```cpp
struct ModeratorEntry {
    RsGxsId mGxsId;           // Moderator's GXS ID
    rstime_t mAddedTime;      // When added as moderator
    rstime_t mTerminatedTime; // 0 if active, timestamp if removed
    bool isActive(rstime_t msgTime) const {
        return mTerminatedTime == 0 || msgTime < mTerminatedTime;
    }
};

std::vector<ModeratorEntry> mModeratorList;  // Admin-controlled moderator list
```

#### File: `libretroshare/src/rsitems/rswikiitems.cc`

Implement serialization for moderator list:
```cpp
void RsGxsWikiGroupItem::serial_process(RsGenericSerializer::SerializeJob j, 
                                        RsGenericSerializer::SerializeContext& ctx)
{
    // ... existing fields ...
    
    // Serialize moderator list
    RS_SERIAL_PROCESS(mModeratorList);
}
```

#### File: `libretroshare/src/retroshare/rswiki.h`

Update `RsWikiCollection`:
```cpp
struct ModeratorInfo {
    RsGxsId gxsId;
    rstime_t addedTime;
    rstime_t terminatedTime;
};

struct RsWikiCollection: RsGxsGenericGroupData
{
    std::string mDescription;
    std::string mCategory;
    std::string mHashTags;
    std::vector<ModeratorInfo> mModerators;  // NEW: Moderator list
};
```

### 2. Validation Logic (libretroshare)

#### File: `libretroshare/src/services/p3wiki.cc`

Add validation method:
```cpp
bool p3Wiki::validateWikiMessage(const RsGxsWikiSnapshotItem* snapshot,
                                 const RsWikiCollection& collection)
{
    // Rule 1: Self-edits always allowed
    if (snapshot->meta.mAuthorId == snapshot->meta.mOrigMsgId) {
        return true;
    }
    
    // Rule 2: Check if author is an active moderator
    rstime_t msgTime = snapshot->meta.mPublishTs;
    for (const auto& mod : collection.mModerators) {
        if (mod.gxsId == snapshot->meta.mAuthorId) {
            // Check if moderator was active at message publish time
            if (mod.terminatedTime == 0 || msgTime < mod.terminatedTime) {
                return true;
            }
        }
    }
    
    // Rule 3: Discard if not self-edit and not by active moderator
    return false;
}
```

Integrate into message processing:
```cpp
void p3Wiki::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
    for (auto change : changes) {
        // ... existing code ...
        
        // NEW: Validate edits against moderator list
        if (change->getType() == NOTIFY_TYPE_ADD || 
            change->getType() == NOTIFY_TYPE_MOD) {
            
            // Get group data
            RsWikiCollection collection;
            if (getCollectionData(change->mGroupId, collection)) {
                // Validate each message
                for (const auto& msgId : change->mMsgIdList) {
                    RsGxsWikiSnapshotItem* snapshot = /* get snapshot */;
                    if (!validateWikiMessage(snapshot, collection)) {
                        // Discard invalid message
                        rejectMessage(msgId);
                    }
                }
            }
        }
    }
}
```

### 3. Interface Methods (libretroshare)

#### File: `libretroshare/src/retroshare/rswiki.h`

Add moderator management methods:
```cpp
class RsWiki: public RsGxsIfaceHelper
{
public:
    // ... existing methods ...
    
    /**
     * Add a moderator to a wiki group
     * @param groupId Wiki group ID
     * @param gxsId GXS ID to add as moderator
     * @return true on success
     */
    virtual bool addModerator(const RsGxsGroupId& groupId, const RsGxsId& gxsId) = 0;
    
    /**
     * Remove a moderator from a wiki group
     * @param groupId Wiki group ID  
     * @param gxsId GXS ID to remove
     * @return true on success
     */
    virtual bool removeModerator(const RsGxsGroupId& groupId, const RsGxsId& gxsId) = 0;
    
    /**
     * Get list of active moderators for a group
     * @param groupId Wiki group ID
     * @param moderators Output list of moderators
     * @return true on success
     */
    virtual bool getModerators(const RsGxsGroupId& groupId, 
                              std::vector<ModeratorInfo>& moderators) = 0;
};
```

#### File: `libretroshare/src/services/p3wiki.h` + `.cc`

Implement the methods:
```cpp
bool p3Wiki::addModerator(const RsGxsGroupId& groupId, const RsGxsId& gxsId)
{
    RsWikiCollection collection;
    if (!getCollectionData(groupId, collection)) return false;
    
    // Check if already a moderator
    for (const auto& mod : collection.mModerators) {
        if (mod.gxsId == gxsId && mod.terminatedTime == 0) {
            return false;  // Already active moderator
        }
    }
    
    // Add new moderator
    ModeratorInfo newMod;
    newMod.gxsId = gxsId;
    newMod.addedTime = time(nullptr);
    newMod.terminatedTime = 0;
    collection.mModerators.push_back(newMod);
    
    // Update group
    return updateCollection(collection);
}

bool p3Wiki::removeModerator(const RsGxsGroupId& groupId, const RsGxsId& gxsId)
{
    RsWikiCollection collection;
    if (!getCollectionData(groupId, collection)) return false;
    
    // Find and terminate moderator
    bool found = false;
    for (auto& mod : collection.mModerators) {
        if (mod.gxsId == gxsId && mod.terminatedTime == 0) {
            mod.terminatedTime = time(nullptr);
            found = true;
            break;
        }
    }
    
    if (!found) return false;
    
    // Update group
    return updateCollection(collection);
}
```

### 4. GUI Integration (retroshare-gui)

#### File: `retroshare-gui/src/gui/gxs/WikiGroupDialog.cpp`

Add moderator management UI (similar to GxsForumGroupDialog):
```cpp
// Add in setupUI():
QGroupBox *moderatorBox = new QGroupBox(tr("Moderators"));
QVBoxLayout *modLayout = new QVBoxLayout();

mModeratorListWidget = new QListWidget();
QPushButton *addModBtn = new QPushButton(tr("Add Moderator"));
QPushButton *removeModBtn = new QPushButton(tr("Remove Moderator"));

connect(addModBtn, SIGNAL(clicked()), this, SLOT(addModerator()));
connect(removeModBtn, SIGNAL(clicked()), this, SLOT(removeModerator()));

modLayout->addWidget(mModeratorListWidget);
modLayout->addWidget(addModBtn);
modLayout->addWidget(removeModBtn);
moderatorBox->setLayout(modLayout);

// Add to main layout
mainLayout->addWidget(moderatorBox);
```

Add moderator management slots:
```cpp
void WikiGroupDialog::addModerator()
{
    // Show GXS ID selection dialog
    RsGxsId selectedId = selectGxsId();
    if (selectedId.isNull()) return;
    
    // Add to list
    if (rsWiki->addModerator(mGroupId, selectedId)) {
        refreshModeratorList();
    }
}

void WikiGroupDialog::removeModerator()
{
    QListWidgetItem *item = mModeratorListWidget->currentItem();
    if (!item) return;
    
    RsGxsId gxsId = item->data(Qt::UserRole).value<RsGxsId>();
    
    if (rsWiki->removeModerator(mGroupId, gxsId)) {
        refreshModeratorList();
    }
}

void WikiGroupDialog::refreshModeratorList()
{
    mModeratorListWidget->clear();
    
    std::vector<RsWiki::ModeratorInfo> moderators;
    if (rsWiki->getModerators(mGroupId, moderators)) {
        for (const auto& mod : moderators) {
            if (mod.terminatedTime == 0) {  // Only show active
                QString name = getGxsIdName(mod.gxsId);
                QListWidgetItem *item = new QListWidgetItem(name);
                item->setData(Qt::UserRole, QVariant::fromValue(mod.gxsId));
                mModeratorListWidget->addItem(item);
            }
        }
    }
}
```

## Testing Plan

### Unit Tests
1. Test moderator serialization/deserialization
2. Test validation logic with various moderator scenarios
3. Test backward compatibility with old wiki data

### Integration Tests
1. Create wiki group and add moderators
2. Test that non-moderator edits are rejected
3. Test that moderator edits are accepted
4. Test moderator removal (terminated moderators can't edit)
5. Test self-edits (always allowed)

### Network Tests
1. Verify validation happens on all nodes
2. Test sync between nodes with different moderator lists
3. Test with mixed old/new client versions

## Backward Compatibility

- Old clients will see empty moderator lists (default: allow all)
- New clients handle missing moderator data gracefully
- Validation only rejects if moderator list exists and user not in it
- Self-edits always allowed regardless of moderator list

## Implementation Order

1. Data model changes (libretroshare)
2. Serialization (libretroshare)
3. Validation logic (libretroshare)
4. Interface methods (libretroshare)
5. GUI integration (retroshare-gui)
6. Testing

## Notes

- This requires changes to the **libretroshare git submodule**
- Must be done as separate PR in libretroshare repository
- Main RetroShare repo then updates submodule reference
- Estimated effort: 2-3 days for implementation + testing
