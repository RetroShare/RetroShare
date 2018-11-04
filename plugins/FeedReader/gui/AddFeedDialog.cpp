/*******************************************************************************
 * plugins/FeedReader/gui/AddFeedDialog.cpp                                    *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

#include <QMessageBox>
#include <QDateTime>
#include <QPushButton>

#include "AddFeedDialog.h"
#include "ui_AddFeedDialog.h"
#include "PreviewFeedDialog.h"
#include "FeedReaderStringDefs.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rsgxsforums.h>
#include <iostream>

#define TOKEN_TYPE_FORUM_GROUPS 1

AddFeedDialog::AddFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent)
	: QDialog(parent, Qt::Window), mFeedReader(feedReader), mNotify(notify), ui(new Ui::AddFeedDialog)
{
	ui->setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(TOKEN_TYPE_FORUM_GROUPS, ui->forumComboBox, UISTATE_LOADING_DISABLED);
	mStateHelper->addWidget(TOKEN_TYPE_FORUM_GROUPS, ui->buttonBox->button(QDialogButtonBox::Ok), UISTATE_LOADING_DISABLED);

	/* Setup TokenQueue */
	mTokenQueue = new TokenQueue(rsGxsForums->getTokenService(), this);

	/* Connect signals */
	connect(ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(createFeed()));
	connect(ui->buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

	connect(ui->useAuthenticationCheckBox, SIGNAL(toggled(bool)), this, SLOT(authenticationToggled()));
	connect(ui->useStandardStorageTimeCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardStorageTimeToggled()));
	connect(ui->useStandardUpdateInterval, SIGNAL(toggled(bool)), this, SLOT(useStandardUpdateIntervalToggled()));
	connect(ui->useStandardProxyCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardProxyToggled()));
	connect(ui->typeForumRadio, SIGNAL(toggled(bool)), this, SLOT(typeForumToggled()));
	connect(ui->previewButton, SIGNAL(clicked()), this, SLOT(preview()));

	/* currently only for local feeds */
	connect(ui->saveCompletePageCheckBox, SIGNAL(toggled(bool)), this, SLOT(denyForumToggled()));

	connect(ui->urlLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->useInfoFromFeedCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeLocalRadio, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeForumRadio, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->forumComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));

	connect(ui->clearCachePushButton, SIGNAL(clicked()), this, SLOT(clearMessageCache()));

	ui->headerFrame->setHeaderText(tr("Feed Details"));
	ui->headerFrame->setHeaderImage(QPixmap(":/images/FeedReader.png"));

	ui->activatedCheckBox->setChecked(true);
	mStateHelper->setWidgetEnabled(ui->forumComboBox, false);
	ui->useInfoFromFeedCheckBox->setChecked(true);
	ui->updateForumInfoCheckBox->setEnabled(false);
	ui->updateForumInfoCheckBox->setChecked(true);
	ui->useAuthenticationCheckBox->setChecked(false);
	ui->useStandardStorageTimeCheckBox->setChecked(true);
	ui->useStandardUpdateInterval->setChecked(true);
	ui->useStandardProxyCheckBox->setChecked(true);

	/* not yet supported */
	ui->authenticationGroupBox->setEnabled(false);

	mTransformationType = RS_FEED_TRANSFORMATION_TYPE_NONE;
	ui->transformationTypeLabel->setText(FeedReaderStringDefs::transforationTypeString(mTransformationType));

	ui->clearCachePushButton->show();

	/* fill own forums */
	requestForumGroups();

	validate();

	ui->urlLineEdit->setFocus();

	/* load settings */
	processSettings(true);
}

AddFeedDialog::~AddFeedDialog()
{
	/* save settings */
	processSettings(false);

	delete(ui);
	delete(mTokenQueue);
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
	mStateHelper->setWidgetEnabled(ui->forumComboBox, checked);
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

	if (ui->typeForumRadio->isChecked() && ui->forumComboBox->itemData(ui->forumComboBox->currentIndex()).toString().isEmpty()) {
		ok = false;
	}

	mStateHelper->setWidgetEnabled(ui->buttonBox->button(QDialogButtonBox::Ok), ok);
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

		mParentId = feedInfo.parentId;

		ui->nameLineEdit->setText(QString::fromUtf8(feedInfo.name.c_str()));
		ui->urlLineEdit->setText(QString::fromUtf8(feedInfo.url.c_str()));
		ui->useInfoFromFeedCheckBox->setChecked(feedInfo.flag.infoFromFeed);
		ui->updateForumInfoCheckBox->setChecked(feedInfo.flag.updateForumInfo);
		ui->activatedCheckBox->setChecked(!feedInfo.flag.deactivated);
		ui->embedImagesCheckBox->setChecked(feedInfo.flag.embedImages);
		ui->saveCompletePageCheckBox->setChecked(feedInfo.flag.saveCompletePage);

		ui->descriptionPlainTextEdit->setPlainText(QString::fromUtf8(feedInfo.description.c_str()));

		if (feedInfo.flag.forum) {
			mStateHelper->setWidgetEnabled(ui->forumComboBox, true);
			ui->typeForumRadio->setChecked(true);
			ui->saveCompletePageCheckBox->setEnabled(false);

			setActiveForumId(feedInfo.forumId);
		} else {
			ui->typeLocalRadio->setChecked(true);
			mStateHelper->setWidgetEnabled(ui->forumComboBox, false);
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

		ui->clearCachePushButton->show();
	}

	return true;
}

void AddFeedDialog::setActiveForumId(const std::string &forumId)
{
	if (mStateHelper->isLoading(TOKEN_TYPE_FORUM_GROUPS)) {
		mFillForumId = forumId;
		return;
	}

	int index = ui->forumComboBox->findData(QString::fromStdString(forumId));
	if (index >= 0) {
		ui->forumComboBox->setCurrentIndex(index);
	} else {
		ui->forumComboBox->setCurrentIndex(0);
	}
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

	if (feedInfo.flag.forum) {
		feedInfo.forumId = ui->forumComboBox->itemData(ui->forumComboBox->currentIndex()).toString().toStdString();
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

void AddFeedDialog::clearMessageCache()
{
	if (mFeedId.empty()) {
		return;
	}

	mFeedReader->clearMessageCache(mFeedId);
}

void AddFeedDialog::requestForumGroups()
{
	mStateHelper->setLoading(TOKEN_TYPE_FORUM_GROUPS, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	mTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_FORUM_GROUPS);

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, TOKEN_TYPE_FORUM_GROUPS);
}

void AddFeedDialog::loadForumGroups(const uint32_t &token)
{
	std::vector<RsGxsForumGroup> groups;
	rsGxsForums->getGroupData(token, groups);

	ui->forumComboBox->clear();

	for (std::vector<RsGxsForumGroup>::iterator it = groups.begin(); it != groups.end(); ++it) {
		const RsGxsForumGroup &group = *it;

		/* show only own forums */
		if (IS_GROUP_PUBLISHER(group.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(group.mMeta.mSubscribeFlags) && !group.mMeta.mAuthorId.isNull()) {
			ui->forumComboBox->addItem(QString::fromUtf8(group.mMeta.mGroupName.c_str()), QString::fromStdString(group.mMeta.mGroupId.toStdString()));
		}
	}

	/* insert empty item */
	ui->forumComboBox->insertItem(0, "", "");
	ui->forumComboBox->setCurrentIndex(0);

	mStateHelper->setLoading(TOKEN_TYPE_FORUM_GROUPS, false);

	if (!mFillForumId.empty()) {
		setActiveForumId(mFillForumId);
		mFillForumId.clear();
	}
}

void AddFeedDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mTokenQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_FORUM_GROUPS:
			loadForumGroups(req.mToken);
			break;

		default:
			std::cerr << "AddFeedDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;

		}
	}
}
