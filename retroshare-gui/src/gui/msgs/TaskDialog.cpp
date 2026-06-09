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
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QDir>
#include "gui/RetroShareLink.h"

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
    mAttachmentsList->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(mAttachmentsList, &QListWidget::customContextMenuRequested, [this](const QPoint& pos) {
        QListWidgetItem* item = mAttachmentsList->itemAt(pos);
        if (!item) return;

        QMenu menu(this);
        QAction* downloadAction = menu.addAction(QIcon(":/icons/png/download.png"), tr("Download"));
        QAction* downloadAllAction = menu.addAction(QIcon(":/icons/mail/downloadall.png"), tr("Download all"));
        QAction* removeAction = menu.addAction(QIcon(":/icons/mail/delete.png"), tr("Remove Attachment"));

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
        } else if (selectedAction == removeAction) {
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

            // Load attachments
            mAttachmentsList->clear();
            for (const auto& att : t.attachments) {
                QListWidgetItem* item = new QListWidgetItem(QFileInfo(att).fileName(), mAttachmentsList);
                item->setData(Qt::UserRole, att);
                item->setToolTip(att);
            }
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

    // Get attachments
    for (int i = 0; i < mAttachmentsList->count(); ++i) {
        t.attachments.append(mAttachmentsList->item(i)->data(Qt::UserRole).toString());
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
