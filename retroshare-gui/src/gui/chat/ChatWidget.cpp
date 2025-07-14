/*******************************************************************************
 * gui/chat/ChatWidget.cpp                                                     *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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
#include "ChatWidget.h"
#include "ui_ChatWidget.h"

#include "gui/MainWindow.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/rsettingswin.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/Emoticons.h"
#include "gui/chat/ChatLobbyDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "util/HandleRichText.h"
#include "gui/chat/ChatUserNotify.h"//For BradCast
#include "util/DateTime.h"
#include "util/imageutil.h"
#include "gui/im_history/ImHistoryBrowser.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsplugin.h>

#include <QApplication>
#include <QBuffer>
#include <QColorDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextCodec>
#include <QTextDocumentFragment>
#include <QTextStream>
#include <QTimer>
#include <QToolTip>
#include <QInputDialog>

#include <time.h>

#define FMM 2.5//fontMetricsMultiplicator
#define FMM_SMALLER 1.8
#define FMM_THRESHOLD 25

/*****
 * #define CHAT_DEBUG 1
 *****/

ChatWidget::ChatWidget(QWidget *parent)
  : QWidget(parent)
  , completionPosition(0), newMessages(false), typing(false), peerStatus(0)
  , sendingBlocked(false), useCMark(false)
  , lastStatusSendTime(0)
  , firstShow(true), inChatCharFormatChanged(false), firstSearch(true)
  , lastUpdateCursorPos(0), lastUpdateCursorEnd(0)
  , completer(NULL), imBrowser(NULL), notify(NULL)
  , ui(new Ui::ChatWidget)
{
	ui->setupUi(this);

	int iconHeight = QFontMetricsF(font()).height();
	double fmm = iconHeight > FMM_THRESHOLD ? FMM : FMM_SMALLER;
	iconHeight *= fmm;
	QSize iconSize = QSize(iconHeight, iconHeight);
	//int butt_size(iconSize.height() + fmm);
	//QSize buttonSize = QSize(butt_size, butt_size);

	lastMsgDate = QDate::currentDate();

	//Resize Tool buttons
	//ui->emoteiconButton->setFixedSize(buttonSize);
	ui->emoteiconButton->setIconSize(iconSize);
	//ui->stickerButton->setFixedSize(buttonSize);
	ui->stickerButton->setIconSize(iconSize);
	//ui->attachPictureButton->setFixedSize(buttonSize);
	ui->attachPictureButton->setIconSize(iconSize);
	//ui->addFileButton->setFixedSize(buttonSize);
	ui->addFileButton->setIconSize(iconSize);
	//ui->pushtoolsButton->setFixedSize(buttonSize);
	ui->pushtoolsButton->setIconSize(iconSize);
	//ui->notifyButton->setFixedSize(buttonSize);
	ui->notifyButton->setIconSize(iconSize);
	//ui->markButton->setFixedSize(buttonSize);
	ui->markButton->setIconSize(iconSize);
	ui->leSearch->setFixedHeight(iconHeight);
	ui->searchBefore->setFixedHeight(iconHeight);
	ui->searchAfter->setFixedHeight(iconHeight);
	//ui->searchButton->setFixedSize(buttonSize);
	ui->searchButton->setIconSize(iconSize);
	//ui->sendButton->setFixedHeight(iconHeight);
	ui->sendButton->setIconSize(iconSize);
	ui->typingLabel->setMaximumHeight(QFontMetricsF(font()).height()*1.2);
	ui->fontcolorButton->setIconSize(iconSize);

	//Initialize search
	iCharToStartSearch=Settings->getChatSearchCharToStartSearch();
	bFindCaseSensitively=Settings->getChatSearchCaseSensitively();
	bFindWholeWords=Settings->getChatSearchWholeWords();
	bMoveToCursor=Settings->getChatSearchMoveToCursor();
	bSearchWithoutLimit=Settings->getChatSearchSearchWithoutLimit();
	uiMaxSearchLimitColor=Settings->getChatSearchMaxSearchLimitColor();
	cFoundColor=Settings->getChatSearchFoundColor();

	ui->actionSearchWithoutLimit->setText(tr("Don't stop to color after")+" "+QString::number(uiMaxSearchLimitColor)+" "+tr("items found (need more CPU)"));

	ui->markButton->setVisible(false);
	ui->leSearch->setVisible(false);
	ui->searchBefore->setVisible(false);
	ui->searchBefore->setToolTip(tr("<b>Find Previous </b><br/><i>Ctrl+Shift+G</i>"));
	ui->searchAfter->setVisible(false);
	ui->searchAfter->setToolTip(tr("<b>Find Next </b><br/><i>Ctrl+G</i>"));
	ui->searchButton->setCheckable(true);
	ui->searchButton->setChecked(false);
	ui->searchButton->setToolTip(tr("<b>Find </b><br/><i>Ctrl+F</i>"));
	ui->leSearch->installEventFilter(this);

	connect(ui->actionFindCaseSensitively, SIGNAL(triggered()), this, SLOT(toogle_FindCaseSensitively()));
	connect(ui->actionFindWholeWords, SIGNAL(triggered()), this, SLOT(toogle_FindWholeWords()));
	connect(ui->actionMoveToCursor, SIGNAL(triggered()), this, SLOT(toogle_MoveToCursor()));
	connect(ui->actionSearchWithoutLimit, SIGNAL(triggered()), this, SLOT(toogle_SeachWithoutLimit()));
	connect(ui->searchButton, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuSearchButton(QPoint)));

	ui->notifyButton->setVisible(false);

	ui->markButton->setToolTip(tr("<b>Mark this selected text</b><br><i>Ctrl+M</i>"));

	connect(ui->emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
	connect(ui->stickerButton, SIGNAL(clicked()), this, SLOT(stickerWidget()));
	connect(ui->attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
	connect(ui->addFileButton, SIGNAL(clicked()), this , SLOT(addExtraFile()));
	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendChat()));

	connect(ui->actionSaveChatHistory, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(ui->actionClearChatHistory, SIGNAL(triggered()), this, SLOT(clearChatHistory()));
	connect(ui->actionDeleteChatHistory, SIGNAL(triggered()), this, SLOT(deleteChatHistory()));
	connect(ui->actionMessageHistory, SIGNAL(triggered()), this, SLOT(messageHistory()));
	connect(ui->actionChooseFont, SIGNAL(triggered()), this, SLOT(chooseFont()));
	connect(ui->actionChooseColor, SIGNAL(triggered()), this, SLOT(chooseColor()));
	connect(ui->actionResetFont, SIGNAL(triggered()), this, SLOT(resetFont()));
	connect(ui->actionQuote, SIGNAL(triggered()), this, SLOT(quote()));
	connect(ui->actionDropPlacemark, SIGNAL(triggered()), this, SLOT(dropPlacemark()));
	connect(ui->actionImport_sticker, SIGNAL(triggered()), this, SLOT(saveSticker()));
	connect(ui->actionShow_Hidden_Images, SIGNAL(triggered()), ui->textBrowser, SLOT(showImages()));
	ui->actionShow_Hidden_Images->setIcon(ui->textBrowser->getBlockedImage());

	connect(ui->hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));
	connect(NotifyQt::getInstance(), SIGNAL(chatFontChanged()), this, SLOT(resetFonts()));

	connect(ui->textBrowser, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTextBrowser(QPoint)));

	//connect(ui->chatTextEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
	// reset text and color after removing all characters from the QTextEdit and after calling QTextEdit::clear
	connect(ui->chatTextEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(chatCharFormatChanged()));
	connect(ui->chatTextEdit, SIGNAL(textChanged()), this, SLOT(updateLenOfChatTextEdit()));

	ui->info_Frame->setVisible(false);
	ui->statusMessageLabel->hide();

	setAcceptDrops(true);
	ui->chatTextEdit->setAcceptDrops(false);
	ui->hashBox->setDropWidget(this);
	ui->hashBox->setAutoHide(true);

	QMenu *fontmenu = new QMenu();
	fontmenu->addAction(ui->actionChooseFont);
	fontmenu->addAction(ui->actionChooseColor);
	fontmenu->addAction(ui->actionResetFont);
	fontmenu->addAction(ui->actionNoEmbed);
	fontmenu->addAction(ui->actionSendAsPlainText);
	#ifdef USE_CMARK
	fontmenu->addAction(ui->actionSend_as_CommonMark);
	#endif
	ui->fontcolorButton->setMenu(fontmenu);

	QMenu *menu = new QMenu();
	menu->addAction(ui->actionMessageHistory);
	menu->addSeparator();
	menu->addAction(ui->actionSaveChatHistory);
	menu->addAction(ui->actionClearChatHistory);
	menu->addAction(ui->actionDeleteChatHistory);

	ui->pushtoolsButton->setMenu(menu);

	ui->actionSendAsPlainText->setChecked(Settings->getChatSendAsPlainTextByDef());
	ui->chatTextEdit->setOnlyPlainText(ui->actionSendAsPlainText->isChecked());
	connect(ui->actionSendAsPlainText, SIGNAL(toggled(bool)), ui->chatTextEdit, SLOT(setOnlyPlainText(bool)) );

#ifdef USE_CMARK
	connect(ui->actionSend_as_CommonMark, SIGNAL(toggled(bool)), this, SLOT(setUseCMark(bool)) );
	connect(ui->chatTextEdit, SIGNAL(textChanged()), this, SLOT(updateCMPreview()) );
#endif
	ui->cmPreview->setVisible(false);

	ui->textBrowser->resetImagesStatus(Settings->getChatLoadEmbeddedImages());
	ui->textBrowser->installEventFilter(this);
	ui->textBrowser->viewport()->installEventFilter(this);
	ui->chatTextEdit->installEventFilter(this);
	//ui->textBrowser->setMouseTracking(true);
	//ui->chatTextEdit->setMouseTracking(true);

#if QT_VERSION < 0x040700
	// embedded images are not supported before QT 4.7.0
	ui->attachPictureButton->setVisible(false);
#endif

	resetStatusBar();

    completer = new QCompleter(this);
    //completer->setModel(modelFromPeers()); //No peers at this point.
    completer->setModelSorting(QCompleter::UnsortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    ui->chatTextEdit->setCompleter(completer);
    ui->chatTextEdit->setCompleterKeyModifiers(Qt::ControlModifier);
    ui->chatTextEdit->setCompleterKey(Qt::Key_Space);

//#ifdef ENABLE_DISTANT_CHAT_AND_MSGS
//	contextMnu->addSeparator();
//    QAction *action = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"), tr("Paste/Create private chat or Message link..."), this);
//    connect(action, SIGNAL(triggered()), this, SLOT(pasteCreateMsgLink()));
//    ui->chatTextEdit->addContextMenuAction(action);
//#endif
}

ChatWidget::~ChatWidget()
{
	processSettings(false);

	/* Cleanup plugin functions */
	foreach (ChatWidgetHolder *chatWidgetHolder, mChatWidgetHolder) {
		delete(chatWidgetHolder);
	}

	delete ui;
}

void ChatWidget::setDefaultExtraFileFlags(TransferRequestFlags fl)
{
	mDefaultExtraFileFlags = fl ;
	ui->hashBox->setDefaultTransferRequestFlags(fl) ;
}

void ChatWidget::addChatHorizontalWidget(QWidget *w)
{
	ui->pluginsVLayout->addWidget(w) ;
	update() ;
}

void ChatWidget::addChatBarWidget(QWidget *w)
{
	int iconHeight = QFontMetricsF(font()).height();
	double fmm = iconHeight > FMM_THRESHOLD ? FMM : FMM_SMALLER;
	iconHeight *= fmm;
	QSize iconSize = QSize(iconHeight, iconHeight);
	int butt_size(iconSize.height() + fmm);
	QSize buttonSize = QSize(butt_size, butt_size);
	w->setFixedSize(buttonSize);
	ui->pluginButtonFrame->layout()->addWidget(w) ;
}

void ChatWidget::addTitleBarWidget(QWidget *w)
{
	ui->trans_Frame_PluginTitle->layout()->addWidget(w) ;
}

void ChatWidget::addTopBarWidget(QWidget *w)
{
	ui->trans_Frame_PluginTop->layout()->addWidget(w) ;
}

void ChatWidget::hideChatText(bool hidden)
{
	ui->chatTextFrame->setHidden(hidden); ;
}

RSButtonOnText* ChatWidget::getNewButtonOnTextBrowser()
{
	return new RSButtonOnText(ui->textBrowser);
}

RSButtonOnText* ChatWidget::getNewButtonOnTextBrowser(QString text)
{
	return new RSButtonOnText(text, ui->textBrowser);
}


void ChatWidget::init(const ChatId &chat_id, const QString &title)
{
    this->chatId = chat_id;
	this->title = title;

	ui->titleLabel->setText(RsHtml::plainText(title));
	ui->chatTextEdit->setMaxBytes(this->maxMessageSize() - 200);

    RsPeerId ownId = rsPeers->getOwnId();
	setName(QString::fromUtf8(rsPeers->getPeerName(ownId).c_str()));

    if(chatId.isPeerId() || chatId.isDistantChatId())
        chatStyle.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);
    if(chatId.isBroadcast() || chatId.isLobbyId())
        chatStyle.setStyleFromSettings(ChatStyle::TYPE_PUBLIC);

    currentColor.setNamedColor(PeerSettings->getPrivateChatColor(chatId));
    currentFont.fromString(PeerSettings->getPrivateChatFont(chatId));

	colorChanged();
	setColorAndFont(true);

	// load style
    PeerSettings->getStyle(chatId, "ChatWidget", style);

	/* Add plugin functions */
	int pluginCount = rsPlugins->nbPlugins();
	for (int i = 0; i < pluginCount; ++i) {
		RsPlugin *plugin = rsPlugins->plugin(i);
		if (plugin) {
			ChatWidgetHolder *chatWidgetHolder = plugin->qt_get_chat_widget_holder(this);
			if (chatWidgetHolder) {
				mChatWidgetHolder.push_back(chatWidgetHolder);
			}
		}
	}

    uint32_t hist_chat_type = 0xFFFF; // a value larger than the biggest RS_HISTORY_TYPE_* value
	int messageCount=0;

	if (chatType() == CHATTYPE_LOBBY) {
		hist_chat_type = RS_HISTORY_TYPE_LOBBY;
		messageCount = Settings->getLobbyChatHistoryCount();

		ui->statusLabel->hide();

		updateTitle();
    } else if (chatType() == CHATTYPE_PRIVATE){
		hist_chat_type = RS_HISTORY_TYPE_PRIVATE ;
		messageCount = Settings->getPrivateChatHistoryCount();

		// initialize first status
		StatusInfo peerStatusInfo;
		// No check of return value. Non existing status info is handled as offline.
        rsStatus->getStatus(chatId.toPeerId(), peerStatusInfo);
        updateStatus(QString::fromStdString(chatId.toPeerId().toStdString()), peerStatusInfo.status);

		// initialize first custom state string
        QString customStateString = QString::fromUtf8(rsMsgs->getCustomStateString(chatId.toPeerId()).c_str());
        updatePeersCustomStateString(QString::fromStdString(chatId.toPeerId().toStdString()), customStateString);
    } else if (chatType() == CHATTYPE_DISTANT){
        hist_chat_type = RS_HISTORY_TYPE_DISTANT ;
        messageCount = Settings->getDistantChatHistoryCount();
    } else if(chatId.isBroadcast()){
        hist_chat_type = RS_HISTORY_TYPE_PUBLIC;
        messageCount = Settings->getPublicChatHistoryCount();

        ui->headerBFrame->setVisible(false);
    }

	if (rsHistory->getEnable(hist_chat_type))
	{
		// get chat messages from history
		std::list<HistoryMsg> historyMsgs;

		if (messageCount > 0)
		{
			rsHistory->getMessages(chatId, historyMsgs, messageCount);

			std::list<HistoryMsg>::iterator historyIt;
			for (historyIt = historyMsgs.begin(); historyIt != historyMsgs.end(); ++historyIt)
			{
				// it can happen that a message is first added to the message history
				// and later the gui receives the message through notify
				// avoid this by not adding history entries if their age is < 2secs
				if (time(nullptr) <= historyIt->recvTime+2)
					continue;

				QString name;
				if (!chatId.isNotSet())
				{
					RsIdentityDetails details;

					if (rsIdentity->getIdDetails(RsGxsId(historyIt->peerId), details))
						name = QString::fromUtf8(details.mNickname.c_str());
					else if(!historyIt->peerName.empty())
						name = QString::fromUtf8(historyIt->peerName.c_str());
					else
						name = QString::fromUtf8(historyIt->peerId.toStdString().c_str());
				} else {
					name = QString::fromUtf8(historyIt->peerId.toStdString().c_str());
				}

				addChatMsg(historyIt->incoming, name, RsGxsId(historyIt->peerId.toStdString().c_str()), QDateTime::fromTime_t(historyIt->sendTime), QDateTime::fromTime_t(historyIt->recvTime), QString::fromUtf8(historyIt->message.c_str()), MSGTYPE_HISTORY);
			}
		}
	}

	processSettings(true);
}

ChatId ChatWidget::getChatId()
{
    return chatId;
}

ChatWidget::ChatType ChatWidget::chatType()
{
    // transformation from ChatId::Type to ChatWidget::ChatType
    // we don't use the type in ChatId directly, because of historic reasons
    // ChatWidget::ChatType existed before ChatId::Type was introduced
    // TODO: check if can change all code to use the type in ChatId directly
    // but maybe it is good to have separate types in libretroshare and gui
    if(chatId.isPeerId())
        return CHATTYPE_PRIVATE;
    if(chatId.isDistantChatId())
        return CHATTYPE_DISTANT;
    if(chatId.isLobbyId())
        return CHATTYPE_LOBBY;

    return CHATTYPE_UNKNOWN;
}

void ChatWidget::blockSending(QString msg)
{
#ifndef RS_ASYNC_CHAT
//	sendingBlocked = true;
//	ui->sendButton->setEnabled(false);
//	ui->stickerButton->setEnabled(false);
#endif
	ui->sendButton->setToolTip(msg);
}

void ChatWidget::unblockSending()
{
    sendingBlocked = false;
	ui->stickerButton->setEnabled(true);
    updateLenOfChatTextEdit();
}

void ChatWidget::processSettings(bool load)
{
	Settings->beginGroup(QString("ChatWidget"));

	if (load) {
		// load settings

		// state of splitter
		ui->chatVSplitter->restoreState(Settings->value("ChatSplitter").toByteArray());
	} else {
		shrinkChatTextEdit(false);
		// save settings

		// state of splitter
		Settings->setValue("ChatSplitter", ui->chatVSplitter->saveState());
	}

	Settings->endGroup();
}

uint32_t ChatWidget::maxMessageSize()
{
	uint32_t maxMessageSize = 0;
	switch (chatType()) {
	case CHATTYPE_UNKNOWN:
		break;
	case CHATTYPE_PRIVATE:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_PRIVATE);
		break;
	case CHATTYPE_LOBBY:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_LOBBY);
		break;
	case CHATTYPE_DISTANT:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_DISTANT);
		break;
	}
	return maxMessageSize;
}

bool ChatWidget::eventFilter(QObject *obj, QEvent *event)
{
	//QEvent::Type type = event->type();
	if (obj == ui->textBrowser || obj == ui->textBrowser->viewport()
	    || obj == ui->leSearch || obj == ui->chatTextEdit) {
		if (event->type() == QEvent::KeyPress) {

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::ControlModifier)
				{
					bool bTextselected=false;
					if (obj == ui->textBrowser )
					{
						if (ui->textBrowser->textCursor().selectedText().length()>0)
						{
							ui->leSearch->setText(ui->textBrowser->textCursor().selectedText());
							bTextselected=true;
						}
					}
					if (obj == ui->chatTextEdit)
					{
						if (ui->chatTextEdit->textCursor().selectedText().length()>0)
						{
							ui->leSearch->setText(ui->chatTextEdit->textCursor().selectedText());
							bTextselected=true;
						}
					}
					ui->searchButton->setChecked(!ui->searchButton->isChecked() || bTextselected);
					ui->leSearch->setVisible(bTextselected);//To discard re-selection of text
					on_searchButton_clicked(ui->searchButton->isChecked());
					return true; // eat event
				}
				if (keyEvent->key() == Qt::Key_G && keyEvent->modifiers() == Qt::ControlModifier)
				{
					if (ui->searchAfter->isVisible())
						on_searchAfter_clicked();
					return true; // eat event
				}
				if (keyEvent->key() == Qt::Key_G && keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
				{
					if (ui->searchBefore->isVisible())
						on_searchBefore_clicked();
					return true; // eat event
				}

			}
		}

		if (notify && chatType() == CHATTYPE_LOBBY) {
			if ((event->type() == QEvent::KeyPress)
			    || (event->type() == QEvent::MouseMove)
			    || (event->type() == QEvent::Enter)
			    || (event->type() == QEvent::Leave)
			    || (event->type() == QEvent::Wheel)
			    || (event->type() == QEvent::ToolTip) ) {

				QTextCursor cursor = ui->textBrowser->cursorForPosition(QPoint(0, 0));
				QPoint bottom_right(ui->textBrowser->viewport()->width() - 1, ui->textBrowser->viewport()->height() - 1);
				int end_pos = ui->textBrowser->cursorForPosition(bottom_right).position();
				cursor.setPosition(end_pos, QTextCursor::KeepAnchor);
				if ((cursor.position() != lastUpdateCursorPos || cursor.selectionEnd() != lastUpdateCursorEnd) &&
				   !cursor.selectedText().isEmpty()) {
					lastUpdateCursorPos = cursor.position();
					lastUpdateCursorEnd = cursor.selectionEnd();
					QRegExp rx("<a name=\"(.*)\"",Qt::CaseSensitive, QRegExp::RegExp2);
					rx.setMinimal(true);
					QString sel=cursor.selection().toHtml();
					QStringList anchors;
					int pos=0;
					while ((pos = rx.indexIn(sel,pos)) != -1) {
						anchors << rx.cap(1);
						pos += rx.matchedLength();
					}
					if (!anchors.isEmpty()){
						for (QStringList::iterator it=anchors.begin();it!=anchors.end();++it) {
							notify->chatLobbyCleared(chatId.toLobbyId(), *it);
						}
					}
				}
			}
		}
	}

    if (obj == ui->textBrowser) {
        if (event->type() == QEvent::KeyPress) {

            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent) {
                if (keyEvent->key() == Qt::Key_Delete) {
					// Delete key pressed
					if (ui->textBrowser->textCursor().selectedText().length() > 0) {
						if (notify && chatType() == CHATTYPE_LOBBY) {
							QRegExp rx("<a name=\"(.*)\"",Qt::CaseSensitive, QRegExp::RegExp2);
							rx.setMinimal(true);
							QString sel=ui->textBrowser->textCursor().selection().toHtml();
							QStringList anchors;
							int pos=0;
							while ((pos = rx.indexIn(sel,pos)) != -1) {
								anchors << rx.cap(1);
								pos += rx.matchedLength();
							}

							for (QStringList::iterator it=anchors.begin();it!=anchors.end();++it) {
								notify->chatLobbyCleared(chatId.toLobbyId(), *it);
							}

						}
                        ui->textBrowser->textCursor().deleteChar();

                }
				}

				if (keyEvent->key() == Qt::Key_M && keyEvent->modifiers() == Qt::ControlModifier)
				{
					on_markButton_clicked(!ui->markButton->isChecked());
				}
            }
        }

		if (event->type() == QEvent::ToolTip)	{
			QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
			QString toolTipText = ui->textBrowser->anchorForPosition(helpEvent->pos());
			if (toolTipText.isEmpty() && !ui->textBrowser->getShowImages()){
				QString imageStr;
				if (ImageUtil::checkImage(ui->textBrowser, helpEvent->pos(), imageStr)) {
					toolTipText = imageStr;
				}
			} else if (toolTipText.startsWith(PERSONID)){
				toolTipText = toolTipText.replace(PERSONID, tr("Person id: ") );
				toolTipText = toolTipText.append(tr("\nDouble click on it to add his name on text writer.") );
			}
			if (!toolTipText.isEmpty()){
				QToolTip::showText(helpEvent->globalPos(), toolTipText);
				return true;
			} else {
				QToolTip::hideText();
			}
		}

	} else if (obj == ui->chatTextEdit) {
		if (chatType() == CHATTYPE_LOBBY) {
		    #define EVENT_IS(q_event) (event->type() == QEvent::q_event)
			if (EVENT_IS(FocusIn)) {
				if (was_shrinked) {
					shrinkChatTextEdit(false);
				}
			}
			#undef EVENT_IS
		}

		if (event->type() == QEvent::KeyPress) {

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (!keyEvent->text().isEmpty()) {
					updateStatusTyping();
				}

				if (chatType() == CHATTYPE_LOBBY) {
					if (keyEvent->key() == Qt::Key_Tab) {
						completeNickname((bool)(keyEvent->modifiers() & Qt::ShiftModifier));
						return true; // eat event
					}
					else {
						completionWord.clear();
					}
					if ((keyEvent->modifiers() & ui->chatTextEdit->getCompleterKeyModifiers()) && keyEvent->key() == ui->chatTextEdit->getCompleterKey()) {
						completer->setModel(modelFromPeers());
					}
					if (keyEvent->text()=="@") {
						ui->chatTextEdit->forceCompleterShowNextKeyEvent("@");
						completer->setModel(modelFromPeers());
					}
				}
				if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
					// Enter pressed
					if (Settings->getChatSendMessageWithCtrlReturn()) {
						if (keyEvent->modifiers() & Qt::ControlModifier) {
							// send message with Ctrl+Enter
							sendChat();
							return true; // eat event
						}
					} else {
						if ((keyEvent->modifiers() & Qt::ControlModifier) || (keyEvent->modifiers() & Qt::ShiftModifier)){
							// insert return
							ui->chatTextEdit->textCursor().insertText("\n");
						} else {
							// send message with Enter
							sendChat();
						}
						return true; // eat event
					}
				}
			}
		}
		if (event->type() == QEvent::StyleChange)
		{
			QString colorName = currentColor.name();
			qreal desiredContrast = Settings->valueFromGroup("Chat", "MinimumContrast", 4.5).toDouble();
			QColor backgroundColor = ui->chatTextEdit->palette().base().color();
			RsHtml::findBestColor(colorName, backgroundColor, desiredContrast);

			currentColor = QColor(colorName);
			ui->chatTextEdit->setTextColor(currentColor);
			colorChanged();
		}
	} else if (obj == ui->leSearch) {
		if (event->type() == QEvent::KeyPress) {

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				QString qsTextToFind=ui->leSearch->text();
				if (keyEvent->key()==Qt::Key_Backspace) {
					qsTextToFind=qsTextToFind.left(qsTextToFind.length()-1);// "\010"
				} else if (keyEvent->key()==Qt::Key_Tab) { // "\011"
				} else if (keyEvent->key()==Qt::Key_Return) { // "\015"
				} else if (keyEvent->text().length()==1)
					qsTextToFind+=keyEvent->text();
				if (((qsTextToFind.length()>=iCharToStartSearch) || (keyEvent->key()==Qt::Key_Return)) && (keyEvent->text().length()>0))
				{

					findText(qsTextToFind);
				} else {
					ui->leSearch->setPalette(qpSave_leSearch);
				}
			}
		}
	} else if (obj == ui->textBrowser->viewport()) {
		if (event->type() == QEvent::MouseButtonDblClick)	{

			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			QString anchor = ui->textBrowser->anchorForPosition(mouseEvent->pos());
			if (!anchor.isEmpty()){
				if (anchor.startsWith(PERSONID)){
					QString strId = anchor.replace(PERSONID,"");
					if (strId.contains(" "))
						strId.truncate(strId.indexOf(" "));

					RsGxsId mId = RsGxsId(strId.toStdString());
					if(!mId.isNull()) {
						RsIdentityDetails details;
						if (rsIdentity->getIdDetails(mId, details)){
							QString text = QString("@").append(GxsIdDetails::getName(details)).append(" ");
							ui->chatTextEdit->textCursor().insertText(text);
						}
					}
				}

			}

		}
	} else {
			if (event->type() == QEvent::WindowActivate) {
				if (isVisible() && (window() == NULL || window()->isActiveWindow())) {
					newMessages = false;
					emit infoChanged(this);
				}
			}
	}
	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}

/**
 * @brief Utility function for completeNickname.
 */
static bool caseInsensitiveCompare(QString a, QString b)
{
	return a.toLower() < b.toLower();
}

/**
 * @brief Completes nickname based on previous characters.
 * @param reverse true to list nicknames in reverse alphabetical order.
 */
void ChatWidget::completeNickname(bool reverse)
{
	// Find lobby we belong to
	ChatLobbyInfo lobby;

	if (! rsMsgs->getChatLobbyInfo(chatId.toLobbyId(),lobby))
		return;

	QTextCursor cursor = ui->chatTextEdit->textCursor();

	// Do nothing if there is a selection
	if (cursor.anchor() != cursor.position())
		return;

	// We want to complete the word typed by the user. This is easy.
	// But also, if there are two participants whose name start with the word
	// the user typed, the first press on <tab> should show the first
	// participant’s name, and the second press on <tab> should show the
	// second participant’s name.

	cursor.beginEditBlock();
	if (!completionWord.isEmpty()) {
		cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, cursor.position() - completionPosition);
	}
	else {
		cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
		completionPosition = cursor.position();
	}
	if (cursor.selectedText() == ": ") {
		cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
	}
	bool firstWord = (cursor.position() == 0);
	QString word = cursor.selectedText();
	if (word.endsWith(": ")) {
		word.chop(2);
	}

	if (word.length() > 0) {
		// Sort participants list
		std::list<QString> participants;
        RsIdentityDetails details ;

	for (auto it = lobby.gxs_ids.begin(); it != lobby.gxs_ids.end(); ++it)
    {
        if(rsIdentity->getIdDetails(it->first,details))
            participants.push_front(QString::fromUtf8(details.mNickname.c_str()));
        }
		participants.sort(caseInsensitiveCompare);

		// Search for a participant nickname that starts with the previous word
		std::list<QString>::const_iterator it, start, end;
		int delta;
		bool skippedParticipant = false;
		if (reverse) {
			// Note: at the moment reverse completion doesn’t work because
			// shift-tab selects the previous widget
			start = participants.end();
			--start;
			end = participants.begin();
			--end;
			delta = -1;
		}
		else {
			start = participants.begin();
			end = participants.end();
			delta = 1;
		}
		do {
			for (it = start; it != end; (delta == 1) ? ++it : --it) {
				QString participant = *it;
				if (participant.startsWith(word, Qt::CaseInsensitive)) {
					if (!completionWord.isEmpty() && !skippedParticipant) {
						// This participant nicknaem was completed with <tab>;
						// skip it and find the next one that starts with
						// completionWord
						word = completionWord;
						skippedParticipant = true;
						continue;
					}
					skippedParticipant = false;
					cursor.insertText(participant);
					if (firstWord) {
						cursor.insertText(QString(": "));
					}
					break;
				}
			}
		} while (skippedParticipant);
		if (completionWord.isEmpty()) {
			completionWord = word;
		}
	}
	cursor.endEditBlock();
}

QAbstractItemModel *ChatWidget::modelFromPeers()
{
    // Find lobby we belong to
    ChatLobbyInfo lobby ;

    if(! rsMsgs->getChatLobbyInfo(chatId.toLobbyId(),lobby))
        return new QStringListModel(completer);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
    // Get participants list
    QStringList participants;

	for (auto it = lobby.gxs_ids.begin(); it != lobby.gxs_ids.end(); ++it)
    {
        RsIdentityDetails details ;
        rsIdentity->getIdDetails(it->first,details) ;

        participants.push_front(QString::fromUtf8(details.mNickname.c_str()));
    }

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    return new QStringListModel(participants, completer);
}

void ChatWidget::addToolsAction(QAction *action)
{
	ui->pushtoolsButton->menu()->addAction(action);
}

void ChatWidget::showEvent(QShowEvent */*event*/)
{
	newMessages = false;
	emit infoChanged(this);
	// if user waded through the jungle of history just let him on
	// own decide whether to continue the journey or start typing
	QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
	bool is_scrollbar_at_end = scrollbar->value() == scrollbar->maximum();
	bool is_chat_text_edit_empty = ui->chatTextEdit->toPlainText().isEmpty();
	// show event will not be called on every change of focus
	if (is_scrollbar_at_end || !is_chat_text_edit_empty) {
		if (!firstShow) {
			shrinkChatTextEdit(false);
		}
		focusDialog();
	} else {
		// otherwise, focus will not even be gotten by chat itself
		ui->textBrowser->setFocus();

		if (!firstShow && !was_shrinked) {
			shrinkChatTextEdit(true);
		}
	}
	ChatUserNotify::clearWaitingChat(chatId);

	if (firstShow) {
		// Workaround: now the scroll position is correct calculated
		firstShow = false;
		QScrollBar *scrollbar2 = ui->textBrowser->verticalScrollBar();
		scrollbar2->setValue(scrollbar2->maximum());
	}
}

void ChatWidget::resizeEvent(QResizeEvent */*event*/)
{
	// it's about resize all chat window, not about chattextedit
	// just unshrink it and do not bother
	if (was_shrinked) {
		shrinkChatTextEdit(false);
	}
	// Workaround: now the scroll position is correct calculated
	QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
	scrollbar->setValue(scrollbar->maximum());
}

void ChatWidget::addToParent(QWidget *newParent)
{
	newParent->window()->installEventFilter(this);
}

void ChatWidget::removeFromParent(QWidget *oldParent)
{
	oldParent->window()->removeEventFilter(this);
}

void ChatWidget::focusDialog()
{
	ui->chatTextEdit->setFocus();
}

QToolButton* ChatWidget::getNotifyButton()
{
	if (ui) if (ui->notifyButton) return ui->notifyButton;
	return NULL;
}

void ChatWidget::setNotify(ChatLobbyUserNotify *clun)
{
	if(clun) notify=clun;
}

void ChatWidget::on_notifyButton_clicked()
{
	if(!notify) return;
	if (chatType() != CHATTYPE_LOBBY) return;

	QMenu* menu = notify->createMenu();
	QIcon icoLobby=(ui->notifyButton->icon());

	notify->makeSubMenu(menu, icoLobby, title, chatId.toLobbyId());
	menu->exec(ui->notifyButton->mapToGlobal(QPoint(0,ui->notifyButton->geometry().height())));
	delete(menu);

}

void ChatWidget::scrollToAnchor(QString anchor)
{
	ui->textBrowser->scrollToAnchor(anchor);
}

void ChatWidget::setWelcomeMessage(QString &text)
{
	ui->textBrowser->setText(text);
}

void ChatWidget::addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, MsgType chatType)
{
	addChatMsg(incoming, name, RsGxsId(), sendTime, recvTime, message, chatType);
}

void ChatWidget::addChatMsg(bool incoming, const QString &name, const RsGxsId gxsId
                            , const QDateTime &sendTime, const QDateTime &recvTime
                            , const QString &message, MsgType chatType)
{
#ifdef CHAT_DEBUG
	std::cout << "ChatWidget::addChatMsg message : " << message.toStdString() << std::endl;
#endif

	unsigned int formatTextFlag = RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_OPTIMIZE;
	unsigned int formatFlag = 0;

    bool addDate = false;
    if (QDate::currentDate()>lastMsgDate)
	 {
		 addDate=true;
    }

	// embed smileys ?
	if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool()) {
		if (!message.contains("NoEmbed=\"true\""))
			formatTextFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
	}

#ifdef USE_CMARK
	//Use CommonMark
	if (message.contains("CMark=\"true\"")) {
		formatTextFlag |= RSHTML_FORMATTEXT_USE_CMARK;
	}
#endif

	// Always fix colors
	formatTextFlag |= RSHTML_FORMATTEXT_FIX_COLORS;
	qreal desiredContrast = Settings->valueFromGroup("Chat", "MinimumContrast", 4.5).toDouble();
	QColor backgroundColor = ui->textBrowser->palette().base().color();

	// Remove font name, size, bold, italics?
	if (!Settings->valueFromGroup("Chat", "EnableCustomFonts", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_FAMILY;
	}
	if (!Settings->valueFromGroup("Chat", "EnableCustomFontSize", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_SIZE;
	}
	int desiredMinimumFontSize = Settings->valueFromGroup("Chat", "MinimumFontSize", 10).toInt();
	if (!Settings->valueFromGroup("Chat", "EnableBold", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_WEIGHT;
	}
	if (!Settings->valueFromGroup("Chat", "EnableItalics", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_STYLE;
	}

	ChatStyle::enumFormatMessage type;
	if (chatType == MSGTYPE_OFFLINE) {
		type = ChatStyle::FORMATMSG_OOUTGOING;
	} else if (chatType == MSGTYPE_SYSTEM) {
		type = ChatStyle::FORMATMSG_SYSTEM;
    } else if (chatType == MSGTYPE_HISTORY || addDate) {
        lastMsgDate=QDate::currentDate();
		type = incoming ? ChatStyle::FORMATMSG_HINCOMING : ChatStyle::FORMATMSG_HOUTGOING;
	} else {
		type = incoming ? ChatStyle::FORMATMSG_INCOMING : ChatStyle::FORMATMSG_OUTGOING;
	}

	if (chatType == MSGTYPE_SYSTEM) {
		formatFlag |= CHAT_FORMATMSG_SYSTEM;
	}

	QString formattedMessage = RsHtml().formatText(ui->textBrowser->document(), message, formatTextFlag, backgroundColor, desiredContrast, desiredMinimumFontSize);
	QDateTime dtTimestamp=incoming ? sendTime : recvTime;
	QString formatMsg = chatStyle.formatMessage(type, name, dtTimestamp, formattedMessage, formatFlag, backgroundColor);
	QString timeStamp = dtTimestamp.toString(Qt::ISODate);

	//replace Date and Time anchors
	formatMsg.replace(QString("<a name=\"date\">"),QString("<a name=\"%1\">").arg(timeStamp));
	formatMsg.replace(QString("<a name=\"time\">"),QString("<a name=\"%1\">").arg(timeStamp));
	//replace Name anchors with GXS Id
	if (!gxsId.isNull()) {
		RsIdentityDetails details;
		QString strPreName = "";

		QString strGxsId = QString::fromStdString(gxsId.toStdString());
		rsIdentity->getIdDetails(gxsId, details);
		bool isUnsigned = !(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED);
		if(isUnsigned && ui->textBrowser->getShowImages()) {
            QIcon icon = FilesDefs::getIconFromQtResourcePath(":/icons/anonymous_blue_128.png");
			int height = ui->textBrowser->fontMetrics().height()*0.8;
			QImage image(icon.pixmap(height,height).toImage());
			QByteArray byteArray;
			QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG"); // writes the image in PNG format inside the buffer
			QString iconBase64 = QString::fromLatin1(byteArray.toBase64().data());
			strPreName = QString("<img src=\"data:image/png;base64,%1\" alt=\"[unsigned]\" />").arg(iconBase64);
		}

		formatMsg.replace(QString("<a name=\"name\">")
		                  ,QString(strPreName).append("<a name=\"").append(PERSONID).append("%1 %2\">").arg(strGxsId, isUnsigned ? tr(" Unsigned"):""));
	} else {
		formatMsg.replace(QString("<a name=\"name\">"),"");
	}

	QTextCursor textCursor = QTextCursor(ui->textBrowser->textCursor());
	textCursor.movePosition(QTextCursor::End);
	textCursor.setBlockFormat(QTextBlockFormat ());
	ui->textBrowser->append(formatMsg);

	if (ui->leSearch->isVisible()) {
		QString qsTextToFind=ui->leSearch->text();
		findText(qsTextToFind);
	}

	resetStatusBar();

	if (incoming && chatType == MSGTYPE_NORMAL) {
		emit newMessage(this);

		if (!isActive()) {
			newMessages = true;
		}

		emit infoChanged(this);
	}
}

bool ChatWidget::isActive()
{
	if (!isVisible() || (window() && (!window()->isActiveWindow() || window()->isMinimized()))) {
		return false;
	}

	return true;
}

void ChatWidget::pasteText(const QString& S)
{
	//std::cerr << "In paste link" << std::endl;
	ui->chatTextEdit->insertHtml(S);
	setColorAndFont(false);
}

//void ChatWidget::pasteCreateMsgLink()
//{
//	RSettingsWin::showYourself(this, RSettingsWin::Chat);
//}

void ChatWidget::contextMenuTextBrowser(QPoint point)
{
	QMenu *contextMnu = ui->textBrowser->createStandardContextMenuFromPoint(point);

	contextMnu->addSeparator();
	contextMnu->addAction(ui->actionClearChatHistory);
	if (ui->textBrowser->textCursor().selection().toPlainText().length())
		contextMnu->addAction(ui->actionQuote);
	contextMnu->addAction(ui->actionDropPlacemark);

	if(ImageUtil::checkImage(ui->textBrowser, point))
	{
		if (! ui->textBrowser->getShowImages())
			contextMnu->addAction(ui->actionShow_Hidden_Images);

		ui->actionImport_sticker->setData(point);
		contextMnu->addAction(ui->actionImport_sticker);
	}

	QString anchor = ui->textBrowser->anchorForPosition(point);
	emit textBrowserAskContextMenu(contextMnu, anchor, point);

	contextMnu->exec(ui->textBrowser->viewport()->mapToGlobal(point));
	delete(contextMnu);
}

void ChatWidget::contextMenuSearchButton(QPoint /*point*/)
{
	QMenu *contextMnu = new QMenu;

	contextMnu->addSeparator();
	ui->actionFindCaseSensitively->setChecked(bFindCaseSensitively);
	contextMnu->addAction(ui->actionFindCaseSensitively);
	ui->actionFindWholeWords->setChecked(bFindWholeWords);
	contextMnu->addAction(ui->actionFindWholeWords);
	ui->actionMoveToCursor->setChecked(bMoveToCursor);
	contextMnu->addAction(ui->actionMoveToCursor);
	ui->actionSearchWithoutLimit->setChecked(bSearchWithoutLimit);
	contextMnu->addAction(ui->actionSearchWithoutLimit);

	contextMnu->exec(QCursor::pos());
	delete(contextMnu);
}

void ChatWidget::chatCharFormatChanged()
{
	if (inChatCharFormatChanged) {
		return;
	}

	inChatCharFormatChanged = true;

	// Reset font and color before inserting a character if edit box is empty
	// (color info disappears when the user deletes all text)
	if (ui->chatTextEdit->toPlainText().isEmpty()) {
		setColorAndFont(false);
	}

	inChatCharFormatChanged = false;
}

void ChatWidget::resetStatusBar()
{
	ui->typingLabel->clear();
	ui->typingPixmapLabel->clear();

	typing = false;

	emit infoChanged(this);
}

void ChatWidget::updateStatusTyping()
{
	if(Settings->getChatDoNotSendIsTyping())
		return;
	if (time(NULL) - lastStatusSendTime > 5)	// limit 'peer is typing' packets to at most every 10 sec
	{
#ifdef ONLY_FOR_LINGUIST
		tr("is typing...");
#endif
		rsMsgs->sendStatusString(chatId, "is typing...");
		lastStatusSendTime = time(NULL) ;
	}
}

void ChatWidget::updateLenOfChatTextEdit()
{
	if(sendingBlocked) return;

	QTextEdit *chatWidget = ui->chatTextEdit;
	QString text;
	RsHtml::optimizeHtml(chatWidget, text);
	std::wstring msg = text.toStdWString();

	uint32_t maxMessageSize = this->maxMessageSize();

	int charRemains = 0;
	if (maxMessageSize > 0) {
		charRemains = maxMessageSize - msg.length();
	}

	ui->sendButton->setEnabled(charRemains>=0);
	if (charRemains>0)
		text = tr("It remains %1 characters\nafter HTML conversion.").arg(charRemains);
	else if(charRemains<0)
		text = tr("Warning: This message is too big of %1 characters\nafter HTML conversion.").arg((0-charRemains));
	else
		text = "";

	ui->sendButton->setToolTip(text);
	ui->chatTextEdit->setToolTip(text);
}

void ChatWidget::sendChat()
{
	if (!ui->sendButton->isEnabled()){
		//Something block sending
		return;
	}

	QTextEdit *chatWidget = ui->chatTextEdit;

	if (chatWidget->toPlainText().isEmpty()) {
		// nothing to send
		return;
	}

	QString text;
	if (ui->actionSendAsPlainText->isChecked()){
		text = chatWidget->toPlainText();
		text.replace(QChar(-4),"");//Char used when image on text.
	} else {
		RsHtml::optimizeHtml(chatWidget, text,
		                     (ui->actionNoEmbed->isChecked() ? RSHTML_FORMATTEXT_NO_EMBED : 0)
		                     + (ui->actionSend_as_CommonMark->isChecked() ? RSHTML_FORMATTEXT_USE_CMARK : 0) );
	}
	std::string msg = text.toUtf8().constData();

	if (msg.empty()) {
		// nothing to send
		return;
	}

#ifdef CHAT_DEBUG
	std::cout << "ChatWidget:sendChat " << std::endl;
#endif
    rsMsgs->sendChat(chatId, msg);

	chatWidget->clear();
	// workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
	// QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
	chatWidget->setCurrentCharFormat(QTextCharFormat ());
}

void ChatWidget::on_closeInfoFrameButton_clicked()
{
	ui->info_Frame->setVisible(false);
}

void ChatWidget::on_searchButton_clicked(bool bValue)
{
	if (firstSearch)
		qpSave_leSearch=ui->leSearch->palette();

	removeFoundText();

	ui->searchBefore->setVisible(false);//findText set it to true
	ui->searchAfter->setVisible(false);//findText set it to true
	ui->leSearch->setPalette(qpSave_leSearch);
	if (bValue) {
		ui->leSearch->setFocus();
		if (!ui->leSearch->isVisible()){//Take text selected if leSearch is Invisible
			if (ui->textBrowser->textCursor().selectedText().length()>0) {
				ui->leSearch->setText(ui->textBrowser->textCursor().selectedText());
				findText(ui->leSearch->text());
			} else if(ui->chatTextEdit->textCursor().selectedText().length()>0) {
				ui->leSearch->setText(ui->chatTextEdit->textCursor().selectedText());
				findText(ui->leSearch->text());
			}
		}
		if (!ui->leSearch->text().isEmpty())
			findText(ui->leSearch->text());

	} else {
		//Erase last result Cursor
		QTextDocument *qtdDocument = ui->textBrowser->document();
		qtcCurrent=QTextCursor(qtdDocument);
	}
	ui->leSearch->setVisible(bValue);
  	ui->markButton->setVisible(bValue);
}
void ChatWidget::on_searchBefore_clicked()
{
	findText(ui->leSearch->text(),true,true);
}
void ChatWidget::on_searchAfter_clicked()
{
	findText(ui->leSearch->text(),false,true);
}

void ChatWidget::toogle_FindCaseSensitively()
{
	bFindCaseSensitively=!bFindCaseSensitively;
}

void ChatWidget::toogle_FindWholeWords()
{
	bFindWholeWords=!bFindWholeWords;
}

void ChatWidget::toogle_MoveToCursor()
{
	bMoveToCursor=!bMoveToCursor;
}

void ChatWidget::toogle_SeachWithoutLimit()
{
	bSearchWithoutLimit=!bSearchWithoutLimit;
}

bool ChatWidget::findText(const QString& qsStringToFind)
{
	return findText(qsStringToFind, false,false);
}

bool ChatWidget::findText(const QString& qsStringToFind, bool bBackWard, bool bForceMove)
{
	QTextDocument *qtdDocument = ui->textBrowser->document();
	bool bFound = false;

	removeFoundText();

	if (qsLastsearchText!=qsStringToFind)
		qtcCurrent=QTextCursor(qtdDocument);
	qsLastsearchText=qsStringToFind;

	if (!qsStringToFind.isEmpty())
	{
		bool bFirstFound = true;
		uint uiFoundCount = 0;

		QPalette qpBackGround=ui->leSearch->palette();

		QTextCursor qtcHighLight(qtdDocument);
		QTextCursor qtcCursor(qtdDocument);

		QTextCharFormat qtcfPlainFormat(qtcHighLight.charFormat());
		QTextCharFormat qtcfColorFormat = qtcfPlainFormat;
		qtcfColorFormat.setBackground(QBrush(cFoundColor));

		if (ui->textBrowser->textCursor().selectedText().length()>0)
			qtcCurrent=ui->textBrowser->textCursor();
		if (bBackWard) qtcHighLight.setPosition(qtdDocument->characterCount()-1);

		qtcCursor.beginEditBlock();

		while(!qtcHighLight.isNull()
					&& ( (!bBackWard && !qtcHighLight.atEnd())
							 || (bBackWard && !qtcHighLight.atStart())
							 ))
		{

			QTextDocument::FindFlags qtdFindFlag;
			if (bFindCaseSensitively) qtdFindFlag|=QTextDocument::FindCaseSensitively;
			if (bFindWholeWords) qtdFindFlag|=QTextDocument::FindWholeWords;
			if (bBackWard) qtdFindFlag|=QTextDocument::FindBackward;

			qtcHighLight=qtdDocument->find(qsStringToFind,qtcHighLight, qtdFindFlag);
			if(!qtcHighLight.isNull())
			{
				bFound=true;

				if (!bFirstFound)
				{
					if (smFoundCursor.size()<uiMaxSearchLimitColor || bSearchWithoutLimit)// stop after uiMaxSearchLimitColor
					{
						QTextCharFormat qtcfSave= qtcHighLight.charFormat();
						smFoundCursor[qtcHighLight]=qtcfSave;
						qtcHighLight.mergeCharFormat(qtcfColorFormat);
					}
				}

				if (bFirstFound &&
						((bBackWard && (qtcHighLight.position()<qtcCurrent.position()))
						 || (!bBackWard && (qtcHighLight.position()>qtcCurrent.position()))
						 ))
				{
					bFirstFound=false;
					qtcCurrent=qtcHighLight;
					if (bMoveToCursor || bForceMove) ui->textBrowser->setTextCursor(qtcHighLight);

				}


				if (uiFoundCount<UINT_MAX)
					uiFoundCount+=1;
			}
		}

		if (bFound)
		{
			qpBackGround.setColor(QPalette::Base,QColor(0,200,0));
			ui->leSearch->setToolTip(QString::number(uiFoundCount)+tr(" items found."));
		} else {
			qpBackGround.setColor(QPalette::Base,QColor(200,0,0));
			ui->leSearch->setToolTip(tr("No items found."));
		}
		ui->leSearch->setPalette(qpBackGround);

		qtcCursor.endEditBlock();

		ui->searchBefore->setVisible((!bFirstFound || (!bBackWard && bFound)));
		ui->searchAfter->setVisible((!bFirstFound || (bBackWard && bFound)));

		firstSearch = false;
	} else { //if (!qsStringToFind.isEmpty())
		ui->leSearch->setPalette(qpSave_leSearch);
	}

	return bFound;

}

void ChatWidget::removeFoundText()
{
	for(std::map<QTextCursor,QTextCharFormat>::const_iterator it=smFoundCursor.begin();it!=smFoundCursor.end();++it)
	{
		QTextCursor qtcCurrent=it->first;
		QTextCharFormat qtcfCurrent=it->second;
		qtcCurrent.setCharFormat(qtcfCurrent);
	}
	smFoundCursor.clear();
}

void ChatWidget::on_markButton_clicked(bool bValue)
{
	if (bValue)
	{
		if (ui->textBrowser->textCursor().selectedText().length()>0)
		{
			qtcMark=ui->textBrowser->textCursor();
			ui->markButton->setToolTip(tr("<b>Return to marked text</b><br><i>Ctrl+M</i>"));

		} else { bValue=false;}
	} else {
		if (qtcMark.position()!=0)
		{
			ui->textBrowser->setTextCursor(qtcMark);
			qtcMark=QTextCursor(ui->textBrowser->document());
			ui->markButton->setToolTip(tr("<b>Mark this selected text</b><br><i>Ctrl+M</i>"));

		}
	}
	ui->markButton->setChecked(bValue);
}

void ChatWidget::chooseColor()
{
	bool ok;
	QRgb color = QColorDialog::getRgba(currentColor.rgba(), &ok, window());
	if (ok) {
		currentColor = QColor(color);
		PeerSettings->setPrivateChatColor(chatId, currentColor.name());
		colorChanged();
		setColorAndFont(false);
	}
}

void ChatWidget::colorChanged()
{
	QPixmap pix(16, 16);
	pix.fill(currentColor);
	ui->actionChooseColor->setIcon(pix);
}

void ChatWidget::chooseFont()
{
	bool ok;
	//Use NULL as parent as with this QFontDialog don't take care of title nether options.
	QFont font = misc::getFont(&ok, currentFont, nullptr, tr("Choose your font."));

	if (ok) {
		QTextCursor cursor = ui->chatTextEdit->textCursor();

		if (cursor.selection().isEmpty()){
			currentFont = font;
			setFont();
		} else {
			// Merge Format doesn't works for only selection.
			// and charFormat() get format for last char.
			QTextCursor selCurs = cursor;
			QTextCursor lastCurs = cursor;
			int pos = cursor.selectionStart();
			lastCurs.setPosition(pos);
			do
			{
				// Get format block in selection iterating char one by one
				selCurs.setPosition(++pos);
				if (selCurs.charFormat() != lastCurs.charFormat())
				{
					// New char format, format last block.
					QTextCharFormat charFormat = lastCurs.charFormat();
					charFormat.setFont(font);
					lastCurs.setCharFormat(charFormat);
					// Last block formated, start it to current char.
					lastCurs.setPosition(pos-1);
				}
				// Add current char.
				lastCurs.setPosition(pos, QTextCursor::KeepAnchor);
			} while (pos < cursor.selectionEnd());

			// Now format last block
			if (lastCurs.selectionStart() != lastCurs.selectionEnd())
			{
				QTextCharFormat charFormat = lastCurs.charFormat();
				charFormat.setFont(font);
				lastCurs.setCharFormat(charFormat);
			}
		}
	}
}

void ChatWidget::resetFont()
{
	currentFont.fromString(Settings->getChatScreenFont());
	setFont();
}

void ChatWidget::resetFonts()
{
	currentFont.fromString(Settings->getChatScreenFont());
	setColorAndFont(true);
	PeerSettings->setPrivateChatFont(chatId, currentFont.toString());
}

void ChatWidget::setColorAndFont(bool both)
{
	QTextCharFormat format;
	format.setForeground(currentColor);
	format.setFont(currentFont);
	ui->chatTextEdit->mergeCurrentCharFormat(format);

	if (both)
	{
		QStringList fontdata=currentFont.toString().split(",");
		QString stylesheet="font-family: '"+fontdata[0]+"'; font-size: "+fontdata[1]+"pt;";
		if (currentFont.bold()) stylesheet+=" font-weight: bold;";
		if (currentFont.italic()) stylesheet+=" font-style: italic;";
		if (currentFont.underline()) stylesheet+=" text-decoration: underline;";
		ui->textBrowser->setStyleSheet(stylesheet);
	}

	ui->chatTextEdit->setFocus();
}

void ChatWidget::setFont()
{
	setColorAndFont(false);
	PeerSettings->setPrivateChatFont(chatId, currentFont.toString());
}

void ChatWidget::smileyWidget()
{
	Emoticons::showSmileyWidget(this, ui->emoteiconButton, SLOT(addSmiley()), true);
}

void ChatWidget::addSmiley()
{
	QString smiley = qobject_cast<QPushButton*>(sender())->toolTip().split("|").first();
	// add trailing space
	smiley += QString(" ");
	// add preceding space when needed (not at start of text or preceding space already exists)
	QString plainText = ui->chatTextEdit->toPlainText();

        int startPosition = ui->chatTextEdit->textCursor().position();
        if (startPosition > 0)
            startPosition -= 1;

        QChar start = plainText[startPosition];
	if(!ui->chatTextEdit->textCursor().atStart() && start != QChar(' '))
		smiley = QString(" ") + smiley;

	ui->chatTextEdit->textCursor().insertText(smiley);
}

void ChatWidget::stickerWidget()
{
	Emoticons::showStickerWidget(this, ui->stickerButton, SLOT(sendSticker()), true);
}

void ChatWidget::sendSticker()
{
	if(sendingBlocked) return;
	QString sticker = qobject_cast<QPushButton*>(sender())->statusTip();
	QString encodedImage;
	if (RsHtml::makeEmbeddedImage(sticker, encodedImage, 640*480, maxMessageSize() - 200)) {		//-200 for the html stuff
		RsHtml::optimizeHtml(encodedImage, 0);
		std::string msg = encodedImage.toUtf8().constData();
		rsMsgs->sendChat(chatId, msg);
	}
}

void ChatWidget::clearChatHistory()
{
	ui->textBrowser->clear();
	on_searchButton_clicked(false);
	ui->markButton->setChecked(false);
	if (chatType() == CHATTYPE_LOBBY) {
		if (notify) notify->chatLobbyCleared(chatId.toLobbyId(),"");
	}
	rsMsgs->clearChatLobby(chatId);
}

void ChatWidget::deleteChatHistory()
{
	if ((QMessageBox::question(this, "RetroShare", tr("Do you really want to physically delete the history?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes) {
		clearChatHistory();
        rsHistory->clear(chatId);
	}
}

void ChatWidget::messageHistory()
{
	if (!imBrowser)
		imBrowser = new ImHistoryBrowser(chatId, ui->chatTextEdit, this->title, window());
	imBrowser->show();
}

void ChatWidget::addExtraFile()
{
	QStringList files;
	if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
		ui->hashBox->addAttachments(files,mDefaultExtraFileFlags /*, 0*/);
	}
}

void ChatWidget::addExtraPicture()
{
	// select a picture file
	QString file;
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg *.gif *.webp )", file)) {
		QString encodedImage;
		uint32_t maxMessageSize = this->maxMessageSize();
        if (RsHtml::makeEmbeddedImage(file, encodedImage, 640*480, maxMessageSize - 200)) {		//-200 for the html stuff
			QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
			ui->chatTextEdit->textCursor().insertFragment(fragment);
		}
	}
}

void ChatWidget::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	std::cerr << "ChatWidget::fileHashingFinished() started." << std::endl;

	QString message;

	QList<HashedFile>::iterator it;
	for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
		HashedFile& hashedFile = *it;
		QString ext = QFileInfo(hashedFile.filename).suffix().toUpper();

		RetroShareLink link;

		// We dont use extra links anymore, since files in the extra list can always be accessed using anonymous+encrypted FT.

		link = RetroShareLink::createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash.toStdString()));

		if (hashedFile.flag & HashedFile::Picture) {
			message += QString("<img src=\"file:///%1\" width=\"100\" height=\"100\">").arg(hashedFile.filepath);
			message+="<br>";
		} else {
			bool preview = false;
			if(hashedFiles.size()==1 && (ext == "JPG" || ext == "PNG" || ext == "JPEG" || ext == "GIF"))
			{
				QString encodedImage;
				uint32_t maxMessageSize = this->maxMessageSize();
				if (RsHtml::makeEmbeddedImage(hashedFile.filepath, encodedImage, 640*480, maxMessageSize - 200 - link.toHtmlSize().length()))
				{	QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
					ui->chatTextEdit->textCursor().insertFragment(fragment);
					preview=true;
				}
			}
			if(!preview)
			{
				QString image = FilesDefs::getImageFromFilename(hashedFile.filename, false);
				if (!image.isEmpty()) {
					message += QString("<img src=\"%1\">").arg(image);
				}
			}
		}
		message += link.toHtmlSize();

		if (it != hashedFiles.end()) {
			message += "<BR>";
		}
	}

#ifdef CHAT_DEBUG
	std::cerr << "ChatWidget::fileHashingFinished message : " << message.toStdString() << std::endl;
#endif

    ui->chatTextEdit->insertHtml(message);

}

bool ChatWidget::fileSave()
{
	if (fileName.isEmpty())
		return fileSaveAs();

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly))
		return false;
	QTextStream ts(&file);
	ts.setCodec(QTextCodec::codecForName("UTF-8"));
	ts << ui->textBrowser->document()->toPlainText();
	ui->textBrowser->document()->setModified(false);
	return true;
}

bool ChatWidget::fileSaveAs()
{
	QString fn;
	if (misc::getSaveFileName(window(), RshareSettings::LASTDIR_HISTORY, tr("Save as..."), tr("Text File (*.txt );;All Files (*)"), fn)) {
		setCurrentFileName(fn);
		return fileSave();
	}

	return false;
}

void ChatWidget::setCurrentFileName(const QString &fileName)
{
	this->fileName = fileName;
	ui->textBrowser->document()->setModified(false);

	setWindowModified(false);
}

void ChatWidget::updateStatus(const QString &peer_id, int status)
{
    if (! (chatType() == CHATTYPE_PRIVATE || chatType() == CHATTYPE_DISTANT))
    {
		// updateTitle is used
		return;
    }

    // make virtual peer id from gxs id in case of distant chat
    RsPeerId vpid;
    if(chatId.isDistantChatId())
        vpid = RsPeerId(chatId.toDistantChatId());
    else
        vpid = chatId.toPeerId();

	/* set font size for status  */
    if (peer_id.toStdString() == vpid.toStdString())
    {
	    // the peers status has changed

		QString tooltip_info ;
	    QString peerName ;

	    if(chatId.isDistantChatId())
	    {
		    DistantChatPeerInfo dcpinfo ;
		    RsIdentityDetails details  ;

		    if(rsMsgs->getDistantChatStatus(chatId.toDistantChatId(),dcpinfo))
			{
			    if(rsIdentity->getIdDetails(dcpinfo.to_id,details))
				    peerName = QString::fromUtf8( details.mNickname.c_str() ) ;
			    else
				    peerName = QString::fromStdString(dcpinfo.to_id.toStdString()) ;

				tooltip_info = QString("Identity Id: ")+QString::fromStdString(dcpinfo.to_id.toStdString());
			}
		    else
			{
			    peerName = QString::fromStdString(chatId.toDistantChatId().toStdString()) ;
				tooltip_info = QString("Identity Id: unknown (bug?)");
			}
	    }
	    else
		{
		    peerName = QString::fromUtf8(rsPeers->getPeerName(chatId.toPeerId()).c_str());
			tooltip_info = QString("Peer Id: ") + QString::fromStdString(chatId.toPeerId().toStdString());
		}

	    // is scrollbar at the end?
	    QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
	    bool atEnd = (scrollbar->value() == scrollbar->maximum());

	    switch (status) {
	    case RS_STATUS_OFFLINE:
		    ui->info_Frame->setVisible(true);
		    ui->infoLabel->setText(peerName + " " + tr("appears to be Offline.") +"\n" + tr("Messages you send will be delivered after Friend is again Online."));
		    break;

	    case RS_STATUS_INACTIVE:
		    ui->info_Frame->setVisible(true);
		    ui->infoLabel->setText(peerName + " " + tr("is Idle and may not reply"));
		    break;

	    case RS_STATUS_ONLINE:
		    ui->info_Frame->setVisible(false);
		    break;

	    case RS_STATUS_AWAY:
		    ui->infoLabel->setText(peerName + " " + tr("is Away and may not reply"));
		    ui->info_Frame->setVisible(true);
		    break;

	    case RS_STATUS_BUSY:
		    ui->infoLabel->setText(peerName + " " + tr("is Busy and may not reply"));
		    ui->info_Frame->setVisible(true);
		    break;
	    }

	    ui->titleLabel->setText(peerName);
	    ui->titleLabel->setToolTip(tooltip_info);
	    ui->statusLabel->setText(QString("(%1)").arg(StatusDefs::name(status)));

	    peerStatus = status;

	    if (atEnd) {
		    // scroll to the end
		    scrollbar->setValue(scrollbar->maximum());
	    }

	    emit infoChanged(this);
	    emit statusChanged(status);

	    // Notify all ChatWidgetHolder
	    foreach (ChatWidgetHolder *chatWidgetHolder, mChatWidgetHolder) {
		    chatWidgetHolder->updateStatus(status);
	    }

	    return;
    }

	// ignore status change
}

void ChatWidget::updateTitle()
{
	if (chatType() != CHATTYPE_LOBBY) {
		// updateStatus is used
		return;
	}

	ui->titleLabel->setText(RsHtml::plainText(name) + "@" + RsHtml::plainText(title));
}

void ChatWidget::updatePeersCustomStateString(const QString& peer_id, const QString& status_string)
{
	if (chatType() != CHATTYPE_PRIVATE )
	{
		return;
	}

	QString status_text;

	if (RsPeerId(peer_id.toStdString()) == chatId.toPeerId()) {
		// the peers status string has changed
		if (status_string.isEmpty()) {
			ui->statusMessageLabel->hide();
			ui->titleLabel->setAlignment ( Qt::AlignTop );
			ui->statusLabel->setAlignment ( Qt::AlignTop );

		} else {
			ui->statusMessageLabel->show();
			status_text = RsHtml().formatText(NULL, status_string, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);
			ui->statusMessageLabel->setText(status_text);
			ui->titleLabel->setAlignment ( Qt::AlignVCenter );
			ui->statusLabel->setAlignment ( Qt::AlignVCenter );
		}
	}
}

void ChatWidget::updateStatusString(const QString &statusMask, const QString &statusString, bool permanent)
{
	ui->typingLabel->setText(QString(statusMask).arg(trUtf8(statusString.toUtf8()))); // displays info for 5 secs.

	if (statusString.contains("Connexion refused")){
		ui->typingPixmapLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":images/denied-32.png") );
	} else {
		ui->typingPixmapLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":icons/png/typing.png") );
	}

	if (statusString == "is typing...") {
		typing = true;

		emit infoChanged(this);
	}

    if(!permanent)
        QTimer::singleShot(5000, this, SLOT(resetStatusBar())) ;
}

void ChatWidget::updatePixmapLabel(const QPixmap &pixmap)
{
	ui->typingPixmapLabel->setPixmap(pixmap);
}

void ChatWidget::setName(const QString &name)
{
	this->name = name;
	updateTitle();
}

bool ChatWidget::setStyle()
{
	if (style.showDialog(window())) {
        PeerSettings->setStyle(chatId, "PopupChatDialog", style);
		return true;
	}

	return false;
}

void ChatWidget::setUseCMark(const bool bUseCMark)
{
	useCMark = bUseCMark;
	ui->cmPreview->setVisible(useCMark);
	updateCMPreview();
}

void ChatWidget::updateCMPreview()
{
	if (!useCMark) return;

	QString message = ui->chatTextEdit->toHtml();
	QString formattedMessage = RsHtml().formatText(ui->cmPreview->document(), message, RSHTML_FORMATTEXT_USE_CMARK);
	ui->cmPreview->setHtml(formattedMessage);
}

void ChatWidget::quote()
{
	QString text = RsHtml::makeQuotedText(ui->textBrowser);
	ui->chatTextEdit->append(text);
}

void ChatWidget::dropPlacemark()
{
    ui->textBrowser->moveCursor(QTextCursor::End);       // *append* inserts text at end but with formatting in effect at
    ui->textBrowser->append("----------");               // current cursor position, such as in the middle of a hotlink,
                                                         // which would be strange.  This OTOH inserts text with
                                                         // formatting in effect on the last line, which may be strange
                                                         // or not.
}

void ChatWidget::saveSticker()
{
	QPoint point = ui->actionImport_sticker->data().toPoint();
	QTextCursor cursor = ui->textBrowser->cursorForPosition(point);
	QString filename = QInputDialog::getText(window(), "Import sticker", "Sticker name");
	if(filename.isEmpty()) return;
	filename = Emoticons::importedStickerPath() + "/" + filename + ".png";
	ImageUtil::extractImage(window(), cursor, filename);
}

void ChatWidget::shrinkChatTextEdit(bool shrink_me)
{
	// here and at eventfiltert check
	if (chatType() != CHATTYPE_LOBBY)
		return;
	if (!Settings->getShrinkChatTextEdit()) {
		if (was_shrinked) {
			ui->chatVSplitter->setSizes(_chatvsplitter_saved_size);
		}
		_chatvsplitter_saved_size.clear();
		was_shrinked = false;
	}

	if (Settings->getShrinkChatTextEdit()) {
		if (shrink_me) {
			if (!was_shrinked) {
				_chatvsplitter_saved_size = ui->chatVSplitter->sizes();

				QList<int> shrinked_v_splitter_size = _chatvsplitter_saved_size;
				// #define TEXT_BROWSER ui->chatVSplitter->indexOf(ui->textBrowser)
				#define TEXT_BROWSER 0
				// when you will update the layout one more time change this appropriately
				// #define BELOW_TEXT_BROWSER ui->chatVSplitter->indexOf(ui->chatVSplitter->widget(1))
				#define BELOW_TEXT_BROWSER 1
				int height_diff = shrinked_v_splitter_size[BELOW_TEXT_BROWSER] - ui->chatTextEdit->minimumHeight(); 
				shrinked_v_splitter_size[BELOW_TEXT_BROWSER] = ui->chatTextEdit->minimumHeight();
				shrinked_v_splitter_size[TEXT_BROWSER] += height_diff;
				ui->chatVSplitter->setSizes( shrinked_v_splitter_size );
				#undef TEXT_BROWSER
				#undef BELOW_TEXT_BROWSER
				was_shrinked = true;
			}
		} else { // (!shrink_me)
			if (was_shrinked) {
				// to not shrink/unshrink at every entry into chat
				// when unshrinked state is enough to a browser be scrollable, but shrinked - not
				QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
				bool is_scrollbar_at_end = scrollbar->value() == scrollbar->maximum();
				ui->chatVSplitter->setSizes(_chatvsplitter_saved_size);
				if (is_scrollbar_at_end)
					scrollbar->setValue(scrollbar->maximum());
				was_shrinked = false;
			}
		}
	}
}
