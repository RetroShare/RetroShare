/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QMessageBox>
#include <QDateTime>
#include <QPushButton>

#include "AddFeedDialog.h"
#include "ui_AddFeedDialog.h"
#include "PreviewFeedDialog.h"
#include "FeedReaderStringDefs.h"
#include "gui/settings/rsharesettings.h"

//Todo: Replace with gxs forums
//#include "retroshare/rsforums.h"

//bool sortForumInfo(const ForumInfo& info1, const ForumInfo& info2)
//{
//	return QString::fromStdWString(info1.forumName).compare(QString::fromStdWString(info2.forumName), Qt::CaseInsensitive);
//}

AddFeedDialog::AddFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent)
	: QDialog(parent, Qt::Window), mFeedReader(feedReader), mNotify(notify), ui(new Ui::AddFeedDialog)
{
	ui->setupUi(this);

	connect(ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(createFeed()));
	connect(ui->buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

	connect(ui->useAuthenticationCheckBox, SIGNAL(toggled(bool)), this, SLOT(authenticationToggled()));
	connect(ui->useStandardStorageTimeCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardStorageTimeToggled()));
	connect(ui->useStandardUpdateInterval, SIGNAL(toggled(bool)), this, SLOT(useStandardUpdateIntervalToggled()));
	connect(ui->useStandardProxyCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardProxyToggled()));
	connect(ui->typeForumRadio, SIGNAL(toggled(bool)), this, SLOT(typeForumToggled()));
	connect(ui->previewButton, SIGNAL(clicked()), this, SLOT(preview()));

	/* currently only for loacl feeds */
	connect(ui->saveCompletePageCheckBox, SIGNAL(toggled(bool)), this, SLOT(denyForumToggled()));

	connect(ui->urlLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->useInfoFromFeedCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeLocalRadio, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeForumRadio, SIGNAL(toggled(bool)), this, SLOT(validate()));

	ui->headerFrame->setHeaderText(tr("Feed Details"));
	ui->headerFrame->setHeaderImage(QPixmap(":/images/FeedReader.png"));

	ui->activatedCheckBox->setChecked(true);
	ui->forumComboBox->setEnabled(false);
	ui->useInfoFromFeedCheckBox->setChecked(true);
	ui->updateForumInfoCheckBox->setEnabled(false);
	ui->updateForumInfoCheckBox->setChecked(true);
	ui->forumNameLabel->hide();
	ui->useAuthenticationCheckBox->setChecked(false);
	ui->useStandardStorageTimeCheckBox->setChecked(true);
	ui->useStandardUpdateInterval->setChecked(true);
	ui->useStandardProxyCheckBox->setChecked(true);

	/* not yet supported */
	ui->authenticationGroupBox->setEnabled(false);

	mTransformationType = RS_FEED_TRANSFORMATION_TYPE_NONE;
	ui->transformationTypeLabel->setText(FeedReaderStringDefs::transforationTypeString(mTransformationType));

	/* fill own forums */
	//Todo: Replace with gxs forums
//	std::list<ForumInfo> forumList;
//	if (rsForums->getForumList(forumList)) {
//		forumList.sort(sortForumInfo);
//		for (std::list<ForumInfo>::iterator it = forumList.begin(); it != forumList.end(); ++it) {
//			ForumInfo &forumInfo = *it;
//			/* show only own anonymous forums */
//			if ((forumInfo.subscribeFlags & RS_DISTRIB_ADMIN) && (forumInfo.forumFlags & RS_DISTRIB_AUTHEN_ANON)) {
//				ui->forumComboBox->addItem(QString::fromStdWString(forumInfo.forumName), QString::fromStdString(forumInfo.forumId));
//			}
//		}
//	}
	/* insert item to create a new forum */
	ui->forumComboBox->insertItem(0, tr("Create a new anonymous public forum"), "");
	ui->forumComboBox->setCurrentIndex(0);

	validate();

	ui->urlLineEdit->setFocus();

	/* load settings */
	processSettings(true);

	//Todo: Replace with gxs forums
	ui->typeForumRadio->setEnabled(false);
}

AddFeedDialog::~AddFeedDialog()
{
	/* save settings */
	processSettings(false);

	delete ui;
}

void AddFeedDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("AddFeedDialog"));

	if (load) {
		// load settings
		QByteArray geometry = Settings->value("Geometry").toByteArray();
		if (!geometry.isEmpty()) {
			restoreGeometry(geometry);
		}
	} else {
		// save settings
		Settings->setValue("Geometry", saveGeometry());
	}

	Settings->endGroup();
}

void AddFeedDialog::authenticationToggled()
{
	bool checked = ui->useAuthenticationCheckBox->isChecked();
	ui->userLineEdit->setEnabled(checked);
	ui->passwordLineEdit->setEnabled(checked);
}

void AddFeedDialog::useStandardStorageTimeToggled()
{
	bool checked = ui->useStandardStorageTimeCheckBox->isChecked();
	ui->storageTimeSpinBox->setEnabled(!checked);
}

void AddFeedDialog::useStandardUpdateIntervalToggled()
{
	bool checked = ui->useStandardUpdateInterval->isChecked();
	ui->updateIntervalSpinBox->setEnabled(!checked);
}

void AddFeedDialog::useStandardProxyToggled()
{
	bool checked = ui->useStandardProxyCheckBox->isChecked();
	ui->proxyAddressLineEdit->setEnabled(!checked);
	ui->proxyPortSpinBox->setEnabled(!checked);
}

void AddFeedDialog::typeForumToggled()
{
	bool checked = ui->typeForumRadio->isChecked();
	ui->forumComboBox->setEnabled(checked);
	ui->updateForumInfoCheckBox->setEnabled(checked);
}

void AddFeedDialog::denyForumToggled()
{
	if (ui->saveCompletePageCheckBox->isChecked()) {
		ui->typeForumRadio->setEnabled(false);
		ui->typeLocalRadio->setChecked(true);
	} else {
		ui->typeForumRadio->setEnabled(true);
	}
}

void AddFeedDialog::validate()
{
	bool ok = true;

	if (ui->urlLineEdit->text().isEmpty()) {
		ok = false;
	}
	if (ui->nameLineEdit->text().isEmpty() && !ui->useInfoFromFeedCheckBox->isChecked()) {
		ok = false;
	}

	ui->previewButton->setEnabled(ok);

	if (!ui->typeLocalRadio->isChecked() && !ui->typeForumRadio->isChecked()) {
		ok = false;
	}

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void AddFeedDialog::setParent(const std::string &parentId)
{
	mParentId = parentId;
}

bool AddFeedDialog::fillFeed(const std::string &feedId)
{
	mFeedId = feedId;

	if (!mFeedId.empty()) {
		FeedInfo feedInfo;
		if (!mFeedReader->getFeedInfo(mFeedId, feedInfo)) {
			mFeedId.clear();
			return false;
		}

		setWindowTitle(tr("Edit feed"));
		ui->typeGroupBox->setEnabled(false);

		mParentId = feedInfo.parentId;

		ui->nameLineEdit->setText(QString::fromUtf8(feedInfo.name.c_str()));
		ui->urlLineEdit->setText(QString::fromUtf8(feedInfo.url.c_str()));
		ui->useInfoFromFeedCheckBox->setChecked(feedInfo.flag.infoFromFeed);
		ui->updateForumInfoCheckBox->setChecked(feedInfo.flag.updateForumInfo);
		ui->activatedCheckBox->setChecked(!feedInfo.flag.deactivated);
		ui->embedImagesCheckBox->setChecked(feedInfo.flag.embedImages);
		ui->saveCompletePageCheckBox->setChecked(feedInfo.flag.saveCompletePage);

		ui->descriptionPlainTextEdit->setPlainText(QString::fromUtf8(feedInfo.description.c_str()));

		ui->typeGroupBox->setEnabled(false);
		ui->forumComboBox->hide();
		ui->forumNameLabel->clear();
		ui->forumNameLabel->show();

		if (feedInfo.flag.forum) {
			ui->typeForumRadio->setChecked(true);
			ui->saveCompletePageCheckBox->setEnabled(false);

			if (feedInfo.forumId.empty()) {
				ui->forumNameLabel->setText(tr("Not yet created"));
			} else {
				//Todo: Replace with gxs forums
//				ForumInfo forumInfo;
//				if (rsForums->getForumInfo(feedInfo.forumId, forumInfo)) {
//					ui->forumNameLabel->setText(QString::fromStdWString(forumInfo.forumName));
//				} else {
//					ui->forumNameLabel->setText(tr("Unknown forum"));
//				}
			}
		} else {
			ui->typeLocalRadio->setChecked(true);
		}

		ui->useAuthenticationCheckBox->setChecked(feedInfo.flag.authentication);
		ui->userLineEdit->setText(QString::fromUtf8(feedInfo.user.c_str()));
		ui->passwordLineEdit->setText(QString::fromUtf8(feedInfo.password.c_str()));

		ui->useStandardProxyCheckBox->setChecked(feedInfo.flag.standardProxy);
		ui->proxyAddressLineEdit->setText(QString::fromUtf8(feedInfo.proxyAddress.c_str()));
		ui->proxyPortSpinBox->setValue(feedInfo.proxyPort);

		ui->useStandardUpdateInterval->setChecked(feedInfo.flag.standardUpdateInterval);
		ui->updateIntervalSpinBox->setValue(feedInfo.updateInterval / 60);
		QDateTime dateTime;
		dateTime.setTime_t(feedInfo.lastUpdate);
		ui->lastUpdate->setText(dateTime.toString());

		ui->useStandardStorageTimeCheckBox->setChecked(feedInfo.flag.standardStorageTime);
		ui->storageTimeSpinBox->setValue(feedInfo.storageTime / (60 * 60 *24));

		mTransformationType = feedInfo.transformationType;
		mXPathsToUse = feedInfo.xpathsToUse;
		mXPathsToRemove = feedInfo.xpathsToRemove;
		mXslt = feedInfo.xslt;

		ui->transformationTypeLabel->setText(FeedReaderStringDefs::transforationTypeString(mTransformationType));
	}

	return true;
}

void AddFeedDialog::getFeedInfo(FeedInfo &feedInfo)
{
	feedInfo.parentId = mParentId;

	feedInfo.name = ui->nameLineEdit->text().toUtf8().constData();
	feedInfo.url = ui->urlLineEdit->text().toUtf8().constData();
	feedInfo.flag.infoFromFeed = ui->useInfoFromFeedCheckBox->isChecked();
	feedInfo.flag.updateForumInfo = ui->updateForumInfoCheckBox->isChecked() && ui->updateForumInfoCheckBox->isEnabled();
	feedInfo.flag.deactivated = !ui->activatedCheckBox->isChecked();
	feedInfo.flag.embedImages = ui->embedImagesCheckBox->isChecked();
	feedInfo.flag.saveCompletePage = ui->saveCompletePageCheckBox->isChecked();

	feedInfo.description = ui->descriptionPlainTextEdit->toPlainText().toUtf8().constData();

	feedInfo.flag.forum = ui->typeForumRadio->isChecked();
	if (mFeedId.empty()) {
		if (feedInfo.flag.forum) {
			/* set forum (only when create a new feed) */
			feedInfo.forumId = ui->forumComboBox->itemData(ui->forumComboBox->currentIndex()).toString().toStdString();
		}
	}

	feedInfo.flag.authentication = ui->useAuthenticationCheckBox->isChecked();
	feedInfo.user = ui->userLineEdit->text().toUtf8().constData();
	feedInfo.password = ui->passwordLineEdit->text().toUtf8().constData();

	feedInfo.flag.standardProxy = ui->useStandardProxyCheckBox->isChecked();
	feedInfo.proxyAddress = ui->proxyAddressLineEdit->text().toUtf8().constData();
	feedInfo.proxyPort = ui->proxyPortSpinBox->value();

	feedInfo.flag.standardUpdateInterval = ui->useStandardUpdateInterval->isChecked();
	feedInfo.updateInterval = ui->updateIntervalSpinBox->value() * 60;

	feedInfo.flag.standardStorageTime = ui->useStandardStorageTimeCheckBox->isChecked();
	feedInfo.storageTime = ui->storageTimeSpinBox->value() * 60 *60 * 24;

	feedInfo.transformationType = mTransformationType;
	feedInfo.xpathsToUse = mXPathsToUse;
	feedInfo.xpathsToRemove = mXPathsToRemove;
	feedInfo.xslt = mXslt;
}

void AddFeedDialog::createFeed()
{
	FeedInfo feedInfo;
	if (!mFeedId.empty()) {
		if (!mFeedReader->getFeedInfo(mFeedId, feedInfo)) {
			QMessageBox::critical(this, tr("Edit feed"), tr("Can't edit feed. Feed does not exist."));
			return;
		}
	}

	getFeedInfo(feedInfo);

	if (mFeedId.empty()) {
		/* add new feed */
		RsFeedAddResult result = mFeedReader->addFeed(feedInfo, mFeedId);
		if (FeedReaderStringDefs::showError(this, result, tr("Create feed"), tr("Cannot create feed."))) {
			return;
		}
	} else {
		RsFeedAddResult result = mFeedReader->setFeed(mFeedId, feedInfo);
		if (FeedReaderStringDefs::showError(this, result, tr("Edit feed"), tr("Cannot change feed."))) {
			return;
		}
	}

	close();
}

void AddFeedDialog::preview()
{
	FeedInfo feedInfo;
	getFeedInfo(feedInfo);

	PreviewFeedDialog dialog(mFeedReader, mNotify, feedInfo, this);
	if (dialog.exec() == QDialog::Accepted) {
		mTransformationType = dialog.getData(mXPathsToUse, mXPathsToRemove, mXslt);
		ui->transformationTypeLabel->setText(FeedReaderStringDefs::transforationTypeString(mTransformationType));
	}
}
