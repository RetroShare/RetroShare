/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/CreateGxsForumMsg.cpp                      *
 *                                                                             *
 * Copyright 2013 Robert Fernie        <retroshare.project@gmail.com>          *
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

#include "CreateGxsForumMsg.h"

#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>
#include <QTextDocumentFragment>

#include <retroshare/rsgxsforums.h>
#include <retroshare/rsgxscircles.h>

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"
#include "gui/common/UIStateHelper.h"
#include "gui/Identity/IdEditDialog.h"
#include "gui/common/FilesDefs.h"

#include "util/HandleRichText.h"
#include "util/misc.h"

#include <sys/stat.h>
#include <iostream>

#define CREATEGXSFORUMMSG_FORUMINFO		1
#define CREATEGXSFORUMMSG_PARENTMSG		2
#define CREATEGXSFORUMMSG_CIRCLENFO		3
#define CREATEGXSFORUMMSG_ORIGMSG    	4

//#define ENABLE_GENERATE

/** Constructor */
CreateGxsForumMsg::CreateGxsForumMsg(const RsGxsGroupId &fId, const RsGxsMessageId &pId, const RsGxsMessageId& mOId, const RsGxsId& posterId, bool isModerating)
    : QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
      mForumId(fId), mParentId(pId), mOrigMsgId(mOId),mPosterId(posterId),mIsModerating(isModerating)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	/* Setup Queue */
	mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);
	mCirclesQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(CREATEGXSFORUMMSG_FORUMINFO, ui.postButton);
	mStateHelper->addWidget(CREATEGXSFORUMMSG_FORUMINFO, ui.innerFrame);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_FORUMINFO, ui.forumName);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_FORUMINFO, ui.forumSubject);
	mStateHelper->addClear(CREATEGXSFORUMMSG_FORUMINFO, ui.forumName);

	mStateHelper->addWidget(CREATEGXSFORUMMSG_PARENTMSG, ui.postButton);
	mStateHelper->addWidget(CREATEGXSFORUMMSG_PARENTMSG, ui.innerFrame);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_PARENTMSG, ui.forumName);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_PARENTMSG, ui.forumSubject);
	mStateHelper->addClear(CREATEGXSFORUMMSG_PARENTMSG, ui.forumName);

	mStateHelper->addWidget(CREATEGXSFORUMMSG_ORIGMSG, ui.postButton);
	mStateHelper->addWidget(CREATEGXSFORUMMSG_ORIGMSG, ui.innerFrame);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_ORIGMSG, ui.forumName);
	mStateHelper->addLoadPlaceholder(CREATEGXSFORUMMSG_ORIGMSG, ui.forumSubject);
	mStateHelper->addClear(CREATEGXSFORUMMSG_ORIGMSG, ui.forumName);


	QString text = mOId.isNull()?(pId.isNull() ? tr("Start New Thread") : tr("Post Forum Message")):tr("Edit Message");
	setWindowTitle(text);
	
	if (!mOId.isNull())
	ui.postButton->setText(tr ("Update"));

	ui.forumMessage->setPlaceholderText(tr ("Text"));

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums.png"));
	ui.headerFrame->setHeaderText(text);

	ui.generateSpinBox->setEnabled(false);

	Settings->loadWidgetInformation(this);

	connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	// connect up the buttons.
	connect(ui.postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
	connect(ui.attachFileButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui.attachPictureButton, SIGNAL(clicked()), this, SLOT(addPicture()));
	connect(ui.forumMessage, SIGNAL(textChanged()), this, SLOT(checkLength()));
	connect(ui.generateCheckBox, SIGNAL(toggled(bool)), ui.generateSpinBox, SLOT(setEnabled(bool)));

	setAcceptDrops(true);
	ui.hashBox->setDropWidget(this);
	ui.hashBox->setAutoHide(false);

	mParentMsgLoaded = false;
	mForumMetaLoaded = false;
	mForumCircleLoaded = false;

	newMsg();
	
	ui.hashGroupBox->hide();

#ifndef ENABLE_GENERATE
	ui.generateCheckBox->hide();
	ui.generateSpinBox->hide();
#endif
    processSettings(true);
}

CreateGxsForumMsg::~CreateGxsForumMsg()
{
    processSettings(false);
	delete(mForumQueue);
	delete(mCirclesQueue);
}

void CreateGxsForumMsg::processSettings(bool load)
{
    Settings->beginGroup(QString("ForumPostsWidget"));

    if (load)
    {
        // state of ID Chooser combobox
        RsGxsId gxs_id(Settings->value("IDChooser", QString::fromStdString(RsGxsId().toStdString())).toString().toStdString());

        if(!gxs_id.isNull() && rsIdentity->isOwnId(gxs_id))
            ui.idChooser->setChosenId(gxs_id);
    }
    else
    {
        // state of ID Chooser combobox
        RsGxsId id;

        if(ui.idChooser->getChosenId(id))
            Settings->setValue("IDChooser", QString::fromStdString(id.toStdString()));
    }

    Settings->endGroup();
}

void  CreateGxsForumMsg::newMsg()
{
	/* clear all */
	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	/* fill in the available OwnIds for signing */
    
    	//std::cerr << "Initing ID chooser. Sign flags = " << std::hex << mForumMeta.mSignFlags << std::dec << std::endl;
        
    if(!mPosterId.isNull())
    {
        std::set<RsGxsId> id_set ;
        id_set.insert(mPosterId) ;

		ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED | IDCHOOSER_NO_CREATE, mPosterId);
		ui.idChooser->setIdConstraintSet(id_set);
    }
    else
		ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED, mPosterId);

        if (mForumId.isNull()) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, false);
		mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, false);
		mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, false);

		mStateHelper->clear(CREATEGXSFORUMMSG_FORUMINFO);
		mStateHelper->clear(CREATEGXSFORUMMSG_PARENTMSG);
		mStateHelper->clear(CREATEGXSFORUMMSG_ORIGMSG);
		ui.forumName->setText(tr("No Forum"));
		return;
	}

	{/* request Data */
		mStateHelper->setLoading(CREATEGXSFORUMMSG_FORUMINFO, true);

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

                std::list<RsGxsGroupId> groupIds;
		groupIds.push_back(mForumId);

		//std::cerr << "ForumsV2Dialog::newMsg() Requesting Group Summary(" << mForumId << ")"<< std::endl;

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
		std::set<RsGxsMessageId> &vect = msgIds[mForumId];
		vect.insert(mParentId);

		//std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		//std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEGXSFORUMMSG_PARENTMSG);
	}

	if (mOrigMsgId.isNull()) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, true);
		mOrigMsgLoaded = true;
	} else {
		mStateHelper->setLoading(CREATEGXSFORUMMSG_ORIGMSG, true);

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

		GxsMsgReq msgIds;
		std::set<RsGxsMessageId> &vect = msgIds[mForumId];
		vect.insert(mOrigMsgId);

		//std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		//std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEGXSFORUMMSG_ORIGMSG);
	}
}

void  CreateGxsForumMsg::loadFormInformation()
{
	if (!mOrigMsgId.isNull())
	{
		if (mOrigMsgLoaded) {
			mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, true);
			mStateHelper->setLoading(CREATEGXSFORUMMSG_ORIGMSG, false);
		} else {
			//std::cerr << "CreateGxsForumMsg::loadMsgInformation() ParentMsg not Loaded Yet";
			//std::cerr << std::endl;

			mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, false);

			return;
		}
	}
    else
    {
		mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, true);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_ORIGMSG, false);
	}


	if (!mParentId.isNull())
	{
		if (mParentMsgLoaded) {
			mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, true);
			mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, false);
		} else {
			//std::cerr << "CreateGxsForumMsg::loadMsgInformation() ParentMsg not Loaded Yet";
			//std::cerr << std::endl;

			mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, false);

			return;
		}
	}
    else
    {
		mStateHelper->setActive(CREATEGXSFORUMMSG_PARENTMSG, true);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_PARENTMSG, false);
	}

	if (mForumMetaLoaded) {
		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, true);
		mStateHelper->setLoading(CREATEGXSFORUMMSG_FORUMINFO, false);
	} else {
		//std::cerr << "CreateGxsForumMsg::loadMsgInformation() ForumMeta not Loaded Yet";
		//std::cerr << std::endl;

		mStateHelper->setActive(CREATEGXSFORUMMSG_FORUMINFO, false);

		return;
	}

	//std::cerr << "CreateGxsForumMsg::loadMsgInformation() Data Available!";
	//std::cerr << std::endl;

	//std::cerr << "CreateGxsForumMsg::loadMsgInformation() using signFlags=" << std::hex << mForumMeta.mSignFlags << std::dec << std::endl;
    
    uint32_t fl = IDCHOOSER_ID_REQUIRED ;

	if( (mForumMeta.mSignFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) || (mForumMeta.mSignFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN))
		fl |= IDCHOOSER_NON_ANONYMOUS;

	if(!mPosterId.isNull())
		fl |= IDCHOOSER_NO_CREATE;

	ui.idChooser->setFlags(fl) ;

	QString name = QString::fromUtf8(mForumMeta.mGroupName.c_str());
	QString subj;

    if(!mOrigMsgId.isNull())
    {
        subj = QString::fromUtf8(mOrigMsg.mMeta.mMsgName.c_str());
    }
	else if (!mParentId.isNull())
	{
		QString title = QString::fromUtf8(mParentMsg.mMeta.mMsgName.c_str());
		name += " " + tr("In Reply to") + ": ";
		name += title;

		QString text = title;

		if (text.startsWith("Re:", Qt::CaseInsensitive))
			subj = title;
		else
			subj = "Re: " + title;
	}

	ui.forumName->setText(misc::removeNewLine(name));

	if(!subj.isNull())
		ui.forumSubject->setText(misc::removeNewLine(subj));

	if (ui.forumSubject->text().isEmpty())
	{
		ui.forumSubject->setFocus();
		ui.forumSubject->setPlaceholderText(tr ("Title"));
	}
	else
		ui.forumMessage->setFocus();

#ifdef TOGXS
	if (mForumMeta.mGroupFlags & RS_DISTRIB_AUTHEN_REQ)
#else
	if (1)
#endif
	{
		ui.signBox->setChecked(true);
		ui.signBox->setEnabled(false);
		ui.signBox->hide();
	}
	else
	{
		/* Uncheck sign box by default for anonymous forums */
		ui.signBox->setChecked(false);
		ui.signBox->setEnabled(true);
	}

	//ui.forumMessage->setText("");
}

static const uint32_t MAX_ALLOWED_GXS_MESSAGE_SIZE = 199000;

void CreateGxsForumMsg::checkLength()
{
	QString text;
	RsHtml::optimizeHtml(ui.forumMessage, text);
	std::wstring msg = text.toStdWString();
	int charRemains = MAX_ALLOWED_GXS_MESSAGE_SIZE - msg.length();
	if(charRemains >= 0) {
		text = tr("It remains %1 characters after HTML conversion.").arg(charRemains);
		ui.info_Label->setStyleSheet("QLabel#info_Label { }");
	}else{
		text = tr("Warning: This message is too big of %1 characters after HTML conversion.").arg((0-charRemains));
	    ui.info_Label->setStyleSheet("QLabel#info_Label {color: red; font: bold; }");
	}
	ui.postButton->setToolTip(text);
	ui.postButton->setEnabled(charRemains>=0);
	ui.info_Label->setText(text);
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
	msg.mMeta.mOrigMsgId = mOrigMsgId;
	msg.mMeta.mMsgFlags = mIsModerating?RS_GXS_FORUM_MSG_FLAGS_MODERATED : 0;
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
			//std::cerr << "CreateGxsForumMsg::createMsg() AuthorId: " << authorId;
			//std::cerr << std::endl;

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
		//std::cerr << "CreateGxsForumMsg::createMsg() No Signature (for now :)";
		//std::cerr << std::endl;
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
        ret = QMessageBox::warning(this, tr("Cancel Forum Message"),
                                   tr("Forum Message has not been sent yet!\n"
                                      "Do you want to discard this message?"),
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
	QString smiley = qobject_cast<QPushButton*>(sender())->toolTip().split("|").first();
	// add trailing space
	smiley += QString(" ");
	// add preceding space when needed (not at start of text or preceding space already exists)
	if(!ui.forumMessage->textCursor().atStart() && ui.forumMessage->toPlainText()[ui.forumMessage->textCursor().position() - 1] != QChar(' '))
		smiley = QString(" ") + smiley;
	ui.forumMessage->textCursor().insertText(smiley);
}

void CreateGxsForumMsg::addFile()
{
	QStringList files;
	if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
		ui.hashBox->addAttachments(files,RS_FILE_REQ_ANONYMOUS_ROUTING);
		ui.hashGroupBox->show();
	}
}

void CreateGxsForumMsg::addPicture()
{
	QString file;
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg)", file)) {
		QString encodedImage;
		if (RsHtml::makeEmbeddedImage(file, encodedImage, 640*480, MAX_ALLOWED_GXS_MESSAGE_SIZE - 200)) {
			QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
			ui.forumMessage->textCursor().insertFragment(fragment);
		}
	}
}

void CreateGxsForumMsg::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	//std::cerr << "CreateGxsForumMsg::fileHashingFinished() started." << std::endl;

	QString mesgString;

	QList<HashedFile>::iterator it;
	for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
		HashedFile& hashedFile = *it;
		RetroShareLink link = RetroShareLink::createFile(hashedFile.filename, hashedFile.size,
		                                                 QString::fromStdString(hashedFile.hash.toStdString()));
		if (link.valid()) {
			mesgString += link.toHtmlSize() + "<br>";
		}
	}

	if (!mesgString.isEmpty()) {
		ui.forumMessage->textCursor().insertHtml(mesgString);
	}

	ui.forumMessage->setFocus( Qt::OtherFocusReason );
	ui.hashGroupBox->hide();
}

void CreateGxsForumMsg::loadForumInfo(const uint32_t &token)
{
    //std::cerr << "CreateGxsForumMsg::loadForumInfo()";
    //std::cerr << std::endl;

    std::list<RsGroupMetaData> groupInfo;
    rsGxsForums->getGroupSummary(token, groupInfo);

    if (groupInfo.size() == 1)
    {
	    RsGroupMetaData fi = groupInfo.front();

	    mForumMeta = fi;
	    mForumMetaLoaded = true;

	    if(!fi.mCircleId.isNull())
	    {
		    //std::cerr << "Circle ID is not null: " << fi.mCircleId << ": loading circle info to add constraint to the GXS ID chooser." << std::endl;

		    RsTokReqOptions opts;
		    opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

		    std::list<RsGxsGroupId> groupIds;
		    groupIds.push_back(RsGxsGroupId(fi.mCircleId));
            		uint32_t _token;

		    mCirclesQueue->requestGroupInfo(_token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, CREATEGXSFORUMMSG_CIRCLENFO);
	    }

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
void CreateGxsForumMsg::loadForumCircleInfo(const uint32_t& token)
{
    //std::cerr << "Loading forum circle info" << std::endl;
    
    std::vector<RsGxsCircleGroup> circle_grp_v ;
    rsGxsCircles->getGroupData(token, circle_grp_v);

    if (circle_grp_v.empty())
    {
        std::cerr << "(EE) unexpected empty result from getGroupData. Cannot process circle now!" << std::endl;
        return ;
    }
        
    if (circle_grp_v.size() != 1)
    {
        std::cerr << "(EE) very weird result from getGroupData. Should get exactly one circle" << std::endl;
        return ;
    }
    
    RsGxsCircleGroup cg = circle_grp_v.front();

    mForumCircleData = cg;
    mForumCircleLoaded = true;

    //std::cerr << "Loaded content of circle " << cg.mMeta.mGroupId << std::endl;
    
    //for(std::set<RsGxsId>::const_iterator it(cg.mInvitedMembers.begin());it!=cg.mInvitedMembers.end();++it)
	//    std::cerr << "  added constraint to circle element " << *it << std::endl;
    
    ui.idChooser->setIdConstraintSet(cg.mInvitedMembers) ;
    ui.idChooser->setFlags(IDCHOOSER_NO_CREATE | ui.idChooser->flags()) ;	// since there's a circle involved, no ID creation can be needed
    
    RsGxsId tmpid ;
    if(ui.idChooser->countEnabledEntries() == 0)
    {
        QMessageBox::information(NULL,tr("No compatible ID for this forum"),tr("None of your identities is allowed to post in this forum. This could be due to the forum being limited to a circle that contains none of your identities, or forum flags requiring a PGP-signed identity.")) ;
        close() ;
    }
}

void CreateGxsForumMsg::loadOrigMsg(const uint32_t &token)
{
	//std::cerr << "CreateGxsForumMsg::loadParentMsg()";
	//std::cerr << std::endl;

	// Only grab one.... ignore more (shouldn't be any).
	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			/* error */
			std::cerr << "CreateGxsForumMsg::loadOrigMsg() ERROR wrong number of msgs";
			std::cerr << std::endl;

			mStateHelper->setActive(CREATEGXSFORUMMSG_ORIGMSG, false);
			mStateHelper->setLoading(CREATEGXSFORUMMSG_ORIGMSG, false);

			return;
		}

		mOrigMsg = msgs[0];
		mOrigMsgLoaded = true;

		loadFormInformation();
	}
}


void CreateGxsForumMsg::loadParentMsg(const uint32_t &token)
{
	//std::cerr << "CreateGxsForumMsg::loadParentMsg()";
	//std::cerr << std::endl;

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
	//std::cerr << "CreateGxsForum::loadRequest() UserType: " << req.mUserType;
	//std::cerr << std::endl;

	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATEGXSFORUMMSG_FORUMINFO:
				loadForumInfo(req.mToken);
				break;
			case CREATEGXSFORUMMSG_ORIGMSG:
				loadOrigMsg(req.mToken);
				break;
			case CREATEGXSFORUMMSG_PARENTMSG:
				loadParentMsg(req.mToken);
				break;
			default:
				std::cerr << "CreateGxsForumMsg::loadRequest() UNKNOWN UserType " << req.mUserType << " for token request in mForumQueue";
				std::cerr << std::endl;
		}
	}
    
    	if(queue == mCirclesQueue)
        {
            switch(req.mUserType)
            {
            case CREATEGXSFORUMMSG_CIRCLENFO:
                loadForumCircleInfo(req.mToken) ;
                break ;
            default:
				std::cerr << "CreateGxsForumMsg::loadRequest() UNKNOWN UserType " << req.mUserType << " for token request in mCirclesQueue";
				std::cerr << std::endl;
            }
        }
}
void CreateGxsForumMsg::setSubject(const QString& msg)
{
	ui.forumSubject->setText(msg);
}

void CreateGxsForumMsg::insertPastedText(const QString& msg)
{
	ui.forumMessage->append(msg);
}
