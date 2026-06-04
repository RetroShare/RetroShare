/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarPropertiesDialog.cpp                    *
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

#include "gui/msgs/CalendarPropertiesDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QStackedWidget>
#include <QColorDialog>
#include <QMessageBox>
#include <QUuid>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxscalendar.h>

CalendarPropertiesDialog::CalendarPropertiesDialog(const QString& calId, QWidget* parent)
    : QDialog(parent), mCalId(calId), mEditMode(!calId.isEmpty()), mSelectedColor(QColor("#4a90e2"))
{
    setupUi();
    loadIdentities();

    if (mEditMode) {
        setWindowTitle(tr("Calendar Properties"));
        // Load existing calendar info
        const auto& cals = CalendarData::instance()->getCalendars();
        CalendarInfo existingCal;
        bool found = false;
        for (const auto& c : cals) {
            if (c.id == mCalId) {
                existingCal = c;
                found = true;
                break;
            }
        }
        if (found) {
            mNameEdit->setText(existingCal.name);
            mSelectedColor = existingCal.color;
            mRemindersCheckBox->setChecked(existingCal.showReminders);
            mRadioNetwork->setChecked(existingCal.onNetwork);
            mRadioComputer->setChecked(!existingCal.onNetwork);
            
            // Try to find the email in the combo box
            int idx = mEmailCombo->findText(existingCal.email);
            if (idx != -1) {
                mEmailCombo->setCurrentIndex(idx);
            } else if (!existingCal.email.isEmpty()) {
                mEmailCombo->addItem(existingCal.email);
                mEmailCombo->setCurrentIndex(mEmailCombo->count() - 1);
            }
        }
        updateColorButton();
        mStackedWidget->setCurrentWidget(mPage2);
    } else {
        setWindowTitle(tr("Create New Calendar"));
        mStackedWidget->setCurrentWidget(mPage1);
    }
}

CalendarPropertiesDialog::~CalendarPropertiesDialog() {}

void CalendarPropertiesDialog::setupUi() {
    // Set fixed size matching the mockup aspect ratio
    setMinimumSize(480, 360);
    resize(500, 380);

    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(15, 15, 15, 15);
    dialogLayout->setSpacing(15);

    mStackedWidget = new QStackedWidget(this);

    // ================= PAGE 1 (Wizard Step 1) =================
    mPage1 = new QWidget(this);
    QVBoxLayout* page1Layout = new QVBoxLayout(mPage1);
    page1Layout->setContentsMargins(5, 5, 5, 5);
    page1Layout->setSpacing(15);

    QLabel* descLabel = new QLabel(
        tr("Your calendar can be stored on your computer or be stored on a server in order to access it remotely or share it with your friends or co-workers."),
        mPage1
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 13px; line-height: 1.4;");
    page1Layout->addWidget(descLabel);

    mRadioComputer = new QRadioButton(tr("On My Computer"), mPage1);
    mRadioComputer->setChecked(true);
    mRadioComputer->setStyleSheet("font-size: 13px; font-weight: bold; padding: 5px;");
    page1Layout->addWidget(mRadioComputer);

    mRadioNetwork = new QRadioButton(tr("On the Network"), mPage1);
    mRadioNetwork->setStyleSheet("font-size: 13px; font-weight: bold; padding: 5px;");
    page1Layout->addWidget(mRadioNetwork);

    mRadioImport = new QRadioButton(tr("Import a calendar from an iCalendar (.ics) file"), mPage1);
    mRadioImport->setStyleSheet("font-size: 13px; font-weight: bold; padding: 5px;");
    page1Layout->addWidget(mRadioImport);

    page1Layout->addStretch();
    mStackedWidget->addWidget(mPage1);

    // ================= PAGE 2 (Wizard Step 2) =================
    mPage2 = new QWidget(this);
    QVBoxLayout* page2Layout = new QVBoxLayout(mPage2);
    page2Layout->setContentsMargins(5, 5, 5, 5);
    page2Layout->setSpacing(15);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight);

    mNameEdit = new QLineEdit(mPage2);
    mNameEdit->setMinimumHeight(26);
    formLayout->addRow(tr("Calendar Name:"), mNameEdit);

    mColorBtn = new QPushButton(mPage2);
    mColorBtn->setFixedWidth(80);
    mColorBtn->setCursor(Qt::PointingHandCursor);
    updateColorButton();
    connect(mColorBtn, SIGNAL(clicked()), this, SLOT(onSelectColor()));
    formLayout->addRow(tr("Colour:"), mColorBtn);

    mRemindersCheckBox = new QCheckBox(tr("Show Reminders"), mPage2);
    mRemindersCheckBox->setChecked(true);
    formLayout->addRow(QString(), mRemindersCheckBox);

    mEmailCombo = new QComboBox(mPage2);
    mEmailCombo->setMinimumHeight(26);
    formLayout->addRow(tr("Email:"), mEmailCombo);

    page2Layout->addLayout(formLayout);
    page2Layout->addStretch();
    mStackedWidget->addWidget(mPage2);

    dialogLayout->addWidget(mStackedWidget);

    // ================= BUTTONS ROW =================
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    mBackBtn = new QPushButton(tr("Back"), this);
    mNextBtn = new QPushButton(tr("Next"), this);
    mCreateOrSaveBtn = new QPushButton(mEditMode ? tr("OK") : tr("Create Calendar"), this);
    mCancelBtn = new QPushButton(tr("Cancel"), this);

    connect(mBackBtn, SIGNAL(clicked()), this, SLOT(onBack()));
    connect(mNextBtn, SIGNAL(clicked()), this, SLOT(onNext()));
    connect(mCreateOrSaveBtn, SIGNAL(clicked()), this, SLOT(onAccept()));
    connect(mCancelBtn, SIGNAL(clicked()), this, SLOT(reject()));

    // Layout buttons correctly
    buttonLayout->addStretch();
    buttonLayout->addWidget(mBackBtn);
    buttonLayout->addWidget(mNextBtn);
    buttonLayout->addWidget(mCreateOrSaveBtn);
    buttonLayout->addWidget(mCancelBtn);
    dialogLayout->addLayout(buttonLayout);

    // Update buttons visibility depending on state
    if (mEditMode) {
        mBackBtn->hide();
        mNextBtn->hide();
    } else {
        mBackBtn->hide();
        mCreateOrSaveBtn->hide();
    }
}

void CalendarPropertiesDialog::updateColorButton() {
    mColorBtn->setStyleSheet(QString(
        "background-color: %1; border: 1px solid #ababab; border-radius: 3px; min-height: 20px;"
    ).arg(mSelectedColor.name()));
}

void CalendarPropertiesDialog::loadIdentities() {
    mEmailCombo->clear();
    QStringList emails;

    if (rsIdentity) {
        std::list<RsGxsId> own_identities;
        rsIdentity->getOwnIds(own_identities);
        for (const auto& id : own_identities) {
            RsIdentityDetails details;
            if (rsIdentity->getIdDetails(id, details)) {
                QString nickname = QString::fromUtf8(details.mNickname.c_str()).trimmed();
                QString gxsId = QString::fromStdString(id.toStdString());
                if (!nickname.isEmpty()) {
                    emails.append(QString("%1 <%1@%2>").arg(nickname).arg(gxsId));
                }
            }
        }
    }

    mEmailCombo->addItems(emails);
}

void CalendarPropertiesDialog::onNext() {
    if (mRadioImport && mRadioImport->isChecked()) {
        accept();
        return;
    }
    mStackedWidget->setCurrentWidget(mPage2);
    mBackBtn->show();
    mCreateOrSaveBtn->show();
    mNextBtn->hide();
}

void CalendarPropertiesDialog::onBack() {
    mStackedWidget->setCurrentWidget(mPage1);
    mBackBtn->hide();
    mCreateOrSaveBtn->hide();
    mNextBtn->show();
}

void CalendarPropertiesDialog::onSelectColor() {
    QColor color = QColorDialog::getColor(mSelectedColor, this, tr("Select Calendar Color"));
    if (color.isValid()) {
        mSelectedColor = color;
        updateColorButton();
    }
}

void CalendarPropertiesDialog::onAccept() {
    QString name = mNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("Please enter a name for the calendar."));
        return;
    }

    CalendarInfo info = getCalendarInfo();

    if (info.onNetwork && rsGxsCalendar) {
        // Extract GXS ID from email
        RsGxsId authorId;
        int idx = info.email.lastIndexOf('@');
        int endIdx = info.email.lastIndexOf('>');
        if (idx != -1 && endIdx != -1 && endIdx > idx) {
            std::string gxsIdStr = info.email.mid(idx + 1, endIdx - idx - 1).toStdString();
            authorId = RsGxsId(gxsIdStr);
        }

        std::string errMsg;
        if (mEditMode) {
            RsGxsGroupId groupId(info.id.toStdString());
            if (rsGxsCalendar->updateCalendar(groupId, info.name.toStdString(), "RetroShare Calendar", authorId, errMsg)) {
                CalendarData::instance()->updateCalendar(info);
            } else {
                QMessageBox::critical(this, tr("GXS Error"), tr("Failed to update network calendar: %1").arg(QString::fromStdString(errMsg)));
                return;
            }
        } else {
            RsGxsGroupId groupId;
            if (rsGxsCalendar->createCalendar(info.name.toStdString(), "RetroShare Calendar", authorId, groupId, errMsg)) {
                info.id = QString::fromStdString(groupId.toStdString());
                rsGxsCalendar->subscribeToCalendar(groupId, true, errMsg);
                CalendarData::instance()->addCalendar(info);
            } else {
                QMessageBox::critical(this, tr("GXS Error"), tr("Failed to create network calendar: %1").arg(QString::fromStdString(errMsg)));
                return;
            }
        }
    } else {
        if (mEditMode) {
            CalendarData::instance()->updateCalendar(info);
        } else {
            CalendarData::instance()->addCalendar(info);
        }
    }

    accept();
}

CalendarInfo CalendarPropertiesDialog::getCalendarInfo() const {
    CalendarInfo info;
    info.id = mCalId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : mCalId;
    info.name = mNameEdit->text().trimmed();
    info.color = mSelectedColor;
    info.onNetwork = mRadioNetwork->isChecked();
    info.isPublic = info.onNetwork;
    info.showReminders = mRemindersCheckBox->isChecked();
    info.email = mEmailCombo->currentText();
    info.owner = "local";
    return info;
}

bool CalendarPropertiesDialog::isImportMode() const {
    return mRadioImport && mRadioImport->isChecked();
}
