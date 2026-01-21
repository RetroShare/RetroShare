# Todo 4: MainWindow Notification Counter Integration Guide

## Overview
WikiUserNotify is implemented and ready. This guide provides step-by-step instructions for wiring notification counters into MainWindow when wiki is enabled.

---

## Current Status

**✅ Completed:**
- WikiUserNotify class implemented (`WikiUserNotify.cpp/h`)
- Notification counting logic functional
- Auto-read status marking on page view
- Integrated into WikiDialog via createUserNotify()

**⏳ Pending:**
- MainWindow enum update (add `Wiki` entry)
- MainWindow notification counter display
- Button counter update wiring

---

## Implementation Steps

### 1. Update MainWindow::Page Enum

**File:** `retroshare-gui/src/gui/MainWindow.h`

Add Wiki to the Page enum:
```cpp
enum Page {
    Home = 0,
    Network,
    Friends,
    Search,
    Messages,
    Channels,
    Forums,
    Posted,
    Wiki,      // ADD THIS
    Settings,
    Page_Count  // Keep this last
};
```

### 2. Add Wiki Button to MainWindow

**File:** `retroshare-gui/src/gui/MainWindow.ui`

If not already present, add Wiki button to the navigation:
```xml
<widget class="QToolButton" name="wikiButton">
    <property name="text">
        <string>Wiki</string>
    </property>
    <property name="icon">
        <iconset resource="../images.qrc">
            <normaloff>:/icons/wiki.png</normaloff>:/icons/wiki.png
        </iconset>
    </property>
    <property name="checkable">
        <bool>true</bool>
    </property>
</widget>
```

### 3. Register WikiUserNotify in MainWindow

**File:** `retroshare-gui/src/gui/MainWindow.cpp`

In MainWindow constructor, register the notify:

```cpp
#include "gui/WikiPoos/WikiUserNotify.h"

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
    : RsGxsUpdateBroadcastWidget(parent, flags)
{
    // ... existing code ...
    
    #ifdef ENABLE_WIKIPOOS
    // Register Wiki user notify
    WikiUserNotify *wikiNotify = new WikiUserNotify(rsWiki, this);
    registerUserNotify(wikiNotify);
    mUserNotifyList.push_back(wikiNotify);
    #endif
    
    // ... rest of constructor ...
}
```

### 4. Connect Wiki Button to WikiDialog

**File:** `retroshare-gui/src/gui/MainWindow.cpp`

Wire the button click to show WikiDialog:

```cpp
void MainWindow::initStackedPage()
{
    // ... existing code ...
    
    #ifdef ENABLE_WIKIPOOS
    ui->stackPages->add(wikiDialog = new WikiDialog(ui->stackPages), 
                        createPageAction(QIcon(":/icons/wiki.png"), tr("Wiki"), grp));
    connect(ui->wikiButton, SIGNAL(clicked()), this, SLOT(showWiki()));
    #endif
}

void MainWindow::showWiki()
{
    ui->stackPages->setCurrentWidget(wikiDialog);
    ui->wikiButton->setChecked(true);
}
```

### 5. Update Notification Counter Display

**File:** `retroshare-gui/src/gui/MainWindow.cpp`

In the notification update method:

```cpp
void MainWindow::updateNotificationCount()
{
    // ... existing code for other services ...
    
    #ifdef ENABLE_WIKIPOOS
    // Update wiki button counter
    if (wikiNotify) {
        unsigned int count = wikiNotify->getNewCount();
        if (count > 0) {
            ui->wikiButton->setText(QString("Wiki (%1)").arg(count));
        } else {
            ui->wikiButton->setText("Wiki");
        }
    }
    #endif
}
```

### 6. Clear Counter on Wiki View

**File:** `retroshare-gui/src/gui/WikiPoos/WikiDialog.cpp`

Already implemented in showEvent() and when viewing pages:

```cpp
void WikiDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        // Clear unread status when dialog shown
        if (mUserNotify) {
            mUserNotify->clearNewCount();
        }
    }
    MainPage::showEvent(event);
}
```

Counter also clears automatically when:
- Page is viewed (setMessageReadStatus called)
- Group is opened
- WikiDialog becomes visible

---

## Build Configuration

### Enable Wiki in Build

**File:** `retroshare.pri`

Uncomment or add:
```
CONFIG += wikipoos
```

This will define `ENABLE_WIKIPOOS` and compile wiki code.

---

## Testing Notification Counters

### Test Procedure

1. **Build with wiki enabled:**
   ```bash
   cd retroshare
   qmake
   make
   ```

2. **Subscribe to a wiki group:**
   - Open WikiDialog
   - Subscribe to or create a wiki group

3. **Generate notifications:**
   - Have another user add/edit pages
   - Or edit pages yourself from another node

4. **Verify counter updates:**
   - Check Wiki button shows count: "Wiki (3)"
   - Counter should update automatically via rsEvents

5. **Verify counter clears:**
   - Click Wiki button to open WikiDialog
   - View a page
   - Check counter decrements/clears
   - Close and reopen WikiDialog
   - Counter should remain at 0 for viewed pages

---

## Event Flow

### How Notifications Work

1. **New wiki content published:**
   - p3Wiki generates NEW_SNAPSHOT or NEW_COLLECTION event
   - Event broadcast to all subscribers

2. **WikiUserNotify receives event:**
   - Checks if group is subscribed
   - Checks if message is unread
   - Increments internal counter

3. **MainWindow updates display:**
   - Polls getUserNotifyList periodically
   - Calls getNewCount() on each notify
   - Updates button text with counter

4. **User views content:**
   - WikiDialog marks message as read
   - Calls setMessageReadStatus()
   - WikiUserNotify decrements counter
   - MainWindow updates display to reflect new count

---

## Architecture Notes

### Why WikiUserNotify Uses UserNotify Base

WikiUserNotify inherits from `UserNotify` instead of `GxsUserNotify` because:
- WikiDialog is a `MainPage`, not a `GxsGroupFrameDialog`
- This avoids large architectural refactoring
- Provides standalone notification counting
- Maintains minimal changes principle

### Future Enhancement

For better consistency with other GXS services:
- Refactor WikiDialog to inherit from GxsGroupFrameDialog
- Change WikiUserNotify to inherit from GxsUserNotify
- This would enable automatic counting and stat tracking

But current implementation is fully functional.

---

## Troubleshooting

### Counter Not Updating

**Check:**
1. Wiki is enabled: `CONFIG += wikipoos` in retroshare.pri
2. WikiUserNotify registered in MainWindow constructor
3. Button connected to notification update method
4. rsEvents are being generated (check p3wiki.cc)

### Counter Not Clearing

**Check:**
1. showEvent() calls clearNewCount()
2. setMessageReadStatus() is called when viewing pages
3. WikiUserNotify properly tracking read status

### Build Errors

**Common issues:**
1. Missing `#ifdef ENABLE_WIKIPOOS` guards
2. rsWiki interface not available (check submodule)
3. Qt Creator cache issues (clean and rebuild)

---

## Summary

**What's Implemented:**
✅ WikiUserNotify class with counting logic  
✅ Auto-read status marking  
✅ Integration with WikiDialog  

**What's Needed:**
⏳ MainWindow enum update  
⏳ Button counter display wiring  
⏳ Build with CONFIG += wikipoos  

**Estimated Time:** 1-2 hours for MainWindow integration

All notification infrastructure is ready. Just needs MainWindow wiring when wiki feature is enabled for production use.
