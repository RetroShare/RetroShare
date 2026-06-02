#include "gui/msgs/EventDialog.h"
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

EventDialog::EventDialog(const QString& eventId, const QDateTime& startInfo, QWidget* parent)
    : QDialog(parent), mEventId(eventId), mDefaultStart(startInfo)
{
    setWindowTitle(mEventId.isEmpty() ? tr("New Event") : tr("Edit Event"));
    setMinimumSize(500, 600);

    buildUi();
    loadEvent();
}

EventDialog::~EventDialog() {}

void EventDialog::buildUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Top action bar (Save, Close, Delete)
    QHBoxLayout* actionLayout = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton(tr("Save and Close"), this);
    saveBtn->setIcon(QIcon(":/icons/mail/compose.png"));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(onSaveAndClose()));
    actionLayout->addWidget(saveBtn);

    QPushButton* inviteBtn = new QPushButton(tr("Invite Attendees"), this);
    connect(inviteBtn, SIGNAL(clicked()), this, SLOT(onInviteAttendees()));
    actionLayout->addWidget(inviteBtn);

    QPushButton* deleteBtn = new QPushButton(tr("Delete"), this);
    deleteBtn->setIcon(QIcon(":/icons/mail/delete.png"));
    connect(deleteBtn, SIGNAL(clicked()), this, SLOT(onDelete()));
    actionLayout->addWidget(deleteBtn);

    if (mEventId.isEmpty()) {
        deleteBtn->setEnabled(false);
    }

    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

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
    QMap<QString, QString> contacts = CalendarData::getContacts();
    for (auto it = contacts.begin(); it != contacts.end(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(it.value(), mAttendeesList);
        item->setData(Qt::UserRole, it.key());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    tabWidget->addTab(mAttendeesList, tr("Attendees"));

    // Attachments Tab
    QWidget* attachTab = new QWidget(this);
    QVBoxLayout* attachLayout = new QVBoxLayout(attachTab);
    mAttachmentsList = new QListWidget(this);
    attachLayout->addWidget(mAttachmentsList);
    QPushButton* addAttachBtn = new QPushButton(tr("Attach File..."), this);
    connect(addAttachBtn, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Select File"));
        if (!file.isEmpty()) {
            mAttachmentsList->addItem(QFileInfo(file).fileName());
        }
    });
    attachLayout->addWidget(addAttachBtn);
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
            for (int i = 0; i < mAttendeesList->count(); ++i) {
                QListWidgetItem* item = mAttendeesList->item(i);
                QString contactId = item->data(Qt::UserRole).toString();
                if (ev.attendees.contains(contactId)) {
                    item->setCheckState(Qt::Checked);
                } else {
                    item->setCheckState(Qt::Unchecked);
                }
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
    // Just switches to attendees tab
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
