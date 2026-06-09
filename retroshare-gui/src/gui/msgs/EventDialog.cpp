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
    mAttendeesList = new QListWidget(this);
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
            QMap<QString, QString> contacts = CalendarData::getContacts();
            for (const auto& contactId : ev.attendees) {
                QString name = contacts.value(contactId, contactId);
                QListWidgetItem* item = new QListWidgetItem(name, mAttendeesList);
                item->setData(Qt::UserRole, contactId);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Checked);
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
    friendsWidget->setModus(FriendSelectionWidget::MODUS_MULTI);
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
    std::set<std::string> psids;
    for (int i = 0; i < mAttendeesList->count(); ++i) {
        QListWidgetItem* item = mAttendeesList->item(i);
        if (item->checkState() == Qt::Checked) {
            psids.insert(item->data(Qt::UserRole).toString().toStdString());
        }
    }
    friendsWidget->setSelectedIdsFromString(FriendSelectionWidget::IDTYPE_GPG, psids, false);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(filterCombo);
    layout->addWidget(friendsWidget);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        std::set<RsPgpId> selected;
        friendsWidget->selectedIds<RsPgpId, FriendSelectionWidget::IDTYPE_GPG>(selected, false);

        mAttendeesList->clear();
        QMap<QString, QString> contacts = CalendarData::getContacts();
        for (const auto& pgpId : selected) {
            QString pgpIdStr = QString::fromStdString(pgpId.toStdString());
            QString name = contacts.value(pgpIdStr, pgpIdStr);
            QListWidgetItem* item = new QListWidgetItem(name, mAttendeesList);
            item->setData(Qt::UserRole, pgpIdStr);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
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
    for (int i = 0; i < mAttendeesList->count(); ++i) {
        QListWidgetItem* item = mAttendeesList->item(i);
        if (item->checkState() == Qt::Checked) {
            ev.attendees.append(item->data(Qt::UserRole).toString());
            invitedNames.append(item->text());
        }
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

    // Mock invitation mailing
    if (mNotifyCheck->isChecked() && !invitedNames.isEmpty()) {
        QMessageBox::information(this, tr("Invitations Sent"),
                                 tr("Invitations successfully sent to: %1").arg(invitedNames.join(", ")));
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
