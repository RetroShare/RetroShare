# GUI Implementation Guides for Issue #3107
## Remaining Wiki Todos - Detailed Implementation Instructions

**Repository:** samuel-asleep/RetroShare  
**Date:** 2026-01-22  
**Status:** All backend work complete, GUI integration remaining

---

## Overview

This document provides step-by-step implementation guides for the remaining GUI work to complete issue #3107. All required backend APIs are implemented and ready for use.

**Remaining Todos:**
1. Todo 1: WikiEditDialog async conversion (Low Priority)
2. Todo 3: Content merging integration (High Priority)
3. Todo 4: MainWindow notification wiring (Low Priority)
4. Todo 6: Moderator management UI (Medium Priority)

---

# Todo 1: Convert WikiEditDialog to Async Pattern

## Priority: Low
## Estimated Effort: 1-2 days
## Status: WikiDialog modernized ✅, WikiEditDialog still uses TokenQueue ❌

### Current State

**File:** `retroshare-gui/src/gui/WikiPoos/WikiEditDialog.h` and `.cpp`

**Issues:**
- Line 27: `#include "util/TokenQueue.h"`
- Line 31: `class WikiEditDialog : public QWidget, public TokenResponse`
- Line 74: `mWikiQueue = new TokenQueue(rsWiki->getTokenService(), this)`
- Uses token-based polling instead of modern async/event pattern

### Implementation Steps

#### 1.1 Remove TokenQueue Dependencies

**WikiEditDialog.h changes:**

```cpp
// REMOVE these lines:
#include "util/TokenQueue.h"
class WikiEditDialog : public QWidget, public TokenResponse
void loadRequest(const TokenQueue *queue, const TokenRequest &req);

// REPLACE with:
#include "util/RsThread.h"
class WikiEditDialog : public QWidget
// Remove loadRequest() - no longer needed
```

#### 1.2 Update Constructor and Member Variables

**WikiEditDialog.cpp changes:**

```cpp
// REMOVE from constructor:
mWikiQueue = new TokenQueue(rsWiki->getTokenService(), this);

// REMOVE member variable:
TokenQueue *mWikiQueue;

// ADD event handler registration (if not already present):
rsEvents->registerEventsHandler(
    [this](std::shared_ptr<const RsEvent> event) {
        if (event->mType == rsEvents->getDynamicEventType("GXS_WIKI")) {
            // Handle wiki events
            RsQThreadUtils::postToObject([this]() {
                // Refresh data as needed
            }, this);
        }
    },
    this
);
```

#### 1.3 Convert Token-Based Loading to Async

**Replace all token request patterns like this:**

```cpp
// OLD pattern:
uint32_t token;
rsWiki->requestSomething(token, params);
mWikiQueue->queueRequest(token, TOKENREQ_SOMETHING, RS_TOKREQ_ANSTYPE_DATA);

// NEW pattern using RsThread::async:
RsThread::async([params]() {
    // Fetch data on background thread
    std::vector<RsWikiSnapshot> snapshots;
    if (rsWiki->getSomething(params, snapshots)) {
        return snapshots;
    }
    return std::vector<RsWikiSnapshot>();
}, [this](std::vector<RsWikiSnapshot>&& snapshots) {
    // Update UI on main thread
    updateUIWithSnapshots(snapshots);
});
```

#### 1.4 Remove loadRequest() Implementation

**Delete the entire loadRequest() method** - it's no longer needed with async pattern.

#### 1.5 Update All Data Loading Methods

Convert these methods from token-based to async:
- `requestPage()`
- `loadPage()`
- `requestGroup()`
- `loadGroup()`
- `requestBaseHistory()`
- `loadBaseHistory()`
- `requestEditTreeData()`
- `loadEditTreeData()`

**Example conversion for requestPage():**

```cpp
// OLD:
void WikiEditDialog::requestPage(const RsGxsGrpMsgIdPair &msgId)
{
    uint32_t token;
    std::list<RsGxsGrpMsgIdPair> msgIds;
    msgIds.push_back(msgId);
    rsWiki->requestSnapshots(token, msgIds);
    mWikiQueue->queueRequest(token, TOKENREQ_PAGEREQUEST, RS_TOKREQ_ANSTYPE_DATA);
}

// NEW:
void WikiEditDialog::requestPage(const RsGxsGrpMsgIdPair &msgId)
{
    RsThread::async([msgId]() {
        std::vector<RsWikiSnapshot> snapshots;
        std::list<RsGxsGrpMsgIdPair> msgIds = {msgId};
        
        uint32_t token;
        rsWiki->getSnapshots(token, msgIds); // Use async API
        // Or use blocking API if available:
        // rsWiki->getSnapshots(msgIds, snapshots);
        
        return snapshots;
    }, [this](std::vector<RsWikiSnapshot>&& snapshots) {
        if (!snapshots.empty()) {
            setPreviousPage(snapshots[0]);
        }
    });
}
```

#### 1.6 Testing

1. Build the modified code
2. Test page editing functionality
3. Verify history loading works
4. Check republish mode
5. Ensure merge mode still functions
6. Verify no polling overhead (check CPU usage)

### Reference Implementation

See `WikiDialog.cpp` (lines 423-584) for working async pattern examples.

---

# Todo 3: Integrate Content Merging

## Priority: High (Blocking)
## Estimated Effort: 3-5 days
## Status: Backend APIs ready ✅, GUI integration incomplete ❌

### Current State

**Files:** `WikiEditDialog.cpp/h`

**What Exists:**
- Merge UI components (merge checkbox, merge button, edit tree)
- `generateMerge()` method with placeholder implementation (lines 113-173)
- Backend APIs available: `getSnapshotContent()`, `getSnapshotsContent()`

**What's Missing:**
- Actual content fetching from selected edits
- Diff/merge algorithms
- Accept/reject edit functionality

### Implementation Steps

#### 3.1 Integrate getSnapshotsContent() API

**Update generateMerge() method:**

```cpp
void WikiEditDialog::generateMerge()
{
    // Get selected edit checkboxes from history tree
    std::vector<RsGxsMessageId> selectedEditIds;
    
    QTreeWidgetItemIterator it(ui.treeWidget_History, QTreeWidgetItemIterator::Checked);
    while (*it) {
        QTreeWidgetItem *item = *it;
        RsGxsMessageId msgId(item->data(0, Qt::UserRole).toString().toStdString());
        if (!msgId.isNull()) {
            selectedEditIds.push_back(msgId);
        }
        ++it;
    }
    
    if (selectedEditIds.empty()) {
        QMessageBox::information(this, tr("Merge"), 
            tr("Please select at least one edit to merge."));
        return;
    }
    
    // Fetch content from selected edits using backend API
    RsThread::async([selectedEditIds]() {
        std::map<RsGxsMessageId, std::string> contents;
        if (rsWiki->getSnapshotsContent(selectedEditIds, contents)) {
            return contents;
        }
        return std::map<RsGxsMessageId, std::string>();
    }, [this, selectedEditIds](std::map<RsGxsMessageId, std::string>&& contents) {
        performMerge(selectedEditIds, contents);
    });
}
```

#### 3.2 Implement Merge Logic

**Add new method performMerge():**

```cpp
void WikiEditDialog::performMerge(
    const std::vector<RsGxsMessageId>& editIds,
    const std::map<RsGxsMessageId, std::string>& contents)
{
    if (contents.empty()) {
        QMessageBox::warning(this, tr("Merge Failed"),
            tr("Could not fetch content from selected edits."));
        return;
    }
    
    // Sort edits by timestamp (chronological order)
    std::vector<std::pair<RsGxsMessageId, rstime_t>> sortedEdits;
    for (const auto& msgId : editIds) {
        // Get timestamp from tree widget item or metadata
        rstime_t timestamp = getEditTimestamp(msgId);
        sortedEdits.push_back({msgId, timestamp});
    }
    std::sort(sortedEdits.begin(), sortedEdits.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Build merged content
    QString mergedText;
    mergedText += "<!-- MERGED CONTENT FROM MULTIPLE EDITS -->\n\n";
    
    for (const auto& [msgId, timestamp] : sortedEdits) {
        auto contentIt = contents.find(msgId);
        if (contentIt != contents.end()) {
            // Get author info
            QString authorName = getAuthorName(msgId);
            QString dateStr = QDateTime::fromTime_t(timestamp).toString();
            
            mergedText += QString("<!-- Edit by %1 on %2 -->\n")
                .arg(authorName).arg(dateStr);
            mergedText += QString::fromStdString(contentIt->second);
            mergedText += "\n\n";
        }
    }
    
    // Set merged content in editor
    ui.textEdit_Page->setPlainText(mergedText);
    
    // Show success message
    QMessageBox::information(this, tr("Merge Complete"),
        tr("Content from %1 edit(s) has been merged. Review and submit.").arg(sortedEdits.size()));
}
```

#### 3.3 Add Advanced Merge Options (Optional)

For more sophisticated merging, consider:

**Three-way merge algorithm:**

```cpp
QString WikiEditDialog::performThreeWayMerge(
    const QString& baseContent,
    const QString& edit1Content,
    const QString& edit2Content)
{
    // Use diff algorithm library (e.g., QtDiff or custom)
    // 1. Compute diff between base and edit1
    // 2. Compute diff between base and edit2
    // 3. Apply both diffs to base
    // 4. Detect conflicts and mark them
    
    QString merged = baseContent;
    
    // Apply changes from edit1
    // Apply changes from edit2
    // Mark conflicts with special markers:
    // <<<<<<< EDIT1
    // content from edit1
    // =======
    // content from edit2
    // >>>>>>> EDIT2
    
    return merged;
}
```

#### 3.4 Add Accept/Reject Workflow (For Moderators)

**Add context menu to history tree:**

```cpp
void WikiEditDialog::showHistoryContextMenu(const QPoint& pos)
{
    QTreeWidgetItem *item = ui.treeWidget_History->itemAt(pos);
    if (!item) return;
    
    RsGxsMessageId editId(item->data(0, Qt::UserRole).toString().toStdString());
    
    QMenu menu;
    QAction *acceptAction = menu.addAction(tr("Accept Edit"));
    QAction *rejectAction = menu.addAction(tr("Reject Edit"));
    
    // Only enable for moderators
    if (!isUserModerator()) {
        acceptAction->setEnabled(false);
        rejectAction->setEnabled(false);
    }
    
    QAction *selected = menu.exec(ui.treeWidget_History->mapToGlobal(pos));
    
    if (selected == acceptAction) {
        acceptEdit(editId);
    } else if (selected == rejectAction) {
        rejectEdit(editId);
    }
}

void WikiEditDialog::acceptEdit(const RsGxsMessageId& editId)
{
    // Mark edit as accepted (could use metadata or separate tracking)
    // Optionally merge into main content automatically
    
    QMessageBox::information(this, tr("Edit Accepted"),
        tr("Edit has been marked as accepted."));
}

void WikiEditDialog::rejectEdit(const RsGxsMessageId& editId)
{
    // Mark edit as rejected (metadata or tracking)
    // Could hide from tree or mark visually
    
    QMessageBox::information(this, tr("Edit Rejected"),
        tr("Edit has been marked as rejected."));
}
```

#### 3.5 UI Enhancements

**Add merge preview:**

```cpp
void WikiEditDialog::previewMerge()
{
    // Show preview dialog with merged content
    // before applying to editor
    
    QDialog *preview = new QDialog(this);
    preview->setWindowTitle(tr("Merge Preview"));
    
    QTextEdit *previewEdit = new QTextEdit(preview);
    previewEdit->setReadOnly(true);
    // ... set merged content ...
    
    preview->exec();
}
```

#### 3.6 Helper Methods Needed

```cpp
rstime_t WikiEditDialog::getEditTimestamp(const RsGxsMessageId& msgId);
QString WikiEditDialog::getAuthorName(const RsGxsMessageId& msgId);
bool WikiEditDialog::isUserModerator();
```

#### 3.7 Testing

1. Create a wiki page with multiple edits
2. Enable merge mode
3. Select 2-3 edits from history
4. Click "Merge" button
5. Verify content is fetched and combined
6. Check chronological ordering
7. Test with conflicting edits
8. Verify moderator accept/reject (if implemented)

---

# Todo 4: MainWindow Notification Wiring

## Priority: Low
## Estimated Effort: < 1 day
## Status: Backend complete ✅, MainWindow integration pending ❌

### Current State

**What Exists:**
- `WikiUserNotify` class fully implemented
- Notification counting logic working
- Auto-read status on page view

**What's Missing:**
- Wiki not in MainWindow::Page enum
- Notification counter not wired to MainWindow
- Requires wiki to be enabled with `CONFIG += wikipoos`

### Implementation Steps

#### 4.1 Add Wiki to MainWindow Enum

**File:** `retroshare-gui/src/gui/MainWindow.h`

```cpp
enum Page {
    Friends = 0,
    Network,
    Transfers,
    // ... other pages ...
    Wiki,  // ADD THIS
    // ...
};
```

#### 4.2 Wire Notification Counter

**File:** `retroshare-gui/src/gui/MainWindow.cpp`

**In constructor or initialization:**

```cpp
#ifdef ENABLE_WIKIPOOS
    // Create Wiki user notify
    mWikiUserNotify = new WikiUserNotify(this);
    
    // Connect to notification counter
    connect(mWikiUserNotify, SIGNAL(countChanged()), 
            this, SLOT(updateWikiNotificationCount()));
    
    // Set icon for Wiki button
    ui.actionWiki->setIcon(mWikiUserNotify->getMainIcon());
#endif
```

**Add update method:**

```cpp
void MainWindow::updateWikiNotificationCount()
{
#ifdef ENABLE_WIKIPOOS
    int count = mWikiUserNotify->getNewCount();
    
    if (count > 0) {
        // Show count badge on Wiki button/action
        ui.actionWiki->setText(tr("Wiki (%1)").arg(count));
        // Or use custom badge widget
    } else {
        ui.actionWiki->setText(tr("Wiki"));
    }
#endif
}
```

#### 4.3 Clear Notifications on View

**File:** `WikiDialog.cpp`

**Already implemented in loadWikiPage():**

```cpp
// Mark page as read (line 582)
rsWiki->setMessageReadStatus(msgId, true);
```

**Ensure notification counter updates:**

```cpp
void WikiDialog::loadWikiPage(const RsGxsMessageId& msgId)
{
    // ... existing code ...
    
    // Mark as read
    rsWiki->setMessageReadStatus(mCurrentPageId, true);
    
    // Trigger notification count update
    if (mUserNotify) {
        mUserNotify->updateCount();
    }
}
```

#### 4.4 Testing

1. Enable wiki: Add `CONFIG += wikipoos` to `retroshare.pri`
2. Build RetroShare
3. Subscribe to a wiki group
4. Have another user add content
5. Verify notification counter appears
6. Open wiki page
7. Verify counter clears

### Reference

See `TODO_4_MAINWINDOW_INTEGRATION.md` for complete step-by-step guide.

---

# Todo 6: Moderator Management UI

## Priority: Medium
## Estimated Effort: 2-3 days
## Status: Backend complete ✅, GUI not implemented ❌

### Current State

**Backend APIs Available:**
- `addModerator(grpId, moderatorId)`
- `removeModerator(grpId, moderatorId)`
- `getModerators(grpId, moderators)`
- `isActiveModerator(grpId, authorId, editTime)`

**What's Missing:**
- Moderator list UI in WikiGroupDialog
- Add/remove moderator buttons
- Moderator list display widget

### Implementation Steps

#### 6.1 Update WikiGroupDialog UI

**File:** `retroshare-gui/src/gui/gxs/WikiGroupDialog.ui`

**Add moderator section to the UI:**

Use Qt Designer or manually edit .ui file to add:

```xml
<widget class="QGroupBox" name="groupBox_Moderators">
  <property name="title">
    <string>Moderators</string>
  </property>
  <layout class="QVBoxLayout" name="verticalLayout_moderators">
    <item>
      <widget class="QListWidget" name="listWidget_Moderators">
        <property name="toolTip">
          <string>List of users who can moderate this wiki</string>
        </property>
      </widget>
    </item>
    <item>
      <layout class="QHBoxLayout" name="horizontalLayout_modButtons">
        <item>
          <widget class="QPushButton" name="pushButton_AddModerator">
            <property name="text">
              <string>Add Moderator</string>
            </property>
          </widget>
        </item>
        <item>
          <widget class="QPushButton" name="pushButton_RemoveModerator">
            <property name="text">
              <string>Remove Moderator</string>
            </property>
          </widget>
        </item>
        <item>
          <spacer name="horizontalSpacer_mod">
            <property name="orientation">
              <enum>Qt::Horizontal</enum>
            </property>
          </spacer>
        </item>
      </layout>
    </item>
  </layout>
</widget>
```

#### 6.2 Implement Backend Integration

**File:** `retroshare-gui/src/gui/gxs/WikiGroupDialog.cpp`

**Add includes:**

```cpp
#include <retroshare/rswiki.h>
#include <retroshare/rsidentity.h>
#include "gui/gxs/GxsIdChooser.h"
```

**Load moderators when opening group:**

```cpp
void WikiGroupDialog::loadGroup(const RsGxsGroupId& groupId)
{
    // ... existing code ...
    
    // Load moderator list
    loadModerators(groupId);
}

void WikiGroupDialog::loadModerators(const RsGxsGroupId& groupId)
{
    ui.listWidget_Moderators->clear();
    
    RsThread::async([groupId]() {
        std::list<RsGxsId> moderators;
        rsWiki->getModerators(groupId, moderators);
        return moderators;
    }, [this](std::list<RsGxsId>&& moderators) {
        for (const auto& modId : moderators) {
            addModeratorToList(modId);
        }
    });
}

void WikiGroupDialog::addModeratorToList(const RsGxsId& gxsId)
{
    // Get identity name
    QString name = QString::fromStdString(gxsId.toStdString());
    
    RsIdentityDetails details;
    if (rsIdentity->getIdDetails(gxsId, details)) {
        name = QString::fromUtf8(details.mNickname.c_str());
    }
    
    // Add to list with ID as user data
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, QString::fromStdString(gxsId.toStdString()));
    ui.listWidget_Moderators->addItem(item);
}
```

**Connect button signals:**

```cpp
// In constructor
connect(ui.pushButton_AddModerator, SIGNAL(clicked()), 
        this, SLOT(addModerator()));
connect(ui.pushButton_RemoveModerator, SIGNAL(clicked()), 
        this, SLOT(removeModerator()));
```

**Implement add moderator:**

```cpp
void WikiGroupDialog::addModerator()
{
    // Show GXS ID chooser dialog
    GxsIdChooser *chooser = new GxsIdChooser(this);
    chooser->setWindowTitle(tr("Select Moderator"));
    
    if (chooser->exec() == QDialog::Accepted) {
        RsGxsId selectedId;
        if (chooser->getChosenId(selectedId)) {
            // Add moderator via backend
            RsGxsGroupId groupId = mCurrentGroupId; // Store current group ID
            
            RsThread::async([groupId, selectedId]() {
                return rsWiki->addModerator(groupId, selectedId);
            }, [this, selectedId](bool success) {
                if (success) {
                    addModeratorToList(selectedId);
                    QMessageBox::information(this, tr("Success"),
                        tr("Moderator added successfully."));
                } else {
                    QMessageBox::warning(this, tr("Error"),
                        tr("Failed to add moderator."));
                }
            });
        }
    }
    
    delete chooser;
}
```

**Implement remove moderator:**

```cpp
void WikiGroupDialog::removeModerator()
{
    QListWidgetItem *item = ui.listWidget_Moderators->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Remove Moderator"),
            tr("Please select a moderator to remove."));
        return;
    }
    
    RsGxsId modId(item->data(Qt::UserRole).toString().toStdString());
    RsGxsGroupId groupId = mCurrentGroupId;
    
    // Confirm removal
    int ret = QMessageBox::question(this, tr("Remove Moderator"),
        tr("Are you sure you want to remove this moderator?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        RsThread::async([groupId, modId]() {
            return rsWiki->removeModerator(groupId, modId);
        }, [this, item](bool success) {
            if (success) {
                delete item;
                QMessageBox::information(this, tr("Success"),
                    tr("Moderator removed successfully."));
            } else {
                QMessageBox::warning(this, tr("Error"),
                    tr("Failed to remove moderator."));
            }
        });
    }
}
```

#### 6.3 Add Permission Checks

**Only allow group admins to modify moderators:**

```cpp
void WikiGroupDialog::updateModeratorControls()
{
    // Check if current user is admin
    bool isAdmin = IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags);
    
    ui.pushButton_AddModerator->setEnabled(isAdmin);
    ui.pushButton_RemoveModerator->setEnabled(isAdmin);
    
    if (!isAdmin) {
        ui.pushButton_AddModerator->setToolTip(
            tr("Only group administrators can manage moderators"));
        ui.pushButton_RemoveModerator->setToolTip(
            tr("Only group administrators can manage moderators"));
    }
}
```

#### 6.4 Visual Enhancements

**Show moderator status:**

```cpp
void WikiGroupDialog::addModeratorToList(const RsGxsId& gxsId)
{
    QString name = getIdentityName(gxsId);
    
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, QString::fromStdString(gxsId.toStdString()));
    
    // Add icon for moderator
    item->setIcon(QIcon(":/images/moderator.png"));
    
    // Check if currently active
    bool isActive = rsWiki->isActiveModerator(mCurrentGroupId, gxsId, time(NULL));
    if (!isActive) {
        item->setForeground(Qt::gray);
        item->setText(name + tr(" (Inactive)"));
    }
    
    ui.listWidget_Moderators->addItem(item);
}
```

#### 6.5 Testing

1. Open wiki group dialog as admin
2. Verify moderator section is visible
3. Click "Add Moderator"
4. Select a GXS ID
5. Verify moderator appears in list
6. Select a moderator
7. Click "Remove Moderator"
8. Verify moderator is removed
9. Test as non-admin (buttons should be disabled)
10. Verify moderator permissions work in practice

### Reference Implementation

See `GxsForumGroupDialog` for similar moderator management pattern.

---

## Summary

All four implementation guides are now complete:

1. **Todo 1:** WikiEditDialog async conversion - Remove TokenQueue, use RsThread::async
2. **Todo 3:** Content merging integration - Use getSnapshotsContent() API, implement merge logic
3. **Todo 4:** MainWindow notifications - Add Wiki enum, wire notification counter
4. **Todo 6:** Moderator UI - Add list widget, add/remove buttons, connect to backend

Each guide includes:
- Current state assessment
- Step-by-step implementation instructions
- Code examples
- Testing procedures
- References to existing implementations

**All backend APIs are ready and documented. GUI work can proceed independently.**
