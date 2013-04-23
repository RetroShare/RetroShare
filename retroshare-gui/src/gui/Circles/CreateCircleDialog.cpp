/*
 * Retroshare Circles.
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "gui/Circles/CreateCircleDialog.h"

#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#if 0
#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"

#include "util/HandleRichText.h"
#include "util/misc.h"

#include <sys/stat.h>
#include <iostream>
#endif


#define 	CREATECIRCLEDIALOG_CIRCLEINFO	2
#define 	CREATECIRCLEDIALOG_IDINFO	3

#define RSCIRCLEID_COL_NICKNAME       0
#define RSCIRCLEID_COL_KEYID          1
#define RSCIRCLEID_COL_IDTYPE         2



/** Constructor */
CreateCircleDialog::CreateCircleDialog()
: QDialog(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

	/* Setup Queue */
	mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);
	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	//QString text = pId.empty() ? tr("Start New Thread") : tr("Post Forum Message");
	//setWindowTitle(text);
	//Settings->loadWidgetInformation(this);

	//connect(ui.forumMessage, SIGNAL( customContextMenuRequested(QPoint)), this, SLOT(forumMessageCostumPopupMenu(QPoint)));
	//connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	// connect up the buttons.
	connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addMember()));
	connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));

	//connect(ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
	//connect(ui.attachFileButton, SIGNAL(clicked()), this, SLOT(addFile()));
	//connect(ui.pastersButton, SIGNAL(clicked()), this, SLOT(pasteLink()));

        connect(ui.treeWidget_membership, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedMember(QTreeWidgetItem*, QTreeWidgetItem*)));
        connect(ui.treeWidget_IdList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedId(QTreeWidgetItem*, QTreeWidgetItem*)));

	ui.removeButton->setEnabled(false);
	ui.addButton->setEnabled(false);
	ui.radioButton_ListAll->setChecked(true);
	requestIdentities();
}



CreateCircleDialog::~CreateCircleDialog()
{
	delete(mCircleQueue);
	delete(mIdQueue);
}


void CreateCircleDialog::selectedId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
        Q_UNUSED(previous);
	ui.addButton->setEnabled(current != NULL);
}

void CreateCircleDialog::selectedMember(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
        Q_UNUSED(previous);
	ui.removeButton->setEnabled(current != NULL);
}


#if 0
#define CREATEGXSFORUMMSG_FORUMINFO		1
#define CREATEGXSFORUMMSG_PARENTMSG		2


/** Constructor */
CreateCircleDialog::CreateCircleDialog(const std::string &fId, const std::string &pId)
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

	connect(ui.forumMessage, SIGNAL( customContextMenuRequested(QPoint)), this, SLOT(forumMessageCostumPopupMenu(QPoint)));

	connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	// connect up the buttons.
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(createMsg()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
	connect(ui.attachFileButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui.pastersButton, SIGNAL(clicked()), this, SLOT(pasteLink()));

	setAcceptDrops(true);
	ui.hashBox->setDropWidget(this);
	ui.hashBox->setAutoHide(false);

	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	newMsg();
}

CreateCircleDialog::~CreateCircleDialog()
{
	delete(mForumQueue);
}

void CreateCircleDialog::forumMessageCostumPopupMenu(QPoint point)
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

void  CreateCircleDialog::newMsg()
{
	/* clear all */
	mParentMsgLoaded = false;
	mForumMetaLoaded = false;

	/* fill in the available OwnIds for signing */
	ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED, "");

	/* lock gui */
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui.innerFrame->setEnabled(false);

	if (mForumId.empty()) {
		ui.forumName->setText(tr("No Forum"));
		return;
	}
	ui.forumName->setText(tr("Loading"));

	/* request Data */
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

		std::list<std::string> groupIds;
		groupIds.push_back(mForumId);

		std::cerr << "ForumsV2Dialog::newMsg() Requesting Group Summary(" << mForumId << ")";
		std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, groupIds, CREATEGXSFORUMMSG_FORUMINFO);
	}

	if (mParentId.empty())
	{
		mParentMsgLoaded = true;
	}
	else
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

		GxsMsgReq msgIds;
		std::vector<RsGxsMessageId> &vect = msgIds[mForumId];
		vect.push_back(mParentId);

		std::cerr << "ForumsV2Dialog::newMsg() Requesting Parent Summary(" << mParentId << ")";
		std::cerr << std::endl;

		uint32_t token;
		mForumQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, CREATEGXSFORUMMSG_PARENTMSG);
	}
}

void  CreateCircleDialog::saveForumInfo(const RsGroupMetaData &meta)
{
	mForumMeta = meta;
	mForumMetaLoaded = true;

	loadFormInformation();
}

void  CreateCircleDialog::saveParentMsg(const RsGxsForumMsg &msg)
{
	mParentMsg = msg;
	mParentMsgLoaded = true;

	loadFormInformation();
}

void  CreateCircleDialog::loadFormInformation()
{
	if ((!mParentMsgLoaded) && (!mParentId.empty()))
	{
		std::cerr << "CreateCircleDialog::loadMsgInformation() ParentMsg not Loaded Yet";
		std::cerr << std::endl;
		return;
	}

	if (!mForumMetaLoaded)
	{
		std::cerr << "CreateCircleDialog::loadMsgInformation() ForumMeta not Loaded Yet";
		std::cerr << std::endl;
		return;
	}

	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	ui.innerFrame->setEnabled(true);

	std::cerr << "CreateCircleDialog::loadMsgInformation() Data Available!";
	std::cerr << std::endl;

	QString name = QString::fromUtf8(mForumMeta.mGroupName.c_str());
	QString subj;
	if (!mParentId.empty())
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

	ui.forumMessage->setText("");
}

void  CreateCircleDialog::createMsg()
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
			std::cerr << "CreateCircleDialog::createMsg() AuthorId: " << authorId;
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "CreateCircleDialog::createMsg() ERROR GETTING AuthorId!";
			std::cerr << std::endl;
			QMessageBox::warning(this, tr("RetroShare"),tr("Please choose Signing Id"),
					 QMessageBox::Ok, QMessageBox::Ok);

			return;
		}
	}
	else
	{
		std::cerr << "CreateCircleDialog::createMsg() No Signature (for now :)";
		std::cerr << std::endl;
		QMessageBox::warning(this, tr("RetroShare"),tr("Please choose Signing Id, it is required"),
			 QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	uint32_t token;
	rsGxsForums->createMsg(token, msg);
	close();
}

void CreateCircleDialog::closeEvent (QCloseEvent * /*event*/)
{
	Settings->saveWidgetInformation(this);
}

void CreateCircleDialog::smileyWidgetForums()
{
	Emoticons::showSmileyWidget(this, ui.emoticonButton, SLOT(addSmileys()), false);
}

void CreateCircleDialog::addSmileys()
{
	ui.forumMessage->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void CreateCircleDialog::addFile()
{
	QStringList files;
	if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
		ui.hashBox->addAttachments(files,RS_FILE_REQ_ANONYMOUS_ROUTING);
	}
}

void CreateCircleDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	std::cerr << "CreateCircleDialog::fileHashingFinished() started." << std::endl;

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
	std::cerr << "CreateCircleDialog::anchorClicked mesgString : " << mesgString.toStdString() << std::endl;
#endif

	if (!mesgString.isEmpty()) {
		ui.forumMessage->textCursor().insertHtml(mesgString);
	}

	ui.forumMessage->setFocus( Qt::OtherFocusReason );
}

void CreateCircleDialog::pasteLink()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtml()) ;
}

void CreateCircleDialog::pasteLinkFull()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtmlFull()) ;
}

void CreateCircleDialog::pasteOwnCertificateLink()
{
	RetroShareLink link ;
	std::string ownId = rsPeers->getOwnId() ;
	if( link.createCertificate(ownId) )	{
		ui.forumMessage->insertHtml(link.toHtml() + " ");
	}
}

void CreateCircleDialog::loadForumInfo(const uint32_t &token)
{
	std::cerr << "CreateCircleDialog::loadForumInfo()";
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
		std::cerr << "CreateCircleDialog::loadForumInfo() ERROR INVALID Number of Forums";
		std::cerr << std::endl;
	}
}

void CreateCircleDialog::loadParentMsg(const uint32_t &token)
{
	std::cerr << "CreateCircleDialog::loadParentMsg()";
	std::cerr << std::endl;

	// Only grab one.... ignore more (shouldn't be any).
	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			/* error */
			std::cerr << "CreateCircleDialog::loadParentMsg() ERROR wrong number of msgs";
			std::cerr << std::endl;
		}
		saveParentMsg(msgs[0]);
	}
}

void CreateCircleDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateCircleDialog::loadRequest() UserType: " << req.mUserType;
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
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;

		}
	}
}
#endif


void  CreateCircleDialog::addMember()
{
        QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
		return;
	}
        QTreeWidgetItem *member = new QTreeWidgetItem();
        member->setText(RSCIRCLEID_COL_NICKNAME, item->text(RSCIRCLEID_COL_NICKNAME));
        member->setText(RSCIRCLEID_COL_KEYID, item->text(RSCIRCLEID_COL_KEYID));
        member->setText(RSCIRCLEID_COL_IDTYPE, item->text(RSCIRCLEID_COL_IDTYPE));

	ui.treeWidget_membership->addTopLevelItem(member);

}


void  CreateCircleDialog::removeMember()
{
        QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();
	if (!item)
	{
		return;
	}

	// does this just work?
	delete(item);
}



void  CreateCircleDialog::createCircle()
{
	QString name = ui.circleName->text();
	QString desc;

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, tr("RetroShare"),tr("Please set a name for your Circle"),
							 QMessageBox::Ok, QMessageBox::Ok);

		return; //Don't add  a empty Subject!!
	}

	RsGxsCircleGroup circle;

	circle.mMeta.mGroupName = std::string(name.toUtf8());

	RsGxsId authorId;
	if (ui.idChooser->getChosenId(authorId))
	{
		circle.mMeta.mAuthorId = authorId;
		std::cerr << "CreateCircleDialog::createCircle() AuthorId: " << authorId;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "CreateCircleDialog::createCircle() No AuthorId Chosen!";
		std::cerr << std::endl;
	}


	/* copy Ids from GUI */



	uint32_t token;
	rsGxsCircles->createGroup(token, circle);
	close();
}



void CreateCircleDialog::requestCircle(const RsGxsGroupId &groupId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId);

	std::cerr << "CreateCircleDialog::requestCircle() Requesting Group Summary(" << groupId << ")";
	std::cerr << std::endl;

	uint32_t token;
	mCircleQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, CREATECIRCLEDIALOG_CIRCLEINFO);
}

void CreateCircleDialog::loadCircle(uint32_t token)
{
	std::cerr << "CreateCircleDialog::loadCircle(" << token << ")";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_membership;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	RsGxsIdGroup data;
	std::vector<RsGxsCircleGroup> groups;
	if (!rsGxsCircles->getGroupData(token, groups))
	{
		std::cerr << "CreateCircleDialog::loadCircle() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "CreateCircleDialog::loadCircle() Error Group.size() != 1";
		std::cerr << std::endl;
		return;
	}
		
	std::cerr << "CreateCircleDialog::loadCircle() Unfinished Loading";
	std::cerr << std::endl;

	//mCircleGroup = groups[0];
}


void CreateCircleDialog::requestIdentities()
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::cerr << "CreateCircleDialog::requestIdentities()";
	std::cerr << std::endl;

	uint32_t token;
	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, CREATECIRCLEDIALOG_IDINFO);
}





void CreateCircleDialog::loadIdentities(uint32_t token)
{
	std::cerr << "CreateCircleDialog::loadIdentities(" << token << ")";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	bool acceptAll = ui.radioButton_ListAll->isChecked();
	bool acceptAllPGP = ui.radioButton_ListAllPGP->isChecked();
	bool acceptKnownPGP = ui.radioButton_ListKnownPGP->isChecked();

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		std::cerr << "CreateCircleDialog::insertIdentities() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	for(vit = datavector.begin(); vit != datavector.end(); vit++)
	{
		data = (*vit);

		/* do filtering */
		bool ok = false;
		if (acceptAll)
		{
			ok = true;
		}
		else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			if (acceptAllPGP)
			{
				ok = true;
			}
			else if (data.mPgpKnown)
			{
				ok = true;
			}
		}

		if (!ok)
		{
			std::cerr << "CreateCircleDialog::insertIdentities() Skipping ID: " << data.mMeta.mGroupId;
			std::cerr << std::endl;
			continue;
		}


                QTreeWidgetItem *item = new QTreeWidgetItem();
                item->setText(RSCIRCLEID_COL_NICKNAME, QString::fromStdString(data.mMeta.mGroupName));
                item->setText(RSCIRCLEID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId));
                if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
                {
                        if (data.mPgpKnown)
                        {
                                RsPeerDetails details;
                                rsPeers->getGPGDetails(data.mPgpId, details);
                                item->setText(RSCIRCLEID_COL_IDTYPE, QString::fromStdString(details.name));
                        }
                        else
                        {
                                item->setText(RSCIRCLEID_COL_IDTYPE, "PGP Linked Id");
                        }
                }
                else
                {
                        item->setText(RSCIRCLEID_COL_IDTYPE, "Anon Id");
                }
		tree->addTopLevelItem(item);
	}
}



void CreateCircleDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateCircleDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mCircleQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATECIRCLEDIALOG_CIRCLEINFO:
				loadCircle(req.mToken);
				break;
			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;

		}
	}

	if (queue == mIdQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATECIRCLEDIALOG_IDINFO:
				loadIdentities(req.mToken);
				break;
			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;

		}
	}
}
