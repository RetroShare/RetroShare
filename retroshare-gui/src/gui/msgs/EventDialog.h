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
    EventDialog(const QString& eventId = "", const QDateTime& startInfo = QDateTime::currentDateTime(), QWidget* parent = nullptr, bool readOnly = false);
    ~EventDialog();

private slots:
    void onSaveAndClose();
    void onDelete();
    void onAllDayToggled(bool checked);
    void onInviteAttendees();
    void onEditClicked();

private:
    void loadEvent();
    void buildUi();
    void updateModeUi();

    QString mEventId;
    QDateTime mDefaultStart;
    bool mReadOnly;

    QWidget* mActionWidget;
    QPushButton* mSaveBtn;
    QPushButton* mInviteBtn;
    QPushButton* mDeleteBtn;

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
    QPushButton* mAddAttachBtn;

    QWidget* mBottomButtonsWidget;
    QPushButton* mEditBtn;
    QPushButton* mCloseBtn;

    QCheckBox* mNotifyCheck;
    QCheckBox* mSeparateCheck;
    QCheckBox* mDisallowCheck;
};

#endif // EVENTDIALOG_H
