#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include "gui/msgs/CalendarData.h"

class QComboBox;
class QLineEdit;
class QCheckBox;
class QDateTimeEdit;
class QTextEdit;
class QListWidget;
class QTabWidget;

class EventDialog : public QDialog {
    Q_OBJECT
public:
    // Pass eventId to edit existing event, or empty string to create a new one.
    // If creating a new one, startInfo can specify the default start time.
    EventDialog(const QString& eventId = "", const QDateTime& startInfo = QDateTime::currentDateTime(), QWidget* parent = nullptr);
    ~EventDialog();

private slots:
    void onSaveAndClose();
    void onDelete();
    void onAllDayToggled(bool checked);
    void onInviteAttendees();

private:
    void loadEvent();
    void buildUi();

    QString mEventId;
    QDateTime mDefaultStart;

    QComboBox* mCalendarCombo;
    QLineEdit* mTitleEdit;
    QLineEdit* mLocationEdit;
    QComboBox* mCategoryCombo;
    QCheckBox* mAllDayCheck;
    QDateTimeEdit* mStartEdit;
    QDateTimeEdit* mEndEdit;
    QComboBox* mRepeatCombo;
    QComboBox* mReminderCombo;
    QTextEdit* mDescriptionEdit;
    QListWidget* mAttendeesList;
    QListWidget* mAttachmentsList;

    QCheckBox* mNotifyCheck;
    QCheckBox* mSeparateCheck;
    QCheckBox* mDisallowCheck;
};

#endif // EVENTDIALOG_H
