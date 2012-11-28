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

#include "CreateForumV2Msg.h"

#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>

#include <retroshare/rsforumsv2.h>
#include <retroshare/rspeers.h>

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"

#include "util/HandleRichText.h"
#include "util/misc.h"

#include <sys/stat.h>
#include <iostream>


#define CREATEFORUMV2MSG_FORUMINFO		1
#define CREATEFORUMV2MSG_PARENTMSG		2


/** Constructor */
CreateForumV2Msg::CreateForumV2Msg(std::string fId, std::string pId)
: QMainWindow(NULL), mForumId(fId), mParentId(pId)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

	/* Setup Queue */
	mForumQueue = new TokenQueue(rsForumsV2, this);

    Settings->loadWidgetInformation(this);

    connect( ui.forumMessage, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumMessageCostumPopupMenu( QPoint ) ) );

    connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

    // connect up the buttons.
    connect( ui.postmessage_action, SIGNAL( triggered (bool) ), this, SLOT( createMsg( ) ) );
    connect( ui.close_action, SIGNAL( triggered (bool) ), this, SLOT( cancelMsg( ) ) );
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
void CreateForumV2Msg::forumMessageCostumPopupMenu(QPoint point)
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

void  CreateForumV2Msg::newMsg()
{
    /* clear all */
	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	/* request Data */
	{
		RsTokReqOptions opts;
	
		std::list<std::string> groupIds;
		groupIds.push_back(mForumId);
	
		std::cerr << "ForumsV2Dialog::newMsg() Requesting Group Summary(" << mForumId << ")";
		std::cerr << std::endl;
	
		uint32_t token;
		mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, groupIds, CREATEFORUMV2MSG_FORUMINFO);

	}
	
	if (mParentId != "")
	{
	
		RsTokReqOptions opts;
			
		std::list<std::string> msgIds;
		msgIds.push_back(mParentId);
			
		std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		std::cerr << std::endl;
			
		uint32_t token;
		mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEFORUMV2MSG_PARENTMSG);
	}		
}


void  CreateForumV2Msg::saveForumInfo(const RsGroupMetaData &meta)
{
	mForumMeta = meta;
	mForumMetaLoaded = true;
	
	loadFormInformation();
}

void  CreateForumV2Msg::saveParentMsg(const RsForumV2Msg &msg)
{
	mParentMsg = msg;
	mParentMsgLoaded = true;
	
	loadFormInformation();
}

void  CreateForumV2Msg::loadFormInformation()
{
	if ((!mParentMsgLoaded) && (mParentId != ""))
	{
		std::cerr << "CreateForumV2Msg::loadMsgInformation() ParentMsg not Loaded Yet";
		std::cerr << std::endl;
		return;
	}
	
	if (!mForumMetaLoaded)
	{
		std::cerr << "CreateForumV2Msg::loadMsgInformation() ForumMeta not Loaded Yet";
		std::cerr << std::endl;
		return;
	}
	
	std::cerr << "CreateForumV2Msg::loadMsgInformation() Data Available!";
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
		
	if (mForumMeta.mGroupFlags & RS_DISTRIB_AUTHEN_REQ)
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



void  CreateForumV2Msg::createMsg()
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

	RsForumV2Msg msg;
	msg.mMeta.mGroupId = mForumId;
	msg.mMeta.mParentId = mParentId;
	msg.mMeta.mMsgId = "";
	if (mParentMsgLoaded)
	{
		msg.mMeta.mThreadId = mParentMsg.mMeta.mThreadId;
	}
	
	msg.mMeta.mMsgName = std::string(name.toUtf8());
	msg.mMsg = std::string(desc.toUtf8());
	msg.mMeta.mMsgFlags = RS_DISTRIB_AUTHEN_REQ;
	
	if ((msg.mMsg == "") && (msg.mMeta.mMsgName == ""))
        return; /* do nothing */
	
	uint32_t token;
	rsForumsV2->createMsg(token, msg, true);
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




void CreateForumV2Msg::closeEvent (QCloseEvent * /*event*/)
{
    Settings->saveWidgetInformation(this);
}

void CreateForumV2Msg::cancelMsg()
{
    close();
}

void CreateForumV2Msg::smileyWidgetForums()
{
    Emoticons::showSmileyWidget(this, ui.emoticonButton, SLOT(addSmileys()), false);
}

void CreateForumV2Msg::addSmileys()
{
    ui.forumMessage->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void CreateForumV2Msg::addFile()
{
    QStringList files;
    if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
        ui.hashBox->addAttachments(files);
    }
}

void CreateForumV2Msg::fileHashingFinished(QList<HashedFile> hashedFiles)
{
    std::cerr << "CreateForumV2Msg::fileHashingFinished() started." << std::endl;

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
    std::cerr << "CreateForumV2Msg::anchorClicked mesgString : " << mesgString.toStdString() << std::endl;
#endif

    if (!mesgString.isEmpty()) {
        ui.forumMessage->textCursor().insertHtml(mesgString);
    }

    ui.forumMessage->setFocus( Qt::OtherFocusReason );
}

void CreateForumV2Msg::pasteLink()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtml()) ;
}

void CreateForumV2Msg::pasteLinkFull()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtmlFull()) ;
}

void CreateForumV2Msg::pasteOwnCertificateLink()
{
	RetroShareLink link ;
	std::string ownId = rsPeers->getOwnId() ;
	if( link.createCertificate(ownId) )	{
		ui.forumMessage->insertHtml(link.toHtml() + " ");
	}
}




void CreateForumV2Msg::loadForumInfo(const uint32_t &token)
{
	std::cerr << "CreateForumV2Msg::loadForumInfo()";
	std::cerr << std::endl;
	
	std::list<RsGroupMetaData> groupInfo;
	rsForumsV2->getGroupSummary(token, groupInfo);
	
	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		saveForumInfo(fi);
	}
	else
	{
		std::cerr << "CreateForumV2Msg::loadForumInfo() ERROR INVALID Number of Forums";
		std::cerr << std::endl;
	}
}


void CreateForumV2Msg::loadParentMsg(const uint32_t &token)
{
	std::cerr << "CreateForumV2Msg::loadParentMsg()";
	std::cerr << std::endl;
	
	// Only grab one.... ignore more (shouldn't be any).
	RsForumV2Msg msg;
	rsForumsV2->getMsgData(token, msg);	
	saveParentMsg(msg);
}



void CreateForumV2Msg::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateForumV2::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATEFORUMV2MSG_FORUMINFO:
				loadForumInfo(req.mToken);
				break;
				
			case CREATEFORUMV2MSG_PARENTMSG:
				loadParentMsg(req.mToken);
				break;
			default:
				std::cerr << "CreateForumV2::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
				
		}
	}
}

