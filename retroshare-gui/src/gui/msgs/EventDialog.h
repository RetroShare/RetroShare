/*******************************************************************************
 * retroshare-gui/src/gui/msgs/EventDialog.h                                   *
 *                                                                             *
 * Copyright (C) 2026 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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
class QTreeWidget;
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
    void sendInvite(const CalendarEvent& ev, const QStringList& invitedNames);

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
    QTreeWidget* mAttendeesList;

    QListWidget* mAttachmentsList;
    QPushButton* mAddAttachBtn;

    QWidget* mBottomButtonsWidget;
    QPushButton* mEditBtn;
    QPushButton* mCloseBtn;

    QCheckBox* mNotifyCheck;
};

#endif // EVENTDIALOG_H
