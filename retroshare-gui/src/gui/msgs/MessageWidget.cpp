/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageWidget.cpp                               *
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

#include <QMenu>
#include <QToolButton>
#include <QDateTime>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDesktopServices>
#include <QPlainTextEdit>
#include <QDialog>

#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "gui/common/TagDefs.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/Emoticons.h"
#include "gui/common/FilesDefs.h"
#include "gui/settings/rsharesettings.h"
#include "MessageComposer.h"
#include "MessageWidget.h"
#include "MessageWindow.h"
#include "util/misc.h"
#include "util/printpreview.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/QtVersion.h"
#include "util/qtthreadsutils.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsmsgs.h>

/* Images for context menu icons */
#define IMAGE_DOWNLOAD         ":/icons/png/download.png"
#define IMAGE_DOWNLOADALL      ":/icons/mail/downloadall.png"

#define COLUMN_FILE_NAME   0
#define COLUMN_FILE_SIZE   1
#define COLUMN_FILE_HASH   2
#define COLUMN_FILE_COUNT  3

#include "gui/msgs/MessageInterface.h"

class RsHtmlMsg : public RsHtml
{
public:
	explicit RsHtmlMsg(uint msgFlags) : RsHtml()
	{
		this->msgFlags = msgFlags;
	}

protected:
	virtual void anchorTextForImg(QDomDocument &doc, QDomElement &element, const RetroShareLink &link, QString &text)
	{
		if (link.type() == RetroShareLink::TYPE_CERTIFICATE) {
			if (msgFlags & RS_MSG_USER_REQUEST) {
				text = QApplication::translate("MessageWidget", "Confirm %1 as friend").arg(link.name());
				return;
			}
			if (msgFlags & RS_MSG_FRIEND_RECOMMENDATION) {
				text = QApplication::translate("MessageWidget", "Add %1 as friend").arg(link.name());
				return;
			}
		}

		RsHtml::anchorTextForImg(doc, element, link, text);
	}

protected:
	uint msgFlags;
};

MessageWidget *MessageWidget::openMsg(const std::string &msgId, bool window)
{
	if (msgId.empty()) {
		return NULL;
	}

	MessageInfo msgInfo;
	if (!rsMail->getMessage(msgId, msgInfo)) {
		std::cerr << "MessageWidget::openMsg() Couldn't find Msg" << std::endl;
		return NULL;
	}

	MessageWindow *parent = NULL;
	if (window) {
		parent = new MessageWindow;
	}
	MessageWidget *msgWidget = new MessageWidget(false, parent);
	msgWidget->isWindow = window;
	msgWidget->fill(msgId);

    if (parent) {
		parent->addWidget(msgWidget);
	}

	if (parent) {
		parent->show();
		parent->activateWindow();
	}

	return msgWidget;
}

/** Constructor */
MessageWidget::MessageWidget(bool controlled, QWidget *parent, Qt::WindowFlags flags)
  : QWidget(parent, flags), toolButtonReply(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	isControlled = controlled;
	isWindow = false;
	currMsgFlags = 0;
	expandFiles = false;
	ui.expandButton->hide();

	ui.actionTextBesideIcon->setData(Qt::ToolButtonTextBesideIcon);
	ui.actionIconOnly->setData(Qt::ToolButtonIconOnly);

	connect(ui.msgList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(msgfilelistWidgetCostumPopupMenu(QPoint)));
	connect(ui.expandFilesButton, SIGNAL(clicked()), this, SLOT(togglefileview()));
	connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(getallrecommended()));
	connect(ui.msgText, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
	connect(ui.sendInviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));
	connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(expandTo()));

	connect(ui.replyButton, SIGNAL(clicked()), this, SLOT(reply()));
	connect(ui.replyallButton, SIGNAL(clicked()), this, SLOT(replyAll()));
	connect(ui.forwardButton, SIGNAL(clicked()), this, SLOT(forward()));
	connect(ui.deleteButton, SIGNAL(clicked()), this, SLOT(remove()));
	
	connect(ui.actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
	connect(ui.actionPrintPreview, SIGNAL(triggered()), this, SLOT(printPreview()));
	connect(ui.actionIconOnly, SIGNAL(triggered()), this, SLOT(buttonStyle()));
	connect(ui.actionTextBesideIcon, SIGNAL(triggered()), this, SLOT(buttonStyle()));
	
	QAction *viewsource = new QAction(tr("View source"), this);
	viewsource->setShortcut(QKeySequence("CTRL+O"));
	connect(viewsource, SIGNAL(triggered()), this, SLOT(viewSource()));

	ui.imageBlockWidget->addButtonAction(tr("Load images always for this message"), this, SLOT(loadImagesAlways()), true);
	ui.msgText->setImageBlockWidget(ui.imageBlockWidget);

	/* hide the Tree +/- */
	ui.msgList->setRootIsDecorated( false );
	ui.msgList->setSelectionMode( QAbstractItemView::ExtendedSelection );

	/* Set header resize modes and initial section sizes */
	QHeaderView * msglheader = ui.msgList->header () ;
	QHeaderView_setSectionResizeModeColumn(msglheader, COLUMN_FILE_NAME, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(msglheader, COLUMN_FILE_SIZE, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(msglheader, COLUMN_FILE_HASH, QHeaderView::Interactive);

	msglheader->resizeSection (COLUMN_FILE_NAME, 200);
	msglheader->resizeSection (COLUMN_FILE_SIZE, 100);
	msglheader->resizeSection (COLUMN_FILE_HASH, 200);

	QMenu *moremenu = new QMenu();
	moremenu->addAction(viewsource);
	moremenu->addAction(ui.actionSaveAs);
	moremenu->addAction(ui.actionPrint);
	moremenu->addAction(ui.actionPrintPreview);
	moremenu->addSeparator();
	moremenu->addAction(ui.actionTextBesideIcon);
	moremenu->addAction(ui.actionIconOnly);
	ui.moreButton->setMenu(moremenu);

	QFont font = QFont("Arial", 10, QFont::Bold);
	ui.subjectText->setFont(font);

	ui.trans_ToText->setMaximumHeight(ui.trans_ToText->fontMetrics().lineSpacing()*1.5);
	ui.ccLabel->setVisible(false);
	ui.trans_CCText->setVisible(false);
	ui.trans_CCText->setMaximumHeight(ui.trans_CCText->fontMetrics().lineSpacing()*1.5);
	ui.bccLabel->setVisible(false);
	ui.trans_BCCText->setVisible(false);
	ui.trans_BCCText->setMaximumHeight(ui.trans_BCCText->fontMetrics().lineSpacing()*1.5);

	ui.tagsLabel->setVisible(false);

	ui.msgText->activateLinkClick(false);

	if (isControlled == false) {
		processSettings("MessageWidget", true);
	}

	ui.dateText-> setText("");
	
	ui.info_Frame_Invite->hide();

	mEventHandlerId = 0;
	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }); }, mEventHandlerId, RsEventType::MAIL_STATUS );
}

MessageWidget::~MessageWidget()
{
	if (isControlled == false) {
		processSettings("MessageWidget", false);
	}

	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void MessageWidget::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if(event->mType != RsEventType::MAIL_STATUS) {
		return;
	}

	const RsMailStatusEvent *fe = dynamic_cast<const RsMailStatusEvent*>(event.get());
	if (!fe) {
		return;
	}

	switch (fe->mMailStatusEventCode) {
	case RsMailStatusEventCode::MESSAGE_REMOVED:
		if (fe->mChangedMsgIds.find(currMsgId) != fe->mChangedMsgIds.end()) {
			if (isControlled) {
				/* processed by MessagesDialog */
				return;
			}

			/* messages was removed */
			if (isWindow) {
				window()->close();
			} else {
				deleteLater();
			}
		}
		break;
	case RsMailStatusEventCode::TAG_CHANGED:
		messagesTagsChanged();
		break;
	case RsMailStatusEventCode::MESSAGE_SENT:
	case RsMailStatusEventCode::MESSAGE_CHANGED:
	case RsMailStatusEventCode::NEW_MESSAGE:
	case RsMailStatusEventCode::MESSAGE_RECEIVED_ACK:
	case RsMailStatusEventCode::SIGNATURE_FAILED:
		break;
	}
}

void MessageWidget::connectAction(enumActionType actionType, QToolButton* button)
{
	switch (actionType) {
	case ACTION_REMOVE:
		connect(button, SIGNAL(clicked()), this, SLOT(remove()));
		break;
	case ACTION_REPLY:
		connect(button, SIGNAL(clicked()), this, SLOT(reply()));
		toolButtonReply = button;
		break;
	case ACTION_REPLY_ALL:
		connect(button, SIGNAL(clicked()), this, SLOT(replyAll()));
		break;
	case ACTION_FORWARD:
		connect(button, SIGNAL(clicked()), this, SLOT(forward()));
		break;
	case ACTION_PRINT:
		connect(button, SIGNAL(clicked()), this, SLOT(print()));
		break;
	case ACTION_PRINT_PREVIEW:
		connect(button, SIGNAL(clicked()), this, SLOT(printPreview()));
		break;
	case ACTION_SAVE_AS:
		connect(button, SIGNAL(clicked()), this, SLOT(saveAs()));
		break;
	}
}

void MessageWidget::connectAction(enumActionType actionType, QAction *action)
{
	switch (actionType) {
	case ACTION_REMOVE:
		connect(action, SIGNAL(triggered()), this, SLOT(remove()));
		break;
	case ACTION_REPLY:
		connect(action, SIGNAL(triggered()), this, SLOT(reply()));
		break;
	case ACTION_REPLY_ALL:
		connect(action, SIGNAL(triggered()), this, SLOT(replyAll()));
		break;
	case ACTION_FORWARD:
		connect(action, SIGNAL(triggered()), this, SLOT(forward()));
		break;
	case ACTION_PRINT:
		connect(action, SIGNAL(triggered()), this, SLOT(print()));
		break;
	case ACTION_PRINT_PREVIEW:
		connect(action, SIGNAL(triggered()), this, SLOT(printPreview()));
		break;
	case ACTION_SAVE_AS:
		connect(action, SIGNAL(triggered()), this, SLOT(saveAs()));
		break;
	}
}

void MessageWidget::processSettings(const QString &settingsGroup, bool load)
{
	Settings->beginGroup(settingsGroup);

	if (load) {
		// load settings

		// expandFiles
		expandFiles = Settings->value("expandFiles", false).toBool();
		
		// toolbar button style
		Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->value("ToolButon_Style", Qt::ToolButtonTextBesideIcon).toInt();
		setToolbarButtonStyle(style);
	} else {
		// save settings

		// expandFiles
		Settings->setValue("expandFiles", expandFiles);

		//toolbar button style
		Settings->setValue("ToolButon_Style", ui.replyButton->toolButtonStyle());
	}

	Settings->endGroup();
}

QString MessageWidget::subject(bool noEmpty)
{
	QString subject = ui.subjectText->text();
	if (subject.isEmpty() && noEmpty) {
		return "[" + tr("No subject") + "]";
	}

	return subject;
}

void MessageWidget::msgfilelistWidgetCostumPopupMenu( QPoint /*point*/ )
{
	QMenu contextMnu(this);

    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DOWNLOAD), tr("Download"), this, SLOT(getcurrentrecommended()));
    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DOWNLOADALL), tr("Download all"), this, SLOT(getallrecommended()));

	contextMnu.exec(QCursor::pos());
}

void MessageWidget::togglefileview(bool noUpdate/*=false*/)
{
	/* if msg header visible -> change icon and tooltip
	* three widgets...
	*/

	if (ui.expandFilesButton->isChecked()) {
        ui.expandFilesButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui.expandFilesButton->setToolTip(tr("Hide the attachment pane"));
	} else {
        ui.expandFilesButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		ui.expandFilesButton->setToolTip(tr("Show the attachment pane"));
	}
	if (!noUpdate)
		expandFiles = ui.expandFilesButton->isChecked();

	ui.msgList->setVisible(ui.expandFilesButton->isChecked());
}

/* download the recommendations... */
void MessageWidget::getcurrentrecommended()
{
	MessageInfo msgInfo;
	if (rsMail->getMessage(currMsgId, msgInfo) == false) {
		return;
	}

    std::list<RsPeerId> srcIds;
    if(msgInfo.from.type()==MsgAddress::MSG_ADDRESS_TYPE_RSPEERID)
        srcIds.push_back(msgInfo.from.toRsPeerId());

	QModelIndexList list = ui.msgList->selectionModel()->selectedIndexes();

	std::map<int,FileInfo> files ;

	for (auto &it : list) {
		FileInfo& fi(files[it.row()]) ;

		switch (it.column()) {
		case COLUMN_FILE_NAME:
			fi.fname = it.data().toString().toUtf8().constData();
			break ;
		case COLUMN_FILE_SIZE:
			fi.size = it.data(Qt::UserRole).toULongLong() ;
			break ;
		case COLUMN_FILE_HASH:
			fi.hash = RsFileHash(it.data().toString().toStdString()) ;
			break ;
		}
	}

	for(std::map<int,FileInfo>::const_iterator it(files.begin());it!=files.end();++it) {
		const FileInfo& fi(it->second) ;
		std::cout << "Requesting file " << fi.fname << ", size=" << fi.size << ", hash=" << fi.hash << std::endl ;

		if (rsFiles->FileRequest(fi.fname, fi.hash, fi.size, "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds) == false) {
			QMessageBox mb(QObject::tr("File Request canceled"), QObject::tr("The following has not been added to your download list, because you already have it:")+"\n    " + QString::fromUtf8(fi.fname.c_str()), QMessageBox::Critical, QMessageBox::Ok, 0, 0);
			mb.exec();
		}
	}
}

void MessageWidget::getallrecommended()
{
	/* get Message */
	MessageInfo msgInfo;
	if (rsMail->getMessage(currMsgId, msgInfo) == false) {
		return;
	}

	const std::list<FileInfo> &recList = msgInfo.files;
	std::list<FileInfo>::const_iterator it;

	/* do the requests */
	for(it = recList.begin(); it != recList.end(); ++it) {
		std::cerr << "MessageWidget::getallrecommended() Calling File Request" << std::endl;
        std::list<RsPeerId> srcIds;

        if(msgInfo.from.type()==MsgAddress::MSG_ADDRESS_TYPE_RSPEERID)
            srcIds.push_back(msgInfo.from.toRsPeerId());

		rsFiles->FileRequest(it->fname, it->hash, it->size, "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
	}
}

void MessageWidget::messagesTagsChanged()
{
    showTagLabels();
}

void MessageWidget::clearTagLabels()
{
	/* clear all tags */
	qDeleteAll(tagLabels);
	tagLabels.clear();

	misc::clearLayout(ui.tagLayout);

	ui.tagsLabel->setVisible(false);
}

void MessageWidget::showTagLabels()
{
	clearTagLabels();

	if (currMsgId.empty()) {
		return;
	}

	MsgTagInfo tagInfo;
	rsMail->getMessageTag(currMsgId, tagInfo);

    if (!tagInfo.empty())
    {
		ui.tagsLabel->setVisible(true);

		MsgTagType Tags;
		rsMail->getMessageTagTypes(Tags);

		std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        for (auto tag:tagInfo)
        {
            Tag = Tags.types.find(tag);
			if (Tag != Tags.types.end()) {
				QLabel *tagLabel = new QLabel(TagDefs::name(Tag->first, Tag->second.first), this);
				tagLabel->setMaximumHeight(16);
				tagLabel->setStyleSheet(TagDefs::labelStyleSheet(Tag->second.second));
				tagLabels.push_back(tagLabel);
				ui.tagLayout->addWidget(tagLabel);
				ui.tagLayout->addSpacing(3);
			}
		}
		ui.tagLayout->addStretch();
	} else {
		ui.tagsLabel->setVisible(false);
	}
}

void MessageWidget::refill()
{
	//uint32_t msg_id = currMsgId ;
	//currMsgId = 0 ;

	//fill(msg_id) ;
}
void MessageWidget::fill(const std::string &msgId)
{
//	if (currMsgId == msgId) {
//		// message doesn't changed
//		return;
//	}

	currMsgId = msgId;

	if (currMsgId.empty()) {
		/* blank it */
		ui.dateText-> setText("");
		ui.trans_ToText->setText("");
		ui.fromText->setText("");
		ui.filesText->setText("");

		ui.ccLabel->setVisible(false);
		ui.trans_CCText->setVisible(false);
		ui.trans_CCText->clear();

		ui.bccLabel->setVisible(false);
		ui.trans_BCCText->setVisible(false);
		ui.trans_BCCText->clear();

		ui.subjectText->setText("");
		ui.msgList->clear();
		ui.msgText->clear();
		ui.msgText->resetImagesStatus(false);

		clearTagLabels();
		checkLength();

		ui.info_Frame_Invite->hide();
		ui.expandFilesButton->setChecked(false);
		ui.downloadButton->setEnabled(false);
		togglefileview(true);

		ui.replyButton->setEnabled(false);
		ui.replyallButton->setEnabled(false);
		ui.forwardButton->setEnabled(false);
		ui.deleteButton->setEnabled(false);
		ui.moreButton->setEnabled(false);

		ui.expandButton->hide();
		ui.trans_ToText->setMaximumHeight(ui.trans_ToText->fontMetrics().lineSpacing()*1.5);

		currMsgFlags = 0;

		return;
	}

	clearTagLabels();


	ui.replyButton->setEnabled(true);
	ui.replyallButton->setEnabled(true);
	ui.forwardButton->setEnabled(true);
	ui.deleteButton->setEnabled(true);
	ui.moreButton->setEnabled(true);

	MessageInfo msgInfo;
	if (rsMail->getMessage(currMsgId, msgInfo) == false) {
		std::cerr << "MessageWidget::fill() Couldn't find Msg" << std::endl;
		return;
	}
	
    if (msgInfo.msgflags & RS_MSG_USER_REQUEST)
        if(msgInfo.from.type() == MsgAddress::MSG_ADDRESS_TYPE_RSPEERID)
        {
            ui.info_Frame_Invite->show();
            ui.sendInviteButton->hide();
            ui.infoLabel_Invite->setText(tr("You got an invite to make friend! You may accept this request."));
        }
        else
        {
            ui.info_Frame_Invite->show();
            ui.sendInviteButton->show();
            ui.infoLabel_Invite->setText(tr("You got an invite to make friend! You may accept this request and send your own Certificate back"));
        }
    else
        ui.info_Frame_Invite->hide();

	const std::list<FileInfo> &recList = msgInfo.files;
	std::list<FileInfo>::const_iterator it;

	ui.msgList->clear();

	QList<QTreeWidgetItem*> items;
	for (it = recList.begin(); it != recList.end(); ++it) {
		QTreeWidgetItem *item = new QTreeWidgetItem;
		item->setText(COLUMN_FILE_NAME, QString::fromUtf8(it->fname.c_str()));
		item->setIcon(COLUMN_FILE_NAME, FilesDefs::getIconFromFileType(it->fname.c_str()));
		item->setText(COLUMN_FILE_SIZE, misc::friendlyUnit(it->size));
		item->setData(COLUMN_FILE_SIZE, Qt::UserRole, QVariant(qulonglong(it->size)) );
		item->setText(COLUMN_FILE_HASH, QString::fromStdString(it->hash.toStdString()));
		item->setTextAlignment( COLUMN_FILE_SIZE, Qt::AlignRight );

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	ui.msgList->insertTopLevelItems(0, items);
	ui.expandFilesButton->setChecked(expandFiles && (items.count()>0) );
	ui.downloadButton->setEnabled(items.count()>0);
	togglefileview(true);

	/* iterate through the sources */
	RetroShareLink link;
    QString to_text,cc_text,bcc_text;

    for(auto m:msgInfo.destinations)
    {
        if(m.type()==MsgAddress::MSG_ADDRESS_TYPE_RSGXSID)
            link = RetroShareLink::createMessage(m.toGxsId(), "");
        else
            link = RetroShareLink::createMessage(m.toRsPeerId(), "");

		if (link.valid())
            switch(m.mode())
            {
            case MsgAddress::MSG_ADDRESS_MODE_TO: to_text += link.toHtml() + "   "; break;
            case MsgAddress::MSG_ADDRESS_MODE_CC: cc_text += link.toHtml() + "   "; break;
            case MsgAddress::MSG_ADDRESS_MODE_BCC: bcc_text += link.toHtml() + "   "; break;
            default: break;
            }
    }

    ui.trans_ToText->setText(to_text);

	int recipientsCount = ui.trans_ToText->toPlainText().split(QRegExp("(\\s|\\n|\\r)+"), QString::SkipEmptyParts).count();
	ui.expandButton->setText( QString::number(recipientsCount)+ " " + tr("more"));

	if (recipientsCount >=20) {
		ui.expandButton->show();
	} else {
		ui.expandButton->hide();
		ui.expandButton->setChecked(false);
		//ui.expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui.trans_ToText->setMaximumHeight(ui.trans_ToText->fontMetrics().lineSpacing()*1.5);
	}

    if (!cc_text.isNull())
	{
		ui.ccLabel->setVisible(true);
		ui.trans_CCText->setVisible(true);

        ui.trans_CCText->setText(cc_text);
	} else {
		ui.ccLabel->setVisible(false);
		ui.trans_CCText->setVisible(false);
		ui.trans_CCText->clear();
	}

    if (!bcc_text.isNull())
	{
		ui.bccLabel->setVisible(true);
		ui.trans_BCCText->setVisible(true);

        ui.trans_BCCText->setText(bcc_text);
	} else {
		ui.bccLabel->setVisible(false);
		ui.trans_BCCText->setVisible(false);
		ui.trans_BCCText->clear();
	}

	ui.dateText->setText(DateTime::formatDateTime(msgInfo.ts));

    RsPeerId ownId = rsPeers->getOwnId();
	QString tooltip_string ;

//	if ((msgInfo.msgflags & RS_MSG_BOXMASK) == RS_MSG_OUTBOX) // outgoing message are from me
//	{
//		tooltip_string = PeerDefs::rsidFromId(ownId) ;
//		link.createMessage(ownId, "");
//	}

    if(msgInfo.from.type()==Rs::Msgs::MsgAddress::MSG_ADDRESS_TYPE_RSGXSID)	// distant message
	{
        tooltip_string = PeerDefs::rsidFromId(msgInfo.from.toGxsId()) ;
        link = RetroShareLink::createMessage(msgInfo.from.toGxsId(), "");
	}
	else
	{
        tooltip_string = PeerDefs::rsidFromId(msgInfo.from.toRsPeerId()) ;
        link = RetroShareLink::createMessage(msgInfo.from.toRsPeerId(), "");
	}

    if ((msgInfo.msgflags & RS_MSG_SYSTEM) && msgInfo.from.toRsPeerId() == ownId)
    {
		ui.fromText->setText("[Notification]");
		if (toolButtonReply) toolButtonReply->setEnabled(false);
    }
    else
    {
		ui.fromText->setText(link.toHtml());
		ui.fromText->setToolTip(tooltip_string) ;
		if (toolButtonReply) toolButtonReply->setEnabled(true);
	}

	ui.subjectText->setText(QString::fromUtf8(msgInfo.title.c_str()));
	
	unsigned int formatTextFlag = RSHTML_FORMATTEXT_EMBED_LINKS ;

	// embed smileys ?
	if (Settings->valueFromGroup(QString("Messages"), QString::fromUtf8("Emoticons"), true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS ;
	}
    QString text = RsHtmlMsg(msgInfo.msgflags).formatText(ui.msgText->document(), QString::fromUtf8(msgInfo.msg.c_str()), formatTextFlag);
	ui.msgText->resetImagesStatus(Settings->getMsgLoadEmbeddedImages() || (msgInfo.msgflags & RS_MSG_LOAD_EMBEDDED_IMAGES));
	ui.msgText->setHtml(text);

	ui.filesText->setText(QString("%1").arg(msgInfo.count));
	ui.filesSize->setText(QString(misc::friendlyUnit(msgInfo.size)));

	showTagLabels();
	checkLength();

	currMsgFlags = msgInfo.msgflags;
}

void MessageWidget::remove()
{
	MessageInfo msgInfo;
	if (rsMail->getMessage(currMsgId, msgInfo) == false) {
		std::cerr << "MessageWidget::fill() Couldn't find Msg" << std::endl;
		return;
	}

	bool deleteReal = false;
	if (msgInfo.msgflags & RS_MSG_TRASH) {
		deleteReal = true;
	} else {
		if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
			deleteReal = true;
		}
	}

	if (deleteReal) {
		rsMail->MessageDelete(currMsgId);
	} else {
		rsMail->MessageToTrash(currMsgId, true);
	}

	if (isWindow) {
		window()->close();
	} else {
		if (isControlled) {
			currMsgId.clear();
			fill(currMsgId);
		} else {
			deleteLater();
		}
	}

	emit messageRemoved();
}

void MessageWidget::print()
{
#ifndef QT_NO_PRINTER
	QPrinter printer(QPrinter::HighResolution);
	printer.setFullPage(true);
	QPrintDialog *dlg = new QPrintDialog(&printer, this);
	if (ui.msgText->textCursor().hasSelection())
		dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
	dlg->setWindowTitle(tr("Print Document"));
	if (dlg->exec() == QDialog::Accepted) {
		ui.msgText->print(&printer);
	}
	delete dlg;
#endif
}

void MessageWidget::printPreview()
{
	PrintPreview *preview = new PrintPreview(ui.msgText->document(), this);
	preview->setWindowModality(Qt::WindowModal);
	preview->setAttribute(Qt::WA_DeleteOnClose);
	preview->show();

	/* window will destroy itself! */
}

void MessageWidget::saveAs()
{
	QString filename;
	if (misc::getSaveFileName(window(), RshareSettings::LASTDIR_MESSAGES, tr("Save as..."), tr("HTML-Files (*.htm *.html);;All Files (*)"), filename)) {
		QFile file(filename);
		if (!file.open(QFile::WriteOnly))
			return;
		QTextStream ts(&file);
		ts.setCodec(QTextCodec::codecForName("UTF-8"));
		ts << ui.msgText->document()->toHtml("UTF-8");
		ui.msgText->document()->setModified(false);
	}
}

void MessageWidget::reply()
{
	/* put msg on msgBoard, and switch to it. */

	if (currMsgId.empty()) {
		return;
	}

	MessageComposer *msgComposer = MessageComposer::replyMsg(currMsgId, false);
	if (msgComposer == NULL) {
		return;
	}

	msgComposer->show();
	msgComposer->activateWindow();

	/* window will destroy itself! */
}

void MessageWidget::replyAll()
{
	/* put msg on msgBoard, and switch to it. */

	if (currMsgId.empty()) {
		return;
	}

	MessageComposer *msgComposer = MessageComposer::replyMsg(currMsgId, true);
	if (msgComposer == NULL) {
		return;
	}

	msgComposer->show();
	msgComposer->activateWindow();

	/* window will destroy itself! */
}

void MessageWidget::forward()
{
	/* put msg on msgBoard, and switch to it. */

	if (currMsgId.empty()) {
		return;
	}

	MessageComposer *msgComposer = MessageComposer::forwardMsg(currMsgId);
	if (msgComposer == NULL) {
		return;
	}

	msgComposer->show();
	msgComposer->activateWindow();

	/* window will destroy itself! */
}

void MessageWidget::anchorClicked(const QUrl &url)
{
	RetroShareLink link(url);

	if(link.valid())
	{
		if (link.type() == RetroShareLink::TYPE_CERTIFICATE && currMsgFlags & RS_MSG_USER_REQUEST) {
			std::cerr << "(WW) Calling some disabled code in MessageWidget::anchorClicked(). Please contact the developpers." << std::endl;
			//	link.setSubType(RSLINK_SUBTYPE_CERTIFICATE_USER_REQUEST);
		}

		QList<RetroShareLink> links;
		links.append(link);
		RetroShareLink::process(links);
	}
    else
		QDesktopServices::openUrl(url) ;
}

void MessageWidget::loadImagesAlways()
{
	if (currMsgId.empty()) {
		return;
	}

	rsMail->MessageLoadEmbeddedImages(currMsgId, true);
}

void MessageWidget::sendInvite()
{
    MessageInfo mi;

    if (!rsMail)
        return;

    if (!rsMail->getMessage(currMsgId, mi))
        return;

    if(mi.from.type()!=MsgAddress::MSG_ADDRESS_TYPE_RSGXSID)
        return;

    if ((QMessageBox::question(this, tr("Send invite?"),tr("Do you really want send a invite with your Certificate?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Cancel))== QMessageBox::Yes)
    {
        MessageComposer::sendInvite(mi.from.toGxsId(),false);
    }
}

void MessageWidget::setToolbarButtonStyle(Qt::ToolButtonStyle style)
{
	ui.deleteButton->setToolButtonStyle(style);
	ui.replyButton->setToolButtonStyle(style);
	ui.replyallButton->setToolButtonStyle(style);
	ui.forwardButton->setToolButtonStyle(style);
	ui.moreButton->setToolButtonStyle(style);
}

void MessageWidget::buttonStyle()
{
	setToolbarButtonStyle((Qt::ToolButtonStyle) dynamic_cast<QAction*>(sender())->data().toInt());
}

void MessageWidget::viewSource()
{
	QDialog *dialog = new QDialog(this);
	QPlainTextEdit *pte = new QPlainTextEdit(dialog);
	pte->setPlainText( ui.msgText->toHtml() );
	QGridLayout *gl = new QGridLayout(dialog);
	gl->addWidget(pte,0,0,1,1);
	dialog->setWindowTitle(tr("Document source"));
	dialog->resize(500, 400);

	dialog->exec();

	ui.msgText->setHtml(pte->toPlainText());

	delete dialog;
}

void MessageWidget::checkLength()
{
	QString text;
	RsHtml::optimizeHtml(ui.msgText, text);
	std::wstring msg = text.toStdWString();
	int charlength = msg.length();

	text = tr("%1 (%2) ").arg(charlength).arg(misc::friendlyUnit(charlength));

	ui.sizeLabel->setText(text);
}

void MessageWidget::expandTo()
{
	if (ui.expandButton->isChecked()) {
		ui.trans_ToText->setMaximumHeight(ui.trans_ToText->fontMetrics().lineSpacing()*3.5);
		ui.expandButton->setToolTip(tr("Show less"));
	} else {
		ui.trans_ToText->setMaximumHeight(ui.trans_ToText->fontMetrics().lineSpacing()*1.5);
		ui.expandButton->setToolTip(tr("Show more"));
	}
}
