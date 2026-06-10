#include "gui/msgs/EventDialog.h"
#include <retroshare/rsgxscalendar.h>
#include "retroshare/rsgxsflags.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include <QPushButton>
#include <QTabWidget>
#include <QMessageBox>
#include <QUuid>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMenu>
#include <QDir>
#include "gui/RetroShareLink.h"
#include "gui/common/FriendSelectionWidget.h"
#include <QDialogButtonBox>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmail.h>
#include "gui/common/PeerDefs.h"
#include <QCoreApplication>
#include "gui/common/AvatarDefs.h"
#include <QPixmap>
#include <QIcon>

namespace {
QString getContactName(const QString& idStr) {
    std::string str = idStr.toStdString();
    if (str.length() == 16) {
        RsPgpId pgpId(str);
        QString name;
        PeerDefs::rsidFromId(pgpId, &name);
        return name;
    } else if (str.length() == 32) {
        RsGxsId gxsId(str);
        RsIdentityDetails details;
        if (rsIdentity && rsIdentity->getIdDetails(gxsId, details)) {
            return QString::fromUtf8(details.mNickname.c_str());
        }
        RsPeerId peerId(str);
        std::string peerName = rsPeers ? rsPeers->getPeerName(peerId) : "";
        if (!peerName.empty()) {
            return QString::fromUtf8(peerName.c_str());
        }
        QString name;
        PeerDefs::rsidFromId(peerId, &name);
        if (name != QCoreApplication::translate("PeerDefs", "Unknown")) {
            return name;
        }
        PeerDefs::rsidFromId(gxsId, &name);
        return name;
    }
    return idStr;
}

QIcon getContactAvatar(const QString& idStr) {
    std::string str = idStr.toStdString();
    QPixmap pixmap;
    if (str.length() == 16) {
        AvatarDefs::getAvatarFromGpgId(RsPgpId(str), pixmap);
    } else if (str.length() == 32) {
        RsGxsId gxsId(str);
        RsIdentityDetails details;
        if (rsIdentity && rsIdentity->getIdDetails(gxsId, details)) {
            AvatarDefs::getAvatarFromGxsId(gxsId, pixmap);
        } else {
            AvatarDefs::getAvatarFromSslId(RsPeerId(str), pixmap);
        }
    }
    if (pixmap.isNull()) {
        pixmap = QPixmap(AVATAR_DEFAULT_IMAGE_SQUARE);
    }
    return QIcon(pixmap);
}
}

EventDialog::EventDialog(const QString& eventId, const QDateTime& startInfo, QWidget* parent, bool readOnly)
    : QDialog(parent), mEventId(eventId), mDefaultStart(startInfo), mReadOnly(readOnly)
{
    setMinimumSize(500, 600);

    buildUi();
    loadEvent();
    updateModeUi();
}

EventDialog::~EventDialog() {}

void EventDialog::buildUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Top action bar (Save, Invite, Delete)
    mActionWidget = new QWidget(this);
    QHBoxLayout* actionLayout = new QHBoxLayout(mActionWidget);
    actionLayout->setContentsMargins(0, 0, 0, 0);

    mSaveBtn = new QPushButton(tr("Save and Close"), this);
    mSaveBtn->setIcon(QIcon(":/icons/mail/compose.png"));
    connect(mSaveBtn, SIGNAL(clicked()), this, SLOT(onSaveAndClose()));
    actionLayout->addWidget(mSaveBtn);

    mInviteBtn = new QPushButton(tr("Invite Attendees"), this);
    connect(mInviteBtn, SIGNAL(clicked()), this, SLOT(onInviteAttendees()));
    actionLayout->addWidget(mInviteBtn);

    mDeleteBtn = new QPushButton(tr("Delete"), this);
    mDeleteBtn->setIcon(QIcon(":/icons/mail/delete.png"));
    connect(mDeleteBtn, SIGNAL(clicked()), this, SLOT(onDelete()));
    actionLayout->addWidget(mDeleteBtn);

    if (mEventId.isEmpty()) {
        mDeleteBtn->setEnabled(false);
    }

    actionLayout->addStretch();
    mainLayout->addWidget(mActionWidget);

    // Form inputs layout
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setSpacing(8);

    mCalendarCombo = new QComboBox(this);
    const QList<CalendarInfo>& cals = CalendarData::instance()->getCalendars();
    for (const auto& cal : cals) {
        mCalendarCombo->addItem(cal.name, cal.id);
    }
    formLayout->addRow(tr("Calendar:"), mCalendarCombo);

    mTitleEdit = new QLineEdit(this);
    mTitleEdit->setPlaceholderText(tr("Event Title"));
    formLayout->addRow(tr("Title:"), mTitleEdit);

    mLocationEdit = new QLineEdit(this);
    mLocationEdit->setPlaceholderText(tr("Location"));
    formLayout->addRow(tr("Location:"), mLocationEdit);

    mCategoryCombo = new QComboBox(this);
    mCategoryCombo->addItems({tr("None"), tr("Meeting"), tr("Work"), tr("Birthday"), tr("Holiday"), tr("Personal")});
    formLayout->addRow(tr("Category:"), mCategoryCombo);

    mAllDayCheck = new QCheckBox(tr("All day Event"), this);
    connect(mAllDayCheck, SIGNAL(toggled(bool)), this, SLOT(onAllDayToggled(bool)));
    formLayout->addRow(QString(), mAllDayCheck);

    mStartEdit = new QDateTimeEdit(mDefaultStart, this);
    mStartEdit->setCalendarPopup(true);
    formLayout->addRow(tr("Start:"), mStartEdit);

    mEndEdit = new QDateTimeEdit(mDefaultStart.addSecs(3600), this);
    mEndEdit->setCalendarPopup(true);
    formLayout->addRow(tr("End:"), mEndEdit);

    mRepeatCombo = new QComboBox(this);
    mRepeatCombo->addItems({tr("Does not repeat"), tr("Daily"), tr("Weekly"), tr("Monthly"), tr("Yearly")});
    formLayout->addRow(tr("Repeat:"), mRepeatCombo);

    mReminderCombo = new QComboBox(this);
    mReminderCombo->addItems({tr("No reminder"), tr("5 minutes before"), tr("15 minutes before"), tr("1 hour before"), tr("1 day before")});
    formLayout->addRow(tr("Reminder:"), mReminderCombo);

    mainLayout->addLayout(formLayout);

    // Tab Widget for Description & Attendees & Attachments
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Description Tab
    mDescriptionEdit = new QTextEdit(this);
    tabWidget->addTab(mDescriptionEdit, tr("Description"));

    // Attendees Tab
    mAttendeesList = new QTreeWidget(this);
    mAttendeesList->setHeaderHidden(true);
    mAttendeesList->setIconSize(QSize(32, 32));
    mAttendeesList->setRootIsDecorated(false);
    tabWidget->addTab(mAttendeesList, tr("Attendees"));

    // Attachments Tab
    QWidget* attachTab = new QWidget(this);
    QVBoxLayout* attachLayout = new QVBoxLayout(attachTab);
    mAttachmentsList = new QListWidget(this);
    mAttachmentsList->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(mAttachmentsList, &QListWidget::customContextMenuRequested, [this](const QPoint& pos) {
        QListWidgetItem* item = mAttachmentsList->itemAt(pos);
        if (!item) return;

        QMenu menu(this);
        QAction* downloadAction = menu.addAction(QIcon(":/icons/png/download.png"), tr("Download"));
        QAction* downloadAllAction = menu.addAction(QIcon(":/icons/mail/downloadall.png"), tr("Download all"));
        QAction* removeAction = nullptr;

        if (!mReadOnly) {
            menu.addSeparator();
            removeAction = menu.addAction(QIcon(":/icons/mail/delete.png"), tr("Remove Attachment"));
        }

        QAction* selectedAction = menu.exec(mAttachmentsList->mapToGlobal(pos));
        if (selectedAction == downloadAction) {
            QString att = item->data(Qt::UserRole).toString();
            if (!att.isEmpty()) {
                RetroShareLink link(att);
                if (link.valid() && (link.type() == RetroShareLink::TYPE_FILE || link.type() == RetroShareLink::TYPE_FILE_TREE)) {
                    QList<RetroShareLink> links;
                    links.append(link);
                    RetroShareLink::process(links);
                } else if (QFileInfo::exists(att)) {
                    QString targetPath = QFileDialog::getSaveFileName(this, tr("Save Attachment As"), QFileInfo(att).fileName());
                    if (!targetPath.isEmpty()) {
                        if (QFile::exists(targetPath)) {
                            QFile::remove(targetPath);
                        }
                        if (!QFile::copy(att, targetPath)) {
                            QMessageBox::warning(this, tr("Download Failed"), tr("Could not save the file to %1").arg(targetPath));
                        }
                    }
                } else {
                    QUrl url(att);
                    if (!url.scheme().isEmpty()) {
                        QDesktopServices::openUrl(url);
                    }
                }
            }
        } else if (selectedAction == downloadAllAction) {
            QList<RetroShareLink> rsLinks;
            QStringList localFiles;
            for (int i = 0; i < mAttachmentsList->count(); ++i) {
                QString att = mAttachmentsList->item(i)->data(Qt::UserRole).toString();
                if (att.isEmpty()) continue;
                RetroShareLink link(att);
                if (link.valid() && (link.type() == RetroShareLink::TYPE_FILE || link.type() == RetroShareLink::TYPE_FILE_TREE)) {
                    rsLinks.append(link);
                } else if (QFileInfo::exists(att)) {
                    localFiles.append(att);
                } else {
                    QUrl url(att);
                    if (!url.scheme().isEmpty()) {
                        QDesktopServices::openUrl(url);
                    }
                }
            }
            if (!rsLinks.isEmpty()) {
                RetroShareLink::process(rsLinks);
            }
            if (!localFiles.isEmpty()) {
                QString targetDir = QFileDialog::getExistingDirectory(this, tr("Select Directory to Save Attachments"));
                if (!targetDir.isEmpty()) {
                    bool success = true;
                    QStringList failedFiles;
                    for (const QString& file : localFiles) {
                        QFileInfo fi(file);
                        QString targetPath = QDir(targetDir).filePath(fi.fileName());
                        if (QFile::exists(targetPath)) {
                            QFile::remove(targetPath);
                        }
                        if (!QFile::copy(file, targetPath)) {
                            success = false;
                            failedFiles.append(fi.fileName());
                        }
                    }
                    if (!success) {
                        QMessageBox::warning(this, tr("Download Failed"), tr("Could not save the following files: %1").arg(failedFiles.join(", ")));
                    }
                }
            }
        } else if (removeAction && selectedAction == removeAction) {
            delete mAttachmentsList->takeItem(mAttachmentsList->row(item));
        }
    });

    connect(mAttachmentsList, &QListWidget::itemDoubleClicked, [](QListWidgetItem* item) {
        QString pathOrUrl = item->data(Qt::UserRole).toString();
        if (!pathOrUrl.isEmpty()) {
            QUrl url(pathOrUrl);
            if (url.scheme().isEmpty()) {
                url = QUrl::fromLocalFile(pathOrUrl);
            }
            QDesktopServices::openUrl(url);
        }
    });

    attachLayout->addWidget(mAttachmentsList);
    mAddAttachBtn = new QPushButton(tr("Attach File..."), this);
    connect(mAddAttachBtn, &QPushButton::clicked, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Select File(s)"));
        for (const QString& file : files) {
            if (!file.isEmpty()) {
                QListWidgetItem* item = new QListWidgetItem(QFileInfo(file).fileName(), mAttachmentsList);
                item->setData(Qt::UserRole, file);
                item->setToolTip(file);
            }
        }
    });
    attachLayout->addWidget(mAddAttachBtn);
    tabWidget->addTab(attachTab, tr("Attachments"));

    mainLayout->addWidget(tabWidget);

    // Bottom Options Checkboxes
    QHBoxLayout* bottomCheckLayout = new QHBoxLayout();
    mNotifyCheck = new QCheckBox(tr("Notify attendees"), this);
    mNotifyCheck->setChecked(true);
    bottomCheckLayout->addWidget(mNotifyCheck);

    mSeparateCheck = new QCheckBox(tr("Separate invitation per attendee"), this);
    bottomCheckLayout->addWidget(mSeparateCheck);

    mDisallowCheck = new QCheckBox(tr("Disallow counter"), this);
    bottomCheckLayout->addWidget(mDisallowCheck);

    mainLayout->addLayout(bottomCheckLayout);

    // Bottom Buttons (Close & Edit for read-only view mode)
    mBottomButtonsWidget = new QWidget(this);
    QHBoxLayout* bottomBtnLayout = new QHBoxLayout(mBottomButtonsWidget);
    bottomBtnLayout->setContentsMargins(0, 0, 0, 0);
    bottomBtnLayout->addStretch();

    mEditBtn = new QPushButton(tr("Edit"), this);
    mEditBtn->setStyleSheet("background-color: #0078d4; color: white; border: none; border-radius: 4px; padding: 6px 16px; font-weight: bold;");
    connect(mEditBtn, SIGNAL(clicked()), this, SLOT(onEditClicked()));
    bottomBtnLayout->addWidget(mEditBtn);

    mCloseBtn = new QPushButton(tr("Close"), this);
    connect(mCloseBtn, SIGNAL(clicked()), this, SLOT(reject()));
    bottomBtnLayout->addWidget(mCloseBtn);

    mainLayout->addWidget(mBottomButtonsWidget);
}

void EventDialog::loadEvent() {
    if (mEventId.isEmpty()) {
        return;
    }

    const QList<CalendarEvent>& events = CalendarData::instance()->getEvents();
    for (const auto& ev : events) {
        if (ev.id == mEventId) {
            // Find calendar index
            int calIdx = mCalendarCombo->findData(ev.calendarId);
            if (calIdx != -1) mCalendarCombo->setCurrentIndex(calIdx);

            mTitleEdit->setText(ev.title);
            mLocationEdit->setText(ev.location);
            mCategoryCombo->setCurrentText(ev.category);
            mAllDayCheck->setChecked(ev.allDay);
            mStartEdit->setDateTime(ev.start);
            mEndEdit->setDateTime(ev.end);
            mRepeatCombo->setCurrentText(ev.repeat);
            mReminderCombo->setCurrentText(ev.reminder);
            mDescriptionEdit->setPlainText(ev.description);

            // Set attendees
            mAttendeesList->clear();
            for (const auto& contactId : ev.attendees) {
                if (contactId.length() == 16) {
                    QString name = getContactName(contactId);
                    QTreeWidgetItem* item = new QTreeWidgetItem(mAttendeesList);
                    item->setIcon(0, getContactAvatar(contactId));
                    item->setText(0, name);
                    item->setData(0, Qt::UserRole, contactId);
                    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                    item->setCheckState(0, Qt::Checked);
                } else if (contactId.length() == 32) {
                    RsGxsId gxsId(contactId.toStdString());
                    RsPeerId peerId(contactId.toStdString());
                    bool isSsl = false;
                    if (rsPeers) {
                        std::string peerName = rsPeers->getPeerName(peerId);
                        if (!peerName.empty() || rsPeers->isFriend(peerId)) {
                            isSsl = true;
                        }
                    }
                    if (isSsl) {
                        QString name = getContactName(contactId);
                        QTreeWidgetItem* item = new QTreeWidgetItem(mAttendeesList);
                        item->setIcon(0, getContactAvatar(contactId));
                        item->setText(0, name);
                        item->setData(0, Qt::UserRole, contactId);
                        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                        item->setCheckState(0, Qt::Checked);
                    } else {
                        // Treat as GXS ID
                        GxsIdRSTreeWidgetItem* item = new GxsIdRSTreeWidgetItem(nullptr, GxsIdDetails::ICON_TYPE_AVATAR, true, mAttendeesList);
                        item->setData(0, Qt::UserRole, contactId);
                        item->setId(gxsId, 0, true);
                        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                        item->setCheckState(0, Qt::Checked);
                    }
                }
            }

            // Load attachments
            mAttachmentsList->clear();
            for (const auto& att : ev.attachments) {
                QListWidgetItem* item = new QListWidgetItem(QFileInfo(att).fileName(), mAttachmentsList);
                item->setData(Qt::UserRole, att);
                item->setToolTip(att);
            }
            break;
        }
    }
}

void EventDialog::onAllDayToggled(bool checked) {
    if (checked) {
        mStartEdit->setDisplayFormat("yyyy-MM-dd");
        mEndEdit->setDisplayFormat("yyyy-MM-dd");
    } else {
        mStartEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
        mEndEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    }
}

void EventDialog::onInviteAttendees() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Invite Attendees"));
    dialog.resize(this->width(), 500);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QComboBox* filterCombo = new QComboBox(&dialog);
    filterCombo->addItem(tr("All people"));
    filterCombo->addItem(tr("My contacts"));
#ifdef RS_DIRECT_CHAT
    filterCombo->addItem(tr("Friend Nodes"));
#endif
    filterCombo->setCurrentIndex(0);

    FriendSelectionWidget* friendsWidget = new FriendSelectionWidget(&dialog);
    friendsWidget->setHeaderText(tr("Select contacts to invite:"));
    friendsWidget->setModus(FriendSelectionWidget::MODUS_CHECK);
    friendsWidget->setShowType(FriendSelectionWidget::SHOW_GXS);
    friendsWidget->start();

    connect(filterCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [friendsWidget](int index) {
        switch (index) {
        default:
        case 0:
            friendsWidget->setShowType(FriendSelectionWidget::SHOW_GXS);
            break;
        case 1:
            friendsWidget->setShowType(FriendSelectionWidget::SHOW_CONTACTS);
            break;
#ifdef RS_DIRECT_CHAT
        case 2:
            friendsWidget->setShowType(FriendSelectionWidget::SHOW_SSL);
            break;
#endif
        }
    });

    // Pre-select current attendees
    std::set<std::string> psidsGpg;
    std::set<std::string> psidsGxs;
    std::set<std::string> psidsSsl;
    for (int i = 0; i < mAttendeesList->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = mAttendeesList->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            std::string idStr = item->data(0, Qt::UserRole).toString().toStdString();
            if (idStr.length() == 16) {
                psidsGpg.insert(idStr);
            } else if (idStr.length() == 32) {
                RsPeerId peerId(idStr);
                bool isSsl = false;
                if (rsPeers) {
                    std::string peerName = rsPeers->getPeerName(peerId);
                    if (!peerName.empty() || rsPeers->isFriend(peerId)) {
                        isSsl = true;
                    }
                }
                if (isSsl) {
                    psidsSsl.insert(idStr);
                } else {
                    psidsGxs.insert(idStr);
                }
            }
        }
    }
    friendsWidget->setSelectedIdsFromString(FriendSelectionWidget::IDTYPE_GPG, psidsGpg, false);
    friendsWidget->setSelectedIdsFromString(FriendSelectionWidget::IDTYPE_GXS, psidsGxs, false);
    friendsWidget->setSelectedIdsFromString(FriendSelectionWidget::IDTYPE_SSL, psidsSsl, false);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(filterCombo);
    layout->addWidget(friendsWidget);
    layout->addWidget(buttonBox);

    while (dialog.exec() == QDialog::Accepted) {
        std::set<RsPgpId> selectedGpg;
        friendsWidget->selectedIds<RsPgpId, FriendSelectionWidget::IDTYPE_GPG>(selectedGpg, false);

        std::set<RsGxsId> selectedGxs;
        friendsWidget->selectedIds<RsGxsId, FriendSelectionWidget::IDTYPE_GXS>(selectedGxs, false);

        std::set<RsPeerId> selectedSsl;
        friendsWidget->selectedIds<RsPeerId, FriendSelectionWidget::IDTYPE_SSL>(selectedSsl, false);

        int totalCount = 0;
        for (const auto& id : selectedGpg) {
            if (QString::fromStdString(id.toStdString()) != "0000000000000000") totalCount++;
        }
        for (const auto& id : selectedGxs) {
            if (QString::fromStdString(id.toStdString()) != "00000000000000000000000000000000") totalCount++;
        }
        for (const auto& id : selectedSsl) {
            if (QString::fromStdString(id.toStdString()) != "00000000000000000000000000000000") totalCount++;
        }

        if (totalCount > 20) {
            QMessageBox::warning(this, tr("Limit Exceeded"), tr("You can select a maximum of 20 attendees. Currently selected: %1").arg(totalCount));
            continue;
        }

        mAttendeesList->clear();

        for (const auto& pgpId : selectedGpg) {
            QString pgpIdStr = QString::fromStdString(pgpId.toStdString());
            if (pgpIdStr == "0000000000000000") continue;
            QString name = getContactName(pgpIdStr);
            QTreeWidgetItem* item = new QTreeWidgetItem(mAttendeesList);
            item->setIcon(0, getContactAvatar(pgpIdStr));
            item->setText(0, name);
            item->setData(0, Qt::UserRole, pgpIdStr);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, Qt::Checked);
        }

        for (const auto& gxsId : selectedGxs) {
            QString gxsIdStr = QString::fromStdString(gxsId.toStdString());
            if (gxsIdStr == "00000000000000000000000000000000") continue;
            GxsIdRSTreeWidgetItem* item = new GxsIdRSTreeWidgetItem(nullptr, GxsIdDetails::ICON_TYPE_AVATAR, true, mAttendeesList);
            item->setData(0, Qt::UserRole, gxsIdStr);
            item->setId(gxsId, 0, true);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, Qt::Checked);
        }

        for (const auto& sslId : selectedSsl) {
            QString sslIdStr = QString::fromStdString(sslId.toStdString());
            if (sslIdStr == "00000000000000000000000000000000") continue;
            QString name = getContactName(sslIdStr);
            QTreeWidgetItem* item = new QTreeWidgetItem(mAttendeesList);
            item->setIcon(0, getContactAvatar(sslIdStr));
            item->setText(0, name);
            item->setData(0, Qt::UserRole, sslIdStr);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, Qt::Checked);
        }
        break;
    }

    // Switch to attendees tab
    QTabWidget* tabWidget = findChild<QTabWidget*>();
    if (tabWidget) {
        tabWidget->setCurrentIndex(1); // Attendees is index 1
    }
}

void EventDialog::onSaveAndClose() {
    if (mTitleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Empty Title"), tr("Please provide a title for the event."));
        return;
    }

    CalendarEvent ev;
    ev.id = mEventId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : mEventId;
    ev.calendarId = mCalendarCombo->currentData().toString();
    ev.title = mTitleEdit->text().trimmed();
    ev.location = mLocationEdit->text().trimmed();
    ev.category = mCategoryCombo->currentText();
    ev.allDay = mAllDayCheck->isChecked();
    ev.start = mStartEdit->dateTime();
    ev.end = mEndEdit->dateTime();
    ev.repeat = mRepeatCombo->currentText();
    ev.reminder = mReminderCombo->currentText();
    ev.description = mDescriptionEdit->toPlainText();
    ev.isPublic = true;

    // Get checked attendees
    QStringList invitedNames;
    for (int i = 0; i < mAttendeesList->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = mAttendeesList->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            ev.attendees.append(item->data(0, Qt::UserRole).toString());
            invitedNames.append(item->text(0));
        }
    }

    if (ev.attendees.size() > 20) {
        QMessageBox::warning(this, tr("Limit Exceeded"), tr("You can select a maximum of 20 attendees. Please uncheck some."));
        return;
    }

    // Get attachments
    for (int i = 0; i < mAttachmentsList->count(); ++i) {
        ev.attachments.append(mAttachmentsList->item(i)->data(Qt::UserRole).toString());
    }

    if (mEventId.isEmpty()) {
        CalendarData::instance()->addEvent(ev);
    } else {
        CalendarData::instance()->updateEvent(ev);
    }

    // Send actual invitations
    if (mNotifyCheck->isChecked() && !invitedNames.isEmpty()) {
        sendInvite(ev, invitedNames);
    }

    accept();
}

void EventDialog::onDelete() {
    if (mEventId.isEmpty()) return;

    if (QMessageBox::question(this, tr("Delete Event"), tr("Are you sure you want to delete this event?")) == QMessageBox::Yes) {
        CalendarData::instance()->removeEvent(mEventId);
        accept();
    }
}

void EventDialog::onEditClicked() {
    mReadOnly = false;
    updateModeUi();
}

void EventDialog::updateModeUi() {
    bool canEdit = false;
    if (mEventId.isEmpty()) {
        canEdit = true;
    } else {
        // Determine if user can edit this event (admin check for shared calendars)
        QString calendarId;
        const auto& events = CalendarData::instance()->getEvents();
        for (const auto& ev : events) {
            if (ev.id == mEventId) {
                calendarId = ev.calendarId;
                break;
            }
        }
        if (!calendarId.isEmpty()) {
            const auto& cals = CalendarData::instance()->getCalendars();
            for (const auto& c : cals) {
                if (c.id == calendarId) {
                    if (!c.onNetwork) {
                        canEdit = true;
                    } else if (rsGxsCalendar) {
                        std::list<RsGroupMetaData> summaries;
                        if (rsGxsCalendar->getCalendarsSummaries(summaries)) {
                            RsGxsGroupId groupId(calendarId.toStdString());
                            for (const auto& meta : summaries) {
                                if (meta.mGroupId == groupId) {
                                    canEdit = IS_GROUP_ADMIN(meta.mSubscribeFlags);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            canEdit = true;
        }
    }

    setWindowTitle(mReadOnly ? tr("View Event") : (mEventId.isEmpty() ? tr("New Event") : tr("Edit Event")));

    // Top action bar is visible only in edit mode
    mActionWidget->setVisible(!mReadOnly);

    // Bottom buttons are visible only in read-only mode
    mBottomButtonsWidget->setVisible(mReadOnly);
    mEditBtn->setVisible(canEdit);

    // Set read-only / enabled state of all fields
    mCalendarCombo->setEnabled(!mReadOnly);
    mTitleEdit->setReadOnly(mReadOnly);
    mLocationEdit->setReadOnly(mReadOnly);
    mCategoryCombo->setEnabled(!mReadOnly);
    mAllDayCheck->setEnabled(!mReadOnly);
    mStartEdit->setReadOnly(mReadOnly);
    mEndEdit->setReadOnly(mReadOnly);
    mRepeatCombo->setEnabled(!mReadOnly);
    mReminderCombo->setEnabled(!mReadOnly);
    mDescriptionEdit->setReadOnly(mReadOnly);
    
    mAttendeesList->setEnabled(!mReadOnly);
    mAddAttachBtn->setVisible(!mReadOnly);
    
    mNotifyCheck->setEnabled(!mReadOnly);
    mSeparateCheck->setEnabled(!mReadOnly);
    mDisallowCheck->setEnabled(!mReadOnly);
}

void EventDialog::sendInvite(const CalendarEvent& ev, const QStringList& invitedNames) {
    bool at_least_one_gxsid = false;
    std::set<Rs::Mail::MsgAddress> destinations;

    for (const auto& contactId : ev.attendees) {
        std::string idStr = contactId.toStdString();
        if (idStr.length() == 16) {
            RsPgpId pgpId(idStr);
            std::list<RsPeerId> sslIds;
            if (rsPeers) {
                rsPeers->getAssociatedSSLIds(pgpId, sslIds);
                for (const auto& sslId : sslIds) {
                    destinations.insert(Rs::Mail::MsgAddress(sslId, Rs::Mail::MsgAddress::AddressMode::MSG_ADDRESS_MODE_TO));
                }
            }
        } else if (idStr.length() == 32) {
            RsPeerId peerId(idStr);
            bool isSsl = false;
            if (rsPeers) {
                std::string peerName = rsPeers->getPeerName(peerId);
                if (!peerName.empty() || rsPeers->isFriend(peerId)) {
                    isSsl = true;
                }
            }
            if (isSsl) {
                destinations.insert(Rs::Mail::MsgAddress(peerId, Rs::Mail::MsgAddress::AddressMode::MSG_ADDRESS_MODE_TO));
            } else {
                destinations.insert(Rs::Mail::MsgAddress(RsGxsId(idStr), Rs::Mail::MsgAddress::AddressMode::MSG_ADDRESS_MODE_TO));
                at_least_one_gxsid = true;
            }
        }
    }

    if (destinations.empty()) {
        return;
    }

    Rs::Mail::MessageInfo mi;
    mi.destinations = destinations;
    mi.title = (tr("Invitation: %1").arg(ev.title)).toUtf8().constData();

    // Construct invitation HTML message body
    QString body;
    body += "<h3>" + tr("You are invited to a calendar event:") + "</h3>";
    body += "<table>";
    body += "<tr><td><b>" + tr("Title:") + "</b></td><td>" + ev.title + "</td></tr>";
    if (!ev.location.isEmpty()) {
        body += "<tr><td><b>" + tr("Location:") + "</b></td><td>" + ev.location + "</td></tr>";
    }
    body += "<tr><td><b>" + tr("Time:") + "</b></td><td>" + ev.start.toString("yyyy-MM-dd hh:mm") + " - " + ev.end.toString("yyyy-MM-dd hh:mm") + "</td></tr>";
    if (!ev.description.isEmpty()) {
        body += "<tr><td><b>" + tr("Description:") + "</b></td><td>" + QString(ev.description).replace("\n", "<br>") + "</td></tr>";
    }
    body += "</table>";
    mi.msg = body.toUtf8().constData();

    if (!at_least_one_gxsid) {
        if (rsPeers) {
            mi.from = Rs::Mail::MsgAddress(rsPeers->getOwnId(), Rs::Mail::MsgAddress::AddressMode::MSG_ADDRESS_MODE_TO);
        }
    } else {
        std::list<RsGxsId> own_ids;
        if (rsIdentity) {
            rsIdentity->getOwnIds(own_ids);
        }
        if (own_ids.empty()) {
            QMessageBox::warning(this, tr("RetroShare"), tr("Please create an identity to sign distant messages, or remove GXS contacts from the attendee list."), QMessageBox::Ok);
            return;
        }
        mi.from = Rs::Mail::MsgAddress(own_ids.front(), Rs::Mail::MsgAddress::AddressMode::MSG_ADDRESS_MODE_TO);
    }

    if (rsMail && rsMail->MessageSend(mi)) {
        QMessageBox::information(this, tr("Invitations Sent"),
                                 tr("Invitations successfully sent to: %1").arg(invitedNames.join(", ")));
    } else {
        QMessageBox::warning(this, tr("Sending Failed"), tr("Failed to send invitations."));
    }
}
