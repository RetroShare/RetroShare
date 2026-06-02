#include "gui/msgs/TaskDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTabWidget>
#include <QMessageBox>
#include <QUuid>
#include <QFileDialog>
#include <QListWidget>
#include <QFileInfo>

TaskDialog::TaskDialog(const QString& taskId, QWidget* parent)
    : QDialog(parent), mTaskId(taskId)
{
    setWindowTitle(mTaskId.isEmpty() ? tr("New Task") : tr("Edit Task"));
    setMinimumSize(450, 550);

    buildUi();
    loadTask();
}

TaskDialog::~TaskDialog() {}

void TaskDialog::buildUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Top action bar
    QHBoxLayout* actionLayout = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton(tr("Save and Close"), this);
    saveBtn->setIcon(QIcon(":/icons/mail/compose.png"));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(onSaveAndClose()));
    actionLayout->addWidget(saveBtn);

    QPushButton* deleteBtn = new QPushButton(tr("Delete"), this);
    deleteBtn->setIcon(QIcon(":/icons/mail/delete.png"));
    connect(deleteBtn, SIGNAL(clicked()), this, SLOT(onDelete()));
    actionLayout->addWidget(deleteBtn);

    if (mTaskId.isEmpty()) {
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
    mTitleEdit->setPlaceholderText(tr("Task Title"));
    formLayout->addRow(tr("Title:"), mTitleEdit);

    mLocationEdit = new QLineEdit(this);
    mLocationEdit->setPlaceholderText(tr("Location"));
    formLayout->addRow(tr("Location:"), mLocationEdit);

    mCategoryCombo = new QComboBox(this);
    mCategoryCombo->addItems({tr("None"), tr("Work"), tr("Personal"), tr("Urgent"), tr("Later")});
    formLayout->addRow(tr("Category:"), mCategoryCombo);

    // Optional Start Date
    QHBoxLayout* startLayout = new QHBoxLayout();
    mStartCheck = new QCheckBox(this);
    mStartEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    mStartEdit->setCalendarPopup(true);
    mStartEdit->setEnabled(false);
    connect(mStartCheck, SIGNAL(toggled(bool)), this, SLOT(onStartToggled(bool)));
    startLayout->addWidget(mStartCheck);
    startLayout->addWidget(mStartEdit);
    formLayout->addRow(tr("Start:"), startLayout);

    // Optional Due Date
    QHBoxLayout* dueLayout = new QHBoxLayout();
    mDueCheck = new QCheckBox(this);
    mDueEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), this);
    mDueEdit->setCalendarPopup(true);
    mDueEdit->setEnabled(false);
    connect(mDueCheck, SIGNAL(toggled(bool)), this, SLOT(onDueToggled(bool)));
    dueLayout->addWidget(mDueCheck);
    dueLayout->addWidget(mDueEdit);
    formLayout->addRow(tr("Due Date:"), dueLayout);

    mStatusCombo = new QComboBox(this);
    mStatusCombo->addItems({tr("Not specified"), tr("Not started"), tr("In progress"), tr("Completed")});
    formLayout->addRow(tr("Status:"), mStatusCombo);

    mPercentSpin = new QSpinBox(this);
    mPercentSpin->setRange(0, 100);
    mPercentSpin->setSuffix("%");
    formLayout->addRow(tr("Complete:"), mPercentSpin);

    mRepeatCombo = new QComboBox(this);
    mRepeatCombo->addItems({tr("Does not repeat"), tr("Daily"), tr("Weekly"), tr("Monthly")});
    formLayout->addRow(tr("Repeat:"), mRepeatCombo);

    mReminderCombo = new QComboBox(this);
    mReminderCombo->addItems({tr("No reminder"), tr("On start date"), tr("On due date")});
    formLayout->addRow(tr("Reminder:"), mReminderCombo);

    mainLayout->addLayout(formLayout);

    // Tab Widget for Description & Attachments
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Description Tab
    mDescriptionEdit = new QTextEdit(this);
    tabWidget->addTab(mDescriptionEdit, tr("Description"));

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
}

void TaskDialog::loadTask() {
    if (mTaskId.isEmpty()) {
        return;
    }

    const QList<CalendarTask>& tasks = CalendarData::instance()->getTasks();
    for (const auto& t : tasks) {
        if (t.id == mTaskId) {
            int calIdx = mCalendarCombo->findData(t.calendarId);
            if (calIdx != -1) mCalendarCombo->setCurrentIndex(calIdx);

            mTitleEdit->setText(t.title);
            mLocationEdit->setText(t.location);
            mCategoryCombo->setCurrentText(t.category);

            mStartCheck->setChecked(t.hasStart);
            if (t.hasStart) mStartEdit->setDateTime(t.start);

            mDueCheck->setChecked(t.hasDue);
            if (t.hasDue) mDueEdit->setDateTime(t.due);

            mStatusCombo->setCurrentText(t.status);
            mPercentSpin->setValue(t.percentComplete);
            mRepeatCombo->setCurrentText(t.repeat);
            mReminderCombo->setCurrentText(t.reminder);
            mDescriptionEdit->setPlainText(t.description);
            break;
        }
    }
}

void TaskDialog::onStartToggled(bool checked) {
    mStartEdit->setEnabled(checked);
}

void TaskDialog::onDueToggled(bool checked) {
    mDueEdit->setEnabled(checked);
}

void TaskDialog::onSaveAndClose() {
    if (mTitleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Empty Title"), tr("Please provide a title for the task."));
        return;
    }

    CalendarTask t;
    t.id = mTaskId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : mTaskId;
    t.calendarId = mCalendarCombo->currentData().toString();
    t.title = mTitleEdit->text().trimmed();
    t.location = mLocationEdit->text().trimmed();
    t.category = mCategoryCombo->currentText();
    t.hasStart = mStartCheck->isChecked();
    t.start = mStartEdit->dateTime();
    t.hasDue = mDueCheck->isChecked();
    t.due = mDueEdit->dateTime();
    t.status = mStatusCombo->currentText();
    t.percentComplete = mPercentSpin->value();
    t.repeat = mRepeatCombo->currentText();
    t.reminder = mReminderCombo->currentText();
    t.description = mDescriptionEdit->toPlainText();
    t.completed = (t.status == tr("Completed") || t.percentComplete == 100);

    if (t.completed && t.percentComplete < 100) {
        t.percentComplete = 100;
    }

    if (mTaskId.isEmpty()) {
        CalendarData::instance()->addTask(t);
    } else {
        CalendarData::instance()->updateTask(t);
    }

    accept();
}

void TaskDialog::onDelete() {
    if (mTaskId.isEmpty()) return;

    if (QMessageBox::question(this, tr("Delete Task"), tr("Are you sure you want to delete this task?")) == QMessageBox::Yes) {
        CalendarData::instance()->removeTask(mTaskId);
        accept();
    }
}
