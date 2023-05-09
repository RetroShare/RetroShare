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
#include <retroshare/rsposted.h>
#include <iostream>

#define TOKEN_TYPE_FORUM_GROUPS  1
#define TOKEN_TYPE_POSTED_GROUPS 2

AddFeedDialog::AddFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent)
	: QDialog(parent, Qt::Window), mFeedReader(feedReader), mNotify(notify), ui(new Ui::AddFeedDialog)
{
	ui->setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mFeedId = 0;
	mParentId = 0;

	mStateHelper->addWidget(TOKEN_TYPE_FORUM_GROUPS, ui->forumComboBox, UISTATE_LOADING_DISABLED);
	mStateHelper->addWidget(TOKEN_TYPE_FORUM_GROUPS, ui->buttonBox->button(QDialogButtonBox::Ok), UISTATE_LOADING_DISABLED);
	mStateHelper->addWidget(TOKEN_TYPE_POSTED_GROUPS, ui->postedComboBox, UISTATE_LOADING_DISABLED);
	mStateHelper->addWidget(TOKEN_TYPE_POSTED_GROUPS, ui->buttonBox->button(QDialogButtonBox::Ok), UISTATE_LOADING_DISABLED);

	/* Setup TokenQueue */
	mForumTokenQueue = new TokenQueue(rsGxsForums->getTokenService(), this);
	mPostedTokenQueue = new TokenQueue(rsPosted->getTokenService(), this);

	/* Connect signals */
	connect(ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(createFeed()));
	connect(ui->buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

	connect(ui->useAuthenticationCheckBox, SIGNAL(toggled(bool)), this, SLOT(authenticationToggled()));
	connect(ui->useStandardStorageTimeCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardStorageTimeToggled()));
	connect(ui->useStandardUpdateInterval, SIGNAL(toggled(bool)), this, SLOT(useStandardUpdateIntervalToggled()));
	connect(ui->useStandardProxyCheckBox, SIGNAL(toggled(bool)), this, SLOT(useStandardProxyToggled()));
	connect(ui->typeForumCheckBox, SIGNAL(toggled(bool)), this, SLOT(typeForumToggled()));
	connect(ui->typePostedCheckBox, SIGNAL(toggled(bool)), this, SLOT(typePostedToggled()));
	connect(ui->typeLocalCheckBox, SIGNAL(toggled(bool)), this, SLOT(typeLocalToggled()));
	connect(ui->postedFirstImageCheckBox, SIGNAL(toggled(bool)), this, SLOT(postedFirstImageToggled()));
	connect(ui->previewButton, SIGNAL(clicked()), this, SLOT(preview()));

	/* currently only for local feeds */
	connect(ui->saveCompletePageCheckBox, SIGNAL(toggled(bool)), this, SLOT(denyForumAndPostedToggled()));

	connect(ui->urlLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));
	connect(ui->useInfoFromFeedCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeLocalCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->typeForumCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->forumComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	connect(ui->typePostedCheckBox, SIGNAL(toggled(bool)), this, SLOT(validate()));
	connect(ui->postedComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));

	connect(ui->clearCachePushButton, SIGNAL(clicked()), this, SLOT(clearMessageCache()));

	ui->headerFrame->setHeaderText(tr("Feed Details"));
	ui->headerFrame->setHeaderImage(QPixmap(":/images/FeedReader.png"));

	ui->activatedCheckBox->setChecked(true);
	mStateHelper->setWidgetEnabled(ui->forumComboBox, false);
	mStateHelper->setWidgetEnabled(ui->postedComboBox, false);
	ui->useInfoFromFeedCheckBox->setChecked(true);
	ui->updateForumInfoCheckBox->setEnabled(false);
	ui->updateForumInfoCheckBox->setChecked(true);
	ui->updatePostedInfoCheckBox->setEnabled(false);
	ui->updatePostedInfoCheckBox->setChecked(true);
	ui->postedFirstImageCheckBox->setEnabled(false);
	ui->postedFirstImageCheckBox->setChecked(false);
	ui->postedOnlyImageCheckBox->setEnabled(false);
	ui->postedOnlyImageCheckBox->setChecked(false);
	ui->postedShinkImageCheckBox->setEnabled(false);
	ui->postedShinkImageCheckBox->setChecked(true);
	ui->useAuthenticationCheckBox->setChecked(false);
	ui->useStandardStorageTimeCheckBox->setChecked(true);
	ui->useStandardUpdateInterval->setChecked(true);
	ui->useStandardProxyCheckBox->setChecked(true);
	ui->embedImagesCheckBox->setChecked(true);
	ui->saveCompletePageCheckBox->setEnabled(false);

	/* not yet supported */
	ui->authenticationGroupBox->setEnabled(false);

	mTransformationType = RS_FEED_TRANSFORMATION_TYPE_NONE;
	ui->transformationTypeLabel->setText(FeedReaderStringDefs::transforationTypeString(mTransformationType));

	ui->clearCachePushButton->show();

	/* fill own forums */
	requestForumGroups();

	/* fill own posted */
	requestPostedGroups();

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
	delete(mForumTokenQueue);
	delete(mPostedTokenQueue);
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
	bool checked = ui->typeForumCheckBox->isChecked();
	mStateHelper->setWidgetEnabled(ui->forumComboBox, checked);
	ui->updateForumInfoCheckBox->setEnabled(checked);

	if (checked) {
		ui->typeLocalCheckBox->setChecked(false);
	}
}

void AddFeedDialog::postedFirstImageToggled()
{
	bool checked = ui->postedFirstImageCheckBox->isChecked();
	ui->postedOnlyImageCheckBox->setEnabled(checked);

	if (!checked) {
		ui->postedOnlyImageCheckBox->setChecked(false);
	}
}

void AddFeedDialog::typePostedToggled()
{
	bool checked = ui->typePostedCheckBox->isChecked();
	mStateHelper->setWidgetEnabled(ui->postedComboBox, checked);
	ui->updatePostedInfoCheckBox->setEnabled(checked);
	ui->postedFirstImageCheckBox->setEnabled(checked);
	ui->postedShinkImageCheckBox->setEnabled(checked);

	if (checked) {
		ui->typeLocalCheckBox->setChecked(false);
	}else {
		ui->postedFirstImageCheckBox->setChecked(false);
	}
}

void AddFeedDialog::typeLocalToggled()
{
	bool checked = ui->typeLocalCheckBox->isChecked();
	if (checked) {
		mStateHelper->setWidgetEnabled(ui->forumComboBox, false);
		mStateHelper->setWidgetEnabled(ui->postedComboBox, false);
		ui->typeForumCheckBox->setChecked(false);
		ui->typePostedCheckBox->setChecked(false);
		ui->saveCompletePageCheckBox->setEnabled(true);
	} else {
		ui->saveCompletePageCheckBox->setEnabled(false);
		ui->saveCompletePageCheckBox->setChecked(false);
	}
}

void AddFeedDialog::denyForumAndPostedToggled()
{
	if (ui->saveCompletePageCheckBox->isChecked()) {
		ui->typeForumCheckBox->setEnabled(false);
		ui->typeForumCheckBox->setChecked(false);
		ui->typePostedCheckBox->setEnabled(false);
		ui->typePostedCheckBox->setChecked(false);
		ui->typeLocalCheckBox->setChecked(true);
	} else {
		ui->typeForumCheckBox->setEnabled(true);
		ui->typePostedCheckBox->setEnabled(true);
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

	if (!ui->typeLocalCheckBox->isChecked() && !ui->typeForumCheckBox->isChecked() && !ui->typePostedCheckBox->isChecked()) {
		ok = false;
	}

	if (ui->typeForumCheckBox->isChecked() && ui->forumComboBox->itemData(ui->forumComboBox->currentIndex()).toString().isEmpty()) {
		ok = false;
	}

	if (ui->typePostedCheckBox->isChecked() && ui->postedComboBox->itemData(ui->postedComboBox->currentIndex()).toString().isEmpty()) {
		ok = false;
	}

	mStateHelper->setWidgetEnabled(ui->buttonBox->button(QDialogButtonBox::Ok), ok);
}

void AddFeedDialog::setParent(uint32_t parentId)
{
	mParentId = parentId;
}

bool AddFeedDialog::fillFeed(uint32_t feedId)
{
	mFeedId = feedId;

	if (mFeedId) {
		FeedInfo feedInfo;
		if (!mFeedReader->getFeedInfo(mFeedId, feedInfo)) {
			mFeedId = 0;
			return false;
		}

		setWindowTitle(tr("Edit feed"));

		mParentId = feedInfo.parentId;

		ui->nameLineEdit->setText(QString::fromUtf8(feedInfo.name.c_str()));
		ui->urlLineEdit->setText(QString::fromUtf8(feedInfo.url.c_str()));
		ui->useInfoFromFeedCheckBox->setChecked(feedInfo.flag.infoFromFeed);
		ui->updateForumInfoCheckBox->setChecked(feedInfo.flag.updateForumInfo);
		ui->updatePostedInfoCheckBox->setChecked(feedInfo.flag.updatePostedInfo);
		ui->postedFirstImageCheckBox->setChecked(feedInfo.flag.postedFirstImage);
		ui->postedOnlyImageCheckBox->setChecked(feedInfo.flag.postedOnlyImage);
		ui->postedShinkImageCheckBox->setChecked(feedInfo.flag.postedShrinkImage);
		ui->activatedCheckBox->setChecked(!feedInfo.flag.deactivated);
		ui->embedImagesCheckBox->setChecked(feedInfo.flag.embedImages);
		ui->saveCompletePageCheckBox->setChecked(feedInfo.flag.saveCompletePage);

		ui->descriptionPlainTextEdit->setPlainText(QString::fromUtf8(feedInfo.description.c_str()));

		if (feedInfo.flag.forum || feedInfo.flag.posted) {
			if (feedInfo.flag.forum) {
				mStateHelper->setWidgetEnabled(ui->forumComboBox, true);
				ui->typeForumCheckBox->setChecked(true);

				setActiveForumId(feedInfo.forumId);
			}
			if (feedInfo.flag.posted) {
				mStateHelper->setWidgetEnabled(ui->postedComboBox, true);
				ui->typePostedCheckBox->setChecked(true);

				setActivePostedId(feedInfo.postedId);
			}
			ui->saveCompletePageCheckBox->setEnabled(false);
		} else {
			ui->typeLocalCheckBox->setChecked(true);
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

void AddFeedDialog::setActivePostedId(const std::string &postedId)
{
	if (mStateHelper->isLoading(TOKEN_TYPE_POSTED_GROUPS)) {
		mFillPostedId = postedId;
		return;
	}

	int index = ui->postedComboBox->findData(QString::fromStdString(postedId));
	if (index >= 0) {
		ui->postedComboBox->setCurrentIndex(index);
	} else {
		ui->postedComboBox->setCurrentIndex(0);
	}
}

void AddFeedDialog::getFeedInfo(FeedInfo &feedInfo)
{
	feedInfo.parentId = mParentId;

	feedInfo.name = ui->nameLineEdit->text().toUtf8().constData();
	feedInfo.url = ui->urlLineEdit->text().toUtf8().constData();
	feedInfo.flag.infoFromFeed = ui->useInfoFromFeedCheckBox->isChecked();
	feedInfo.flag.updateForumInfo = ui->updateForumInfoCheckBox->isChecked() && ui->updateForumInfoCheckBox->isEnabled();
	feedInfo.flag.updatePostedInfo = ui->updatePostedInfoCheckBox->isChecked() && ui->updatePostedInfoCheckBox->isEnabled();
	feedInfo.flag.postedFirstImage = ui->postedFirstImageCheckBox->isChecked() && ui->postedFirstImageCheckBox->isEnabled();
	feedInfo.flag.postedOnlyImage = ui->postedOnlyImageCheckBox->isChecked() && ui->postedOnlyImageCheckBox->isEnabled();
	feedInfo.flag.postedShrinkImage = ui->postedShinkImageCheckBox->isChecked() && ui->postedShinkImageCheckBox->isEnabled();
	feedInfo.flag.deactivated = !ui->activatedCheckBox->isChecked();
	feedInfo.flag.embedImages = ui->embedImagesCheckBox->isChecked();
	feedInfo.flag.saveCompletePage = ui->saveCompletePageCheckBox->isChecked();

	feedInfo.description = ui->descriptionPlainTextEdit->toPlainText().toUtf8().constData();

	feedInfo.flag.forum = ui->typeForumCheckBox->isChecked();

	if (feedInfo.flag.forum) {
		feedInfo.forumId = ui->forumComboBox->itemData(ui->forumComboBox->currentIndex()).toString().toStdString();
	}

	feedInfo.flag.posted = ui->typePostedCheckBox->isChecked();

	if (feedInfo.flag.posted) {
		feedInfo.postedId = ui->postedComboBox->itemData(ui->postedComboBox->currentIndex()).toString().toStdString();
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
	if (mFeedId) {
		if (!mFeedReader->getFeedInfo(mFeedId, feedInfo)) {
			QMessageBox::critical(this, tr("Edit feed"), tr("Can't edit feed. Feed does not exist."));
			return;
		}
	}

	getFeedInfo(feedInfo);

	if (mFeedId == 0) {
		/* add new feed */
		RsFeedResult result = mFeedReader->addFeed(feedInfo, mFeedId);
		if (FeedReaderStringDefs::showError(this, result, tr("Create feed"), tr("Cannot create feed."))) {
			return;
		}
	} else {
		RsFeedResult result = mFeedReader->setFeed(mFeedId, feedInfo);
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
	if (mFeedId == 0) {
		return;
	}

	mFeedReader->clearMessageCache(mFeedId);
}

void AddFeedDialog::requestForumGroups()
{
	mStateHelper->setLoading(TOKEN_TYPE_FORUM_GROUPS, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	mForumTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_FORUM_GROUPS);

	uint32_t token;
	mForumTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, TOKEN_TYPE_FORUM_GROUPS);
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

void AddFeedDialog::requestPostedGroups()
{
	mStateHelper->setLoading(TOKEN_TYPE_POSTED_GROUPS, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	mPostedTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_POSTED_GROUPS);

	uint32_t token;
	mPostedTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, TOKEN_TYPE_POSTED_GROUPS);
}

void AddFeedDialog::loadPostedGroups(const uint32_t &token)
{
	std::vector<RsPostedGroup> groups;
	rsPosted->getGroupData(token, groups);

	ui->postedComboBox->clear();

	for (std::vector<RsPostedGroup>::iterator it = groups.begin(); it != groups.end(); ++it) {
		const RsPostedGroup &group = *it;

		/* show only own posted */
		if (IS_GROUP_PUBLISHER(group.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(group.mMeta.mSubscribeFlags) && !group.mMeta.mAuthorId.isNull()) {
			ui->postedComboBox->addItem(QString::fromUtf8(group.mMeta.mGroupName.c_str()), QString::fromStdString(group.mMeta.mGroupId.toStdString()));
		}
	}

	/* insert empty item */
	ui->postedComboBox->insertItem(0, "", "");
	ui->postedComboBox->setCurrentIndex(0);

	mStateHelper->setLoading(TOKEN_TYPE_POSTED_GROUPS, false);

	if (!mFillPostedId.empty()) {
		setActivePostedId(mFillPostedId);
		mFillPostedId.clear();
	}
}

void AddFeedDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mForumTokenQueue)
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

	if (queue == mPostedTokenQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_POSTED_GROUPS:
			loadPostedGroups(req.mToken);
			break;

		default:
			std::cerr << "AddFeedDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;

		}
	}
}
