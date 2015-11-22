/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "CreateGxsForumMsg.h"

#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>

#include <retroshare/rsgxsforums.h>

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"
#include "gui/common/UIStateHelper.h"
#include "gui/Identity/IdEditDialog.h"

#include "util/HandleRichText.h"
#include "util/misc.h"

#include <sys/stat.h>
#include <iostream>

#define CREATEGXSFORUMMSG_FORUMINFO		1
#define CREATEGXSFORUMMSG_PARENTMSG		2

//#define ENABLE_GENERATE

/** Constructor */
CreateGxsForumMsg::CreateGxsForumMsg(const RsGxsGroupId &fId, const RsGxsMessageId &pId)
: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint), mForumId(fId), mParentId(pId)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	/* Setup Queue */
	mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(CREATEGXSFORUMMSG_FORUMINFO, ui.buttonBox->button(QDialogButtonBox::Ok));
	mStateHelper->addWidget(CREATEGXSFORUMMSG_FORUMINFO, ui.innerFrame);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_FORUMINFO, ui.forumName);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_FORUMINFO, ui.forumSubject);
	mStateHelper->addClear(CREATEGXSFORUMMSG_FORUMINFO, ui.forumName);

	mStateHelper->addWidget(CREATEGXSFORUMMSG_PARENTMSG, ui.buttonBox->button(QDialogButtonBox::Ok));
	mStateHelper->addWidget(CREATEGXSFORUMMSG_PARENTMSG, ui.innerFrame);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_PARENTMSG, ui.forumName);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_PARENTMSG, ui.forumSubject);
	mStateHelper->addClear(CREATEGXSFORUMMSG_PARENTMSG, ui.forumName);

	QString text = pId.isNull() ? tr("Start New Thread") : tr("Post Forum Message");
	setWindowTitle(text);

	ui.headerFrame->setHeaderImage(QPixmap(":/images/konversation64.png"));
	ui.headerFrame->setHeaderText(text);

	ui.generateSpinBox->setEnabled(false);

	Settings->loadWidgetInformation(this);

	connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	/* Rename Ok button */
	ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Send"));

	// connect up the buttons.
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(createMsg()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
	connect(ui.attachFileButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui.generateCheckBox, SIGNAL(toggled(bool)), ui.generateSpinBox, SLOT(setEnabled(bool)));

	setAcceptDrops(true);
	ui.hashBox->setDropWidget(this);
	ui.hashBox->setAutoHide(false);

	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	newMsg();

#ifndef ENABLE_GENERATE
	ui.generateCheckBox->hide();
	ui.generateSpinBox->hide();
#endif
}

CreateGxsForumMsg::~CreateGxsForumMsg()
{
	delete(mForumQueue);
}

void  CreateGxsForumMsg::newMsg()
{
	/* clear all */
	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	/* fill in the available OwnIds for signing */
    	std::cerr << "Initing ID chooser. Sign flags = " << std::hex << mForumMeta.mSignFlags << std::dec << std::endl;
        
	ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());

        if (mForumId.isNull()) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, false);
		mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, false);
		mStateHelper->clear(CREATEGXSFORUMMSG_FORUMINFO);
		mStateHelper->clear(CREATEGXSFORUMMSG_PARENTMSG);
		ui.forumName->setText(tr("No Forum"));
		return;
	}//if ( mForumId.isNull())

	{/* request Data */
		mStateHelper->setLoading(CREATEGXSFORUMMSG_FORUMINFO, true);

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

                std::list<RsGxsGroupId> groupIds;
		groupIds.push_back(mForumId);

		std::cerr << "ForumsV2Dialog::newMsg() Requesting Group Summary(" << mForumId << ")";
		std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, groupIds, CREATEGXSFORUMMSG_FORUMINFO);
	}/* request Data */

	if (mParentId.isNull()) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, true);
		mParentMsgLoaded = true;
	} else {
		mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, true);

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

		GxsMsgReq msgIds;
		std::vector<RsGxsMessageId> &vect = msgIds[mForumId];
		vect.push_back(mParentId);

		std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEGXSFORUMMSG_PARENTMSG);
	}//if (mParentId.isNull())
}

void  CreateGxsForumMsg::loadFormInformation()
{
        if (!mParentId.isNull()) {
		if (mParentMsgLoaded) {
			mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, true);
			mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, false);
		} else {
			std::cerr << "CreateGxsForumMsg::loadMsgInformation() ParentMsg not Loaded Yet";
			std::cerr << std::endl;

			mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, false);

			return;
		}
	} else {
		mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, true);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, false);
	}

	if (mForumMetaLoaded) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, true);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_FORUMINFO, false);
	} else {
		std::cerr << "CreateGxsForumMsg::loadMsgInformation() ForumMeta not Loaded Yet";
		std::cerr << std::endl;

		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, false);

		return;
	}

	std::cerr << "CreateGxsForumMsg::loadMsgInformation() Data Available!";
	std::cerr << std::endl;

	std::cerr << "CreateGxsForumMsg::loadMsgInformation() using signFlags=" << std::hex << mForumMeta.mSignFlags << std::dec << std::endl;
    
    	if(mForumMeta.mSignFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG)
		ui.idChooser->setFlags(IDCHOOSER_ID_REQUIRED | IDCHOOSER_NON_ANONYMOUS) ;
        else
		ui.idChooser->setFlags(IDCHOOSER_ID_REQUIRED) ;
    	
	QString name = QString::fromUtf8(mForumMeta.mGroupName.c_str());
	QString subj;
        if (!mParentId.isNull())
	{
		QString title = QString::fromUtf8(mParentMsg.mMeta.mMsgName.c_str());
		name += " " + tr("In Reply to") + ": ";
		name += title;

		QString text = title;

		if (text.startsWith("Re:", Qt::CaseInsensitive))
		{
			subj = title;
		}
		else
		{
			subj = "Re: " + title;
		}
	}

	ui.forumName->setText(misc::removeNewLine(name));
	ui.forumSubject->setText(misc::removeNewLine(subj));

	if (ui.forumSubject->text().isEmpty())
	{
		ui.forumSubject->setFocus();
	}
	else
	{
		ui.forumMessage->setFocus();
	}

#ifdef TOGXS
	if (mForumMeta.mGroupFlags & RS_DISTRIB_AUTHEN_REQ)
#else
	if (1)
#endif
	{
		ui.signBox->setChecked(true);
		ui.signBox->setEnabled(false);
	}
	else
	{
		/* Uncheck sign box by default for anonymous forums */
		ui.signBox->setChecked(false);
		ui.signBox->setEnabled(true);
	}

	//ui.forumMessage->setText("");
}

void  CreateGxsForumMsg::createMsg()
{
	QString name = misc::removeNewLine(ui.forumSubject->text());
	QString desc;

	RsHtml::optimizeHtml(ui.forumMessage, desc);

	if(name.isEmpty() | desc.isEmpty()) {
		/* error message */
		QMessageBox::warning(this, tr("RetroShare"),tr("Please set a Forum Subject and Forum Message"), QMessageBox::Ok, QMessageBox::Ok);

		return; //Don't add  a empty Subject!!
	}//if(name.isEmpty())

	RsGxsForumMsg msg;
	msg.mMeta.mGroupId = mForumId;
	msg.mMeta.mParentId = mParentId;
	msg.mMeta.mMsgId.clear() ;
	if (mParentMsgLoaded) {
		msg.mMeta.mThreadId = mParentMsg.mMeta.mThreadId;
	}//if (mParentMsgLoaded)

	msg.mMeta.mMsgName = std::string(name.toUtf8());
	msg.mMsg = std::string(desc.toUtf8());
#ifdef TOGXS
	msg.mMeta.mMsgFlags = RS_DISTRIB_AUTHEN_REQ;
#endif

	if ((msg.mMsg == "") && (msg.mMeta.mMsgName == ""))
		return; /* do nothing */

	if (ui.signBox->isChecked()) {
		RsGxsId authorId;
		switch (ui.idChooser->getChosenId(authorId)) {
			case GxsIdChooser::KnowId:
			case GxsIdChooser::UnKnowId:
			msg.mMeta.mAuthorId = authorId;
			std::cerr << "CreateGxsForumMsg::createMsg() AuthorId: " << authorId;
			std::cerr << std::endl;

			break;
            case GxsIdChooser::None:
            {
                // This is ONLY for the case where no id exists yet
                // If an id exists, the chooser would not return None
                IdEditDialog dlg(this);
                dlg.setupNewId(false);
                dlg.exec();
                // fetch new id, we will then see if the identity creation was successful
                std::list<RsGxsId> own_ids;
                if(!rsIdentity->getOwnIds(own_ids) || own_ids.size() != 1)
                    return;
                // we have only a single id, so we can use the first one
                authorId = own_ids.front();
                break;
            }
			case GxsIdChooser::NoId:
			default:
			std::cerr << "CreateGxsForumMsg::createMsg() ERROR GETTING AuthorId!";
			std::cerr << std::endl;
            QMessageBox::warning(this, tr("RetroShare"),tr("Congrats, you found a bug!")+" "+QString(__FILE__)+":"+QString(__LINE__), QMessageBox::Ok, QMessageBox::Ok);

			return;
		}//switch (ui.idChooser->getChosenId(authorId))
	} else {
		std::cerr << "CreateGxsForumMsg::createMsg() No Signature (for now :)";
		std::cerr << std::endl;
		QMessageBox::warning(this, tr("RetroShare"),tr("Please choose Signing Id, it is required"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}//if (ui.signBox->isChecked())

	int generateCount = 0;

#ifdef ENABLE_GENERATE
	if (ui.generateCheckBox->isChecked()) {
		generateCount = ui.generateSpinBox->value();
		if (QMessageBox::question(this, tr("Generate mass data"), tr("Do you really want to generate %1 messages ?").arg(generateCount), QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
			return;
		}//if (QMessageBox::question(this,
	}//if (ui.generateCheckBox->isChecked())
#endif

	uint32_t token;
	if (generateCount) {
#ifdef ENABLE_GENERATE
		for (int count = 0; count < generateCount; ++count) {
			RsGxsForumMsg generateMsg = msg;
			generateMsg.mMeta.mMsgName = QString("%1 %2").arg(QString::fromUtf8(msg.mMeta.mMsgName.c_str())).arg(count + 1, 3, 10, QChar('0')).toUtf8().constData();

			rsGxsForums->createMsg(token, generateMsg);
		}//for (int count = 0
#endif
	} else {
		rsGxsForums->createMsg(token, msg);
	}//if (generateCount)

	close();
}

void CreateGxsForumMsg::closeEvent (QCloseEvent * /*event*/)
{
	Settings->saveWidgetInformation(this);
}

void CreateGxsForumMsg::reject()
{
    if (ui.forumMessage->document()->isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Forum Message"),
                                   tr("Forum Message has not been Sent.\n"
                                      "Do you want to reject this message?"),
                                   QMessageBox::Yes | QMessageBox::No);
        switch (ret) {
        case QMessageBox::Yes:
            break;
        case QMessageBox::No:
            return; // don't close
        default:
            break;
        }
    }

    QDialog::reject();
}

void CreateGxsForumMsg::smileyWidgetForums()
{
	Emoticons::showSmileyWidget(this, ui.emoticonButton, SLOT(addSmileys()), false);
}

void CreateGxsForumMsg::addSmileys()
{
	ui.forumMessage->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void CreateGxsForumMsg::addFile()
{
	QStringList files;
	if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
		ui.hashBox->addAttachments(files,RS_FILE_REQ_ANONYMOUS_ROUTING);
	}
}

void CreateGxsForumMsg::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	std::cerr << "CreateGxsForumMsg::fileHashingFinished() started." << std::endl;

	QString mesgString;

	QList<HashedFile>::iterator it;
	for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
		HashedFile& hashedFile = *it;
		RetroShareLink link;
                if (link.createFile(hashedFile.filename, hashedFile.size,
                                    QString::fromStdString(hashedFile.hash.toStdString()))) {
			mesgString += link.toHtmlSize() + "<br>";
		}
	}

	if (!mesgString.isEmpty()) {
		ui.forumMessage->textCursor().insertHtml(mesgString);
	}

	ui.forumMessage->setFocus( Qt::OtherFocusReason );
}

void CreateGxsForumMsg::loadForumInfo(const uint32_t &token)
{
	std::cerr << "CreateGxsForumMsg::loadForumInfo()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> groupInfo;
	rsGxsForums->getGroupSummary(token, groupInfo);

	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();

		mForumMeta = fi;
		mForumMetaLoaded = true;

		loadFormInformation();
	}
	else
	{
		std::cerr << "CreateGxsForumMsg::loadForumInfo() ERROR INVALID Number of Forums";
		std::cerr << std::endl;

		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, false);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_FORUMINFO, false);
	}
}

void CreateGxsForumMsg::loadParentMsg(const uint32_t &token)
{
	std::cerr << "CreateGxsForumMsg::loadParentMsg()";
	std::cerr << std::endl;

	// Only grab one.... ignore more (shouldn't be any).
	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			/* error */
			std::cerr << "CreateGxsForumMsg::loadParentMsg() ERROR wrong number of msgs";
			std::cerr << std::endl;

			mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, false);
			mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, false);

			return;
		}

		mParentMsg = msgs[0];
		mParentMsgLoaded = true;

		loadFormInformation();
	}
}

void CreateGxsForumMsg::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateGxsForum::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATEGXSFORUMMSG_FORUMINFO:
				loadForumInfo(req.mToken);
				break;
			case CREATEGXSFORUMMSG_PARENTMSG:
				loadParentMsg(req.mToken);
				break;
			default:
				std::cerr << "CreateGxsForum::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
		}
	}
}

void CreateGxsForumMsg::insertPastedText(QString msg)
{
	ui.forumMessage->append(msg);
}
