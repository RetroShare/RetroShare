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
#include "gui/gxs/GxsIdChooser.h"
#include "gui/gxs/GxsCircleChooser.h"
#include "gui/common/GroupChooser.h"
#include <retroshare/rsgxscircles.h>
#include <QGroupBox>
#include <QPlainTextEdit>

CalendarPropertiesDialog::CalendarPropertiesDialog(const QString& calId, QWidget* parent)
    : QDialog(parent), mCalId(calId), mEditMode(!calId.isEmpty()), mSelectedColor(QColor("#4a90e2"))
{
    setupUi();

    // Default initialization for GXS choosers
    mIdChooser->loadIds(0, RsGxsId());
    mCircleCombo->loadCircles(RsGxsCircleId());
    mLocalCombo->loadGroups(0, RsNodeGroupId());
    mRadioPublic->setChecked(true);
    updateCircleOptions();

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
            mRadioNetwork->setChecked(existingCal.onNetwork);
            mRadioComputer->setChecked(!existingCal.onNetwork);

            // GXS fields loading if calendar is on network
            RsGxsId authorId;
            int idxGxs = existingCal.email.lastIndexOf('@');
            int endIdxGxs = existingCal.email.lastIndexOf('>');
            if (idxGxs != -1 && endIdxGxs != -1 && endIdxGxs > idxGxs) {
                std::string gxsIdStr = existingCal.email.mid(idxGxs + 1, endIdxGxs - idxGxs - 1).toStdString();
                authorId = RsGxsId(gxsIdStr);
            }
            mIdChooser->loadIds(0, authorId);

            mDescEdit->setPlainText(existingCal.description);

            if (existingCal.circleType == GXS_CIRCLE_TYPE_PUBLIC) {
                mRadioPublic->setChecked(true);
            } else if (existingCal.circleType == GXS_CIRCLE_TYPE_EXTERNAL) {
                mRadioCircle->setChecked(true);
            } else if (existingCal.circleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY) {
                mRadioNodeGroup->setChecked(true);
            } else {
                mRadioPublic->setChecked(true);
            }

            RsGxsCircleId cid(existingCal.circleId.toStdString());
            mCircleCombo->loadCircles(cid);

            RsNodeGroupId ngi(existingCal.internalCircle.toStdString());
            mLocalCombo->loadGroups(0, ngi);

            updateCircleOptions();

            // Disable distribution and description controls for non-admin users
            // owner == "local" means the current user created (and administers) this calendar
            bool isAdmin = (existingCal.owner == "local");
            if (existingCal.onNetwork && !isAdmin) {
                mIdChooser->setEnabled(false);
                mRadioPublic->setEnabled(false);
                mRadioCircle->setEnabled(false);
                mRadioNodeGroup->setEnabled(false);
                mDescEdit->setReadOnly(true);
                mDescEdit->setEnabled(false);
                mNameEdit->setEnabled(false);
            }
        }
        updateColorButton();
        mStackedWidget->setCurrentWidget(mPage2);
        updatePage2Layout();
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
        tr("Your calendar can be stored on your computer or share it with your friends or co-workers."),
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

    mFormLayout = new QFormLayout();
    mFormLayout->setSpacing(12);
    mFormLayout->setLabelAlignment(Qt::AlignRight);

    mNameEdit = new QLineEdit(mPage2);
    mNameEdit->setMinimumHeight(26);
    mFormLayout->addRow(tr("Calendar Name:"), mNameEdit);

    mColorBtn = new QPushButton(mPage2);
    mColorBtn->setFixedWidth(80);
    mColorBtn->setCursor(Qt::PointingHandCursor);
    updateColorButton();
    connect(mColorBtn, SIGNAL(clicked()), this, SLOT(onSelectColor()));
    mFormLayout->addRow(tr("Colour:"), mColorBtn);

    mIdChooser = new GxsIdChooser(mPage2);
    mFormLayout->addRow(tr("Owner:"), mIdChooser);

    page2Layout->addLayout(mFormLayout);

    // Message Distribution group box
    mDistribGroupBox = new QGroupBox(tr("Calendar Distribution"), mPage2);
    QVBoxLayout* distribLayout = new QVBoxLayout(mDistribGroupBox);
    distribLayout->setContentsMargins(10, 10, 10, 10);
    distribLayout->setSpacing(8);

    QHBoxLayout* radioLayout = new QHBoxLayout();
    mRadioPublic = new QRadioButton(tr("Public"), mDistribGroupBox);
    mRadioCircle = new QRadioButton(tr("Restricted to Circle"), mDistribGroupBox);
    mRadioNodeGroup = new QRadioButton(tr("Restricted node group"), mDistribGroupBox);

    mRadioPublic->setChecked(true);
    radioLayout->addWidget(mRadioPublic);
    radioLayout->addWidget(mRadioCircle);
    radioLayout->addWidget(mRadioNodeGroup);
    distribLayout->addLayout(radioLayout);

    mCircleCombo = new GxsCircleChooser(mDistribGroupBox);
    mCircleCombo->setMinimumHeight(26);
    mLocalCombo = new GroupChooser(mDistribGroupBox);
    mLocalCombo->setMinimumHeight(26);

    distribLayout->addWidget(mCircleCombo);
    distribLayout->addWidget(mLocalCombo);

    page2Layout->addWidget(mDistribGroupBox);

    mDescLabel = new QLabel(tr("Description"), mPage2);
    mDescEdit = new QPlainTextEdit(mPage2);
    mDescEdit->setPlaceholderText(tr("Set a descriptive description here"));
    mDescEdit->setMaximumHeight(80);

    page2Layout->addWidget(mDescLabel);
    page2Layout->addWidget(mDescEdit);

    page2Layout->addStretch();
    mStackedWidget->addWidget(mPage2);

    dialogLayout->addWidget(mStackedWidget);

    // ================= BUTTONS ROW =================
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    mBackBtn = new QPushButton(tr("Back"), this);
    mNextBtn = new QPushButton(tr("Next"), this);
    mCreateOrSaveBtn = new QPushButton(mEditMode ? tr("OK") : tr("Create Calendar"), this);
    mCancelBtn = new QPushButton(tr("Cancel"), this);

    connect(mRadioPublic, SIGNAL(clicked()), this, SLOT(updateCircleOptions()));
    connect(mRadioCircle, SIGNAL(clicked()), this, SLOT(updateCircleOptions()));
    connect(mRadioNodeGroup, SIGNAL(clicked()), this, SLOT(updateCircleOptions()));

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

    if (info.onNetwork && rsGxsCalendar && info.owner == "local") {
        // Extract GXS ID from email
        RsGxsId authorId;
        int idx = info.email.lastIndexOf('@');
        int endIdx = info.email.lastIndexOf('>');
        if (idx != -1 && endIdx != -1 && endIdx > idx) {
            std::string gxsIdStr = info.email.mid(idx + 1, endIdx - idx - 1).toStdString();
            authorId = RsGxsId(gxsIdStr);
        }

        std::string errMsg;
        std::string descStr = info.description.toStdString();
        RsGxsCircleId circleId(info.circleId.toStdString());
        RsGxsCircleId internalCircle(info.internalCircle.toStdString());

        if (mEditMode) {
            RsGxsGroupId groupId(info.id.toStdString());
            if (rsGxsCalendar->updateCalendar(groupId, info.name.toStdString(), descStr, authorId, info.circleType, circleId, internalCircle, info.groupFlags, errMsg)) {
                CalendarData::instance()->updateCalendar(info);
            } else {
                QMessageBox::critical(this, tr("GXS Error"), tr("Failed to update network calendar: %1").arg(QString::fromStdString(errMsg)));
                return;
            }
        } else {
            RsGxsGroupId groupId;
            if (rsGxsCalendar->createCalendar(info.name.toStdString(), descStr, authorId, info.circleType, circleId, internalCircle, info.groupFlags, groupId, errMsg)) {
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
    
    // Preserve owner and defaults if in edit mode
    info.owner = "local";
    info.showReminders = true;
    info.email = "";
    if (mEditMode) {
        const auto& cals = CalendarData::instance()->getCalendars();
        for (const auto& c : cals) {
            if (c.id == mCalId) {
                info.owner = c.owner;
                info.showReminders = c.showReminders;
                info.email = c.email;
                break;
            }
        }
    }

    info.circleType = GXS_CIRCLE_TYPE_PUBLIC;
    info.circleId = "";
    info.internalCircle = "";
    info.groupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
    info.description = "";

    if (info.onNetwork) {
        RsGxsId authorId;
        if (mIdChooser->getChosenId(authorId) == GxsIdChooser::KnowId) {
            std::string nickname = "";
            if (rsIdentity) {
                RsIdentityDetails details;
                if (rsIdentity->getIdDetails(authorId, details)) {
                    nickname = details.mNickname;
                }
            }
            if (!nickname.empty()) {
                info.email = QString("%1 <%1@%2>").arg(QString::fromStdString(nickname)).arg(QString::fromStdString(authorId.toStdString()));
            } else {
                info.email = QString("<%1@%1>").arg(QString::fromStdString(authorId.toStdString()));
            }
        } else {
            info.email = "";
        }

        if (mRadioPublic->isChecked()) {
            info.circleType = GXS_CIRCLE_TYPE_PUBLIC;
            info.groupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
        } else if (mRadioCircle->isChecked()) {
            info.circleType = GXS_CIRCLE_TYPE_EXTERNAL;
            RsGxsCircleId cid;
            mCircleCombo->getChosenCircle(cid);
            info.circleId = QString::fromStdString(cid.toStdString());
            info.groupFlags = GXS_SERV::FLAG_PRIVACY_RESTRICTED;
        } else if (mRadioNodeGroup->isChecked()) {
            info.circleType = GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY;
            RsNodeGroupId ngi;
            mLocalCombo->getChosenGroup(ngi);
            info.internalCircle = QString::fromStdString(ngi.toStdString());
            info.groupFlags = GXS_SERV::FLAG_PRIVACY_PRIVATE;
        }
        info.description = mDescEdit->toPlainText();
    }

    return info;
}

bool CalendarPropertiesDialog::isImportMode() const {
    return mRadioImport && mRadioImport->isChecked();
}

void CalendarPropertiesDialog::updateCircleOptions() {
    mCircleCombo->setVisible(mRadioCircle->isChecked());
    mLocalCombo->setVisible(mRadioNodeGroup->isChecked());
}

void CalendarPropertiesDialog::updatePage2Layout() {
    bool onNetwork = mRadioNetwork->isChecked();

    mIdChooser->setVisible(onNetwork);
    if (QWidget* lbl = mFormLayout->labelForField(mIdChooser)) {
        lbl->setVisible(onNetwork);
    }

    mDistribGroupBox->setVisible(onNetwork);
    mDescLabel->setVisible(onNetwork);
    mDescEdit->setVisible(onNetwork);

    updateCircleOptions();
}
