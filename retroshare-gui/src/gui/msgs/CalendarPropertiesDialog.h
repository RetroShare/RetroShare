/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarPropertiesDialog.h                      *
 *                                                                             *
 * Copyright (C) 2011 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef CALENDARPROPERTIESDIALOG_H
#define CALENDARPROPERTIESDIALOG_H

#include <QDialog>
#include <QColor>
#include "gui/msgs/CalendarData.h"

class QStackedWidget;
class QRadioButton;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;
class GxsIdChooser;
class GxsCircleChooser;
class GroupChooser;
class QGroupBox;
class QPlainTextEdit;
class QLabel;
class QFormLayout;

class CalendarPropertiesDialog : public QDialog {
    Q_OBJECT
public:
    CalendarPropertiesDialog(const QString& calId = "", QWidget* parent = nullptr);
    ~CalendarPropertiesDialog();

    CalendarInfo getCalendarInfo() const;
    bool isImportMode() const;

private slots:
    void onNext();
    void onBack();
    void onSelectColor();
    void onAccept();
    void updateCircleOptions();

private:
    void setupUi();
    void updateColorButton();
    void updatePage2Layout();

    QString mCalId;
    bool mEditMode;
    QColor mSelectedColor;

    QStackedWidget* mStackedWidget;
    QWidget* mPage1;
    QWidget* mPage2;

    // Page 1 widgets
    QRadioButton* mRadioComputer;
    QRadioButton* mRadioNetwork;
    QRadioButton* mRadioImport;

    // Page 2 widgets
    QFormLayout* mFormLayout;
    QLineEdit* mNameEdit;
    QPushButton* mColorBtn;

    // GXS network calendar widgets
    GxsIdChooser* mIdChooser;
    QGroupBox* mDistribGroupBox;
    QRadioButton* mRadioPublic;
    QRadioButton* mRadioCircle;
    QRadioButton* mRadioNodeGroup;
    GxsCircleChooser* mCircleCombo;
    GroupChooser* mLocalCombo;
    QLabel* mDescLabel;
    QPlainTextEdit* mDescEdit;

    // Buttons
    QPushButton* mNextBtn;
    QPushButton* mBackBtn;
    QPushButton* mCreateOrSaveBtn;
    QPushButton* mCancelBtn;
};

#endif // CALENDARPROPERTIESDIALOG_H
