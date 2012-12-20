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

#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>

#include <retroshare/rsgxsforums.h>
#include <retroshare/rspeers.h>

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"

#include "util/HandleRichText.h"
#include "util/misc.h"

#include <sys/stat.h>
#include <iostream>


#define CREATEGXSFORUMMSG_FORUMINFO		1
#define CREATEGXSFORUMMSG_PARENTMSG		2


/** Constructor */
CreateGxsForumMsg::CreateGxsForumMsg(const std::string &fId, const std::string &pId)
: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint), mForumId(fId), mParentId(pId)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

	/* Setup Queue */
	mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);

    QString text = pId.empty() ? tr("Start New Thread") : tr("Post Forum Message");
    setWindowTitle(text);

    ui.headerFrame->setHeaderImage(QPixmap(":/images/konversation64.png"));
    ui.headerFrame->setHeaderText(text);

    Settings->loadWidgetInformation(this);

    connect( ui.forumMessage, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumMessageCostumPopupMenu( QPoint ) ) );

    connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

    // connect up the buttons.
    connect( ui.buttonBox, SIGNAL(accepted()), this, SLOT(createMsg()));
    connect( ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect( ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
    connect( ui.attachFileButton, SIGNAL(clicked() ), this , SLOT(addFile()));
    connect( ui.pastersButton, SIGNAL(clicked() ), this , SLOT(pasteLink()));

    setAcceptDrops(true);
    ui.hashBox->setDropWidget(this);
    ui.hashBox->setAutoHide(false);

	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

    newMsg();
}

/** context menu searchTablewidget2 **/
void CreateGxsForumMsg::forumMessageCostumPopupMenu(QPoint point)
{
    QMenu *contextMnu = ui.forumMessage->createStandardContextMenu(point);

    contextMnu->addSeparator();
    QAction *pasteLinkAct = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    QAction *pasteLinkFullAct = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste full RetroShare Link"), this, SLOT(pasteLinkFull()));
    contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste my certificate link"), this, SLOT(pasteOwnCertificateLink()));

    if (RSLinkClipboard::empty()) {
        pasteLinkAct->setDisabled (true);
        pasteLinkFullAct->setDisabled (true);
    }

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void  CreateGxsForumMsg::newMsg()
{
    /* clear all */
	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	/* fill in the available OwnIds for signing */
	ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED, "");

	/* request Data */
	{
		RsTokReqOptions opts;
	
		std::list<std::string> groupIds;
		groupIds.push_back(mForumId);
	
		std::cerr << "ForumsV2Dialog::newMsg() Requesting Group Summary(" << mForumId << ")";
		std::cerr << std::endl;
	
		uint32_t token;
		mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, groupIds, CREATEGXSFORUMMSG_FORUMINFO);

	}
	
	if (mParentId != "")
	{
	
		RsTokReqOptions opts;
			
		std::list<std::string> msgIds;
		msgIds.push_back(mParentId);
			
		std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		std::cerr << std::endl;
			
		uint32_t token;
#ifdef TOGXS
		mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEGXSFORUMMSG_PARENTMSG);
#endif
	}		
}


void  CreateGxsForumMsg::saveForumInfo(const RsGroupMetaData &meta)
{
	mForumMeta = meta;
	mForumMetaLoaded = true;
	
	loadFormInformation();
}

void  CreateGxsForumMsg::saveParentMsg(const RsGxsForumMsg &msg)
{
	mParentMsg = msg;
	mParentMsgLoaded = true;
	
	loadFormInformation();
}

void  CreateGxsForumMsg::loadFormInformation()
{
	if ((!mParentMsgLoaded) && (mParentId != ""))
	{
		std::cerr << "CreateGxsForumMsg::loadMsgInformation() ParentMsg not Loaded Yet";
		std::cerr << std::endl;
		return;
	}
	
	if (!mForumMetaLoaded)
	{
		std::cerr << "CreateGxsForumMsg::loadMsgInformation() ForumMeta not Loaded Yet";
		std::cerr << std::endl;
		return;
	}
	
	std::cerr << "CreateGxsForumMsg::loadMsgInformation() Data Available!";
	std::cerr << std::endl;
	
	QString name = QString::fromUtf8(mForumMeta.mGroupName.c_str());
	QString subj;
	if (mParentId != "")
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
		
	if (!ui.forumSubject->text().isEmpty())
	{
		ui.forumMessage->setFocus();
	}
	else
	{
		ui.forumSubject->setFocus();
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

    ui.forumMessage->setText("");
}



void  CreateGxsForumMsg::createMsg()
{
    QString name = misc::removeNewLine(ui.forumSubject->text());
    QString desc;

	RsHtml::optimizeHtml(ui.forumMessage, desc);

    if(name.isEmpty())
    {	/* error message */
        QMessageBox::warning(this, tr("RetroShare"),tr("Please set a Forum Subject and Forum Message"),
                             QMessageBox::Ok, QMessageBox::Ok);

        return; //Don't add  a empty Subject!!
    }

	RsGxsForumMsg msg;
	msg.mMeta.mGroupId = mForumId;
	msg.mMeta.mParentId = mParentId;
	msg.mMeta.mMsgId = "";
	if (mParentMsgLoaded)
	{
		msg.mMeta.mThreadId = mParentMsg.mMeta.mThreadId;
	}
	
	msg.mMeta.mMsgName = std::string(name.toUtf8());
	msg.mMsg = std::string(desc.toUtf8());
#ifdef TOGXS
	msg.mMeta.mMsgFlags = RS_DISTRIB_AUTHEN_REQ;
#endif
	
	if ((msg.mMsg == "") && (msg.mMeta.mMsgName == ""))
        	return; /* do nothing */

	if (ui.signBox->isChecked())
	{
		RsGxsId authorId;
		if (ui.idChooser->getChosenId(authorId))
		{
			msg.mMeta.mAuthorId = authorId;
			std::cerr << "CreateGxsForumMsg::createMsg() AuthorId: " << authorId;
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "CreateGxsForumMsg::createMsg() ERROR GETTING AuthorId!";
			std::cerr << std::endl;
		}
	}
	else
	{
		std::cerr << "CreateGxsForumMsg::createMsg() No Signature (for now :)";
		std::cerr << std::endl;
	}

	uint32_t token;
	rsGxsForums->createMsg(token, msg);
	close();
	
	
	// Previous Info - for reference.
	
    //ForumMsgInfo msgInfo;

    //msgInfo.forumId = mForumId;
    //msgInfo.threadId = "";
    //msgInfo.parentId = mParentId;
    //msgInfo.msgId = "";

    //msgInfo.title = name.toStdWString();
    //msgInfo.msg = desc.toStdWString();
    //msgInfo.msgflags = 0;

    //if (ui.signBox->isChecked())
    //{
    //    msgInfo.msgflags = RS_DISTRIB_AUTHEN_REQ;
    //}

    //if ((msgInfo.msg == L"") && (msgInfo.title == L""))
    //    return; /* do nothing */

    //if (rsForumsV2->ForumMessageSend(msgInfo) == true) {
    //    close();
    //}
}




void CreateGxsForumMsg::closeEvent (QCloseEvent * /*event*/)
{
    Settings->saveWidgetInformation(this);
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
        if (link.createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash))) {
            mesgString += link.toHtmlSize() + "<br>";
        }
    }

#ifdef CHAT_DEBUG
    std::cerr << "CreateGxsForumMsg::anchorClicked mesgString : " << mesgString.toStdString() << std::endl;
#endif

    if (!mesgString.isEmpty()) {
        ui.forumMessage->textCursor().insertHtml(mesgString);
    }

    ui.forumMessage->setFocus( Qt::OtherFocusReason );
}

void CreateGxsForumMsg::pasteLink()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtml()) ;
}

void CreateGxsForumMsg::pasteLinkFull()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtmlFull()) ;
}

void CreateGxsForumMsg::pasteOwnCertificateLink()
{
	RetroShareLink link ;
	std::string ownId = rsPeers->getOwnId() ;
	if( link.createCertificate(ownId) )	{
		ui.forumMessage->insertHtml(link.toHtml() + " ");
	}
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
		saveForumInfo(fi);
	}
	else
	{
		std::cerr << "CreateGxsForumMsg::loadForumInfo() ERROR INVALID Number of Forums";
		std::cerr << std::endl;
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
	

		}	
		saveParentMsg(msgs[0]);
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

