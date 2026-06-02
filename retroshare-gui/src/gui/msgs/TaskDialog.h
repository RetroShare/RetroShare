#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include "gui/msgs/CalendarData.h"

class QComboBox;
class QLineEdit;
class QCheckBox;
class QDateTimeEdit;
class QTextEdit;
class QSpinBox;
class QListWidget;

class TaskDialog : public QDialog {
    Q_OBJECT
public:
    TaskDialog(const QString& taskId = "", QWidget* parent = nullptr);
    ~TaskDialog();

private slots:
    void onSaveAndClose();
    void onDelete();
    void onStartToggled(bool checked);
    void onDueToggled(bool checked);

private:
    void loadTask();
    void buildUi();

    QString mTaskId;

    QComboBox* mCalendarCombo;
    QLineEdit* mTitleEdit;
    QLineEdit* mLocationEdit;
    QComboBox* mCategoryCombo;
    QCheckBox* mStartCheck;
    QDateTimeEdit* mStartEdit;
    QCheckBox* mDueCheck;
    QDateTimeEdit* mDueEdit;
    QComboBox* mStatusCombo;
    QSpinBox* mPercentSpin;
    QComboBox* mRepeatCombo;
    QComboBox* mReminderCombo;
    QTextEdit* mDescriptionEdit;
    QListWidget* mAttachmentsList;
};

#endif // TASKDIALOG_H
