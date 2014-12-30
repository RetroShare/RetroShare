/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, RetroShare Team
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

#include <QApplication>
#include <QMenu>
#include <QKeyEvent>
#include <QScrollBar>
#include <QColorDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTextCodec>
#include <QTimer>
#include <QTextDocumentFragment>
#include <QStringListModel>

#include "ChatWidget.h"
#include "ui_ChatWidget.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/rsettingswin.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/im_history/ImHistoryBrowser.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/Emoticons.h"
#include "util/misc.h"
#include "util/HandleRichText.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsplugin.h>

#include <time.h>

/*****
 * #define CHAT_DEBUG 1
 *****/

ChatWidget::ChatWidget(QWidget *parent) :
	QWidget(parent), ui(new Ui::ChatWidget)
{
	ui->setupUi(this);

	newMessages = false;
	typing = false;
	peerStatus = 0;
	firstShow = true;
	firstSearch = true;
	inChatCharFormatChanged = false;
	completer = NULL;
	lastMsgDate = QDate::currentDate();

	lastStatusSendTime = 0 ;

	iCharToStartSearch=Settings->getChatSearchCharToStartSearch();
	bFindCaseSensitively=Settings->getChatSearchCaseSensitively();
	bFindWholeWords=Settings->getChatSearchWholeWords();
	bMoveToCursor=Settings->getChatSearchMoveToCursor();
	bSearchWithoutLimit=Settings->getChatSearchSearchWithoutLimit();
	uiMaxSearchLimitColor=Settings->getChatSearchMaxSearchLimitColor();
	cFoundColor=Settings->getChatSearchFoundColor();


	ui->actionSearchWithoutLimit->setText(tr("Don't stop to color after ")+QString::number(uiMaxSearchLimitColor)+tr(" items found (need more CPU)"));

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
	connect(ui->actionSearch_History, SIGNAL(triggered()), this, SLOT(searchHistory()));

	ui->markButton->setToolTip(tr("<b>Mark this selected text</b><br><i>Ctrl+M</i>"));

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendChat()));
	connect(ui->addFileButton, SIGNAL(clicked()), this , SLOT(addExtraFile()));

	connect(ui->attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
	connect(ui->emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
	connect(ui->actionSaveChatHistory, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(ui->actionClearChatHistory, SIGNAL(triggered()), this, SLOT(clearChatHistory()));
	connect(ui->actionDeleteChatHistory, SIGNAL(triggered()), this, SLOT(deleteChatHistory()));
	connect(ui->actionMessageHistory, SIGNAL(triggered()), this, SLOT(messageHistory()));
	connect(ui->actionChooseFont, SIGNAL(triggered()), this, SLOT(chooseFont()));
	connect(ui->actionChooseColor, SIGNAL(triggered()), this, SLOT(chooseColor()));
	connect(ui->actionResetFont, SIGNAL(triggered()), this, SLOT(resetFont()));

	connect(ui->hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

	connect(ui->textBrowser, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTextBrowser(QPoint)));

	connect(ui->chatTextEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
	// reset text and color after removing all characters from the QTextEdit and after calling QTextEdit::clear
	connect(ui->chatTextEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(chatCharFormatChanged()));
	connect(ui->chatTextEdit, SIGNAL(textChanged()), this, SLOT(updateLenOfChatTextEdit()));

	ui->infoFrame->setVisible(false);
	ui->statusMessageLabel->hide();
	
	ui->searchframe->hide();

	setAcceptDrops(true);
	ui->chatTextEdit->setAcceptDrops(false);
	ui->hashBox->setDropWidget(this);
	ui->hashBox->setAutoHide(true);

	QMenu *menu = new QMenu();
	menu->addAction(ui->actionChooseFont);
	menu->addAction(ui->actionChooseColor);
	menu->addAction(ui->actionResetFont);
	ui->fontButton->setMenu(menu);

	menu = new QMenu();
	menu->addAction(ui->actionClearChatHistory);
	menu->addAction(ui->actionDeleteChatHistory);
	menu->addAction(ui->actionSaveChatHistory);
	menu->addAction(ui->actionMessageHistory);
	menu->addAction(ui->actionSearch_History);
	ui->pushtoolsButton->setMenu(menu);

	ui->chatTextEdit->installEventFilter(this);
	ui->textBrowser->installEventFilter(this);

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
//    QAction *action = new QAction(QIcon(":/images/pasterslink.png"), tr("Paste/Create private chat or Message link..."), this);
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
	ui->verticalLayout_2->addWidget(w) ;
	update() ;
}

void ChatWidget::addChatBarWidget(QWidget *w)
{
	ui->pluginButtonFrame->layout()->addWidget(w) ;
}

void ChatWidget::addVOIPBarWidget(QWidget *w)
{
	ui->titleBarFrame->layout()->addWidget(w) ;
}


void ChatWidget::init(const ChatId &chat_id, const QString &title)
{
    this->chatId = chat_id;
	this->title = title;

	ui->titleLabel->setText(RsHtml::plainText(title));

    RsPeerId ownId = rsPeers->getOwnId();
	setName(QString::fromUtf8(rsPeers->getPeerName(ownId).c_str()));

    if(chatId.isPeerId() || chatId.isGxsId())
        chatStyle.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);
    if(chatId.isBroadcast() || chatId.isLobbyId())
        chatStyle.setStyleFromSettings(ChatStyle::TYPE_PUBLIC);

    currentColor.setNamedColor(PeerSettings->getPrivateChatColor(chatId));
    currentFont.fromString(PeerSettings->getPrivateChatFont(chatId));

	colorChanged();
	setColorAndFont();

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
	int messageCount;

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
    } else if(chatId.isBroadcast()){
        hist_chat_type = RS_HISTORY_TYPE_PUBLIC;
        messageCount = Settings->getPublicChatHistoryCount();

        ui->titleBarFrame->setVisible(false);
        ui->actionSearch_History->setVisible(false);
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
                if((time(NULL)-2) > historyIt->recvTime)
                    addChatMsg(historyIt->incoming, QString::fromUtf8(historyIt->peerName.c_str()), QDateTime::fromTime_t(historyIt->sendTime), QDateTime::fromTime_t(historyIt->recvTime), QString::fromUtf8(historyIt->message.c_str()), MSGTYPE_HISTORY);
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
    if(chatId.isGxsId())
        return CHATTYPE_DISTANT;
    if(chatId.isLobbyId())
        return CHATTYPE_LOBBY;

    return CHATTYPE_UNKNOWN;
}

void ChatWidget::processSettings(bool load)
{
	Settings->beginGroup(QString("ChatWidget"));

	if (load) {
		// load settings

		// state of splitter
		ui->chatsplitter->restoreState(Settings->value("ChatSplitter").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("ChatSplitter", ui->chatsplitter->saveState());
	}

	Settings->endGroup();
}

bool ChatWidget::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->textBrowser || obj == ui->leSearch || obj == ui->chatTextEdit) {
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
					ui->searchButton->setChecked(!ui->searchButton->isChecked() | bTextselected);
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
	}
    if (obj == ui->textBrowser) {
        if (event->type() == QEvent::KeyPress) {

            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent) {
                if (keyEvent->key() == Qt::Key_Delete ) {
                    // Delete pressed
                    if (ui->textBrowser->textCursor().selectedText().length()>0)
                        ui->textBrowser->textCursor().deleteChar();

                }
				if (keyEvent->key() == Qt::Key_M && keyEvent->modifiers() == Qt::ControlModifier)
				{
					on_markButton_clicked(!ui->markButton->isChecked());
				}
            }
        }
    } else if (obj == ui->chatTextEdit) {
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
				}
					if ((keyEvent->modifiers() & ui->chatTextEdit->getCompleterKeyModifiers()) && keyEvent->key() == ui->chatTextEdit->getCompleterKey()) {
						completer->setModel(modelFromPeers());
					}
					if (keyEvent->text()=="@") {
						ui->chatTextEdit->forceCompleterShowNextKeyEvent("@");
						completer->setModel(modelFromPeers());
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
						if (keyEvent->modifiers() & Qt::ControlModifier) {
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
	} else if (obj == ui->leSearch) {
		if (event->type() == QEvent::KeyPress) {

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				QString qsTextToFind=ui->leSearch->text();
				if (((qsTextToFind.length()>iCharToStartSearch) || (keyEvent->key()==Qt::Key_Return)) && (keyEvent->text().length()>0))
				{
					if (keyEvent->key()==Qt::Key_Backspace) {
						qsTextToFind=qsTextToFind.left(qsTextToFind.length()-1);// "\010"
					} else if (keyEvent->key()==Qt::Key_Tab) { // "\011"
					} else if (keyEvent->key()==Qt::Key_Return) { // "\015"
					} else if (keyEvent->text().length()==1)
						qsTextToFind+=keyEvent->text();

					findText(qsTextToFind);
				} else {
					ui->leSearch->setPalette(qpSave_leSearch);
				}
			}
		}
	} else {
		if (event->type() == QEvent::WindowActivate) {
			if (isVisible() && (window() == NULL || window()->isActiveWindow())) {
				newMessages = false;
				emit infoChanged(this);
				focusDialog();
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
	const ChatLobbyInfo *lobby = NULL;
	std::list<ChatLobbyInfo> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

	std::list<ChatLobbyInfo>::const_iterator lobbyIt;
	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
        if (chatId.toLobbyId() == lobbyIt->lobby_id) {
            lobby = &*lobbyIt;
            break;
        }
	}

	if (!lobby)
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
		for (	std::map<std::string,time_t>::const_iterator it = lobby->nick_names.begin();
				it != lobby->nick_names.end();
				++it) {
			participants.push_front(QString::fromUtf8(it->first.c_str()));
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
	const ChatLobbyInfo *lobby = NULL;
	std::list<ChatLobbyInfo> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

	std::list<ChatLobbyInfo>::const_iterator lobbyIt;
	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
        if (chatId.toLobbyId() == lobbyIt->lobby_id) {
            lobby = &*lobbyIt;
            break;
        }
	}

	if (!lobby)
		return new QStringListModel(completer);

#ifndef QT_NO_CURSOR
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
	// Get participants list
	 QStringList participants;
	for (	std::map<std::string,time_t>::const_iterator it = lobby->nick_names.begin();
			it != lobby->nick_names.end();
			++it) {
		participants.push_front(QString::fromUtf8(it->first.c_str()));
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
	focusDialog();

	if (firstShow) {
		// Workaround: now the scroll position is correct calculated
		firstShow = false;
		QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
		scrollbar->setValue(scrollbar->maximum());
	}
}

void ChatWidget::resizeEvent(QResizeEvent */*event*/)
{
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

void ChatWidget::setWelcomeMessage(QString &text)
{
	ui->textBrowser->setText(text);
}

void ChatWidget::addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, MsgType chatType)
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
		formatTextFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
	}

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

	QString formattedMessage = RsHtml().formatText(ui->textBrowser->document(), message, formatTextFlag, backgroundColor, desiredContrast);
	QString formatMsg = chatStyle.formatMessage(type, name, incoming ? sendTime : recvTime, formattedMessage, formatFlag);

    ui->textBrowser->textCursor().setBlockFormat(QTextBlockFormat ());
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
	setColorAndFont();
}

void ChatWidget::pasteCreateMsgLink()
{
	RSettingsWin::showYourself(this, RSettingsWin::Chat);
}

void ChatWidget::contextMenuTextBrowser(QPoint point)
{
	QMatrix matrix;
	matrix.translate(ui->textBrowser->horizontalScrollBar()->value(), ui->textBrowser->verticalScrollBar()->value());

	QMenu *contextMnu = ui->textBrowser->createStandardContextMenu(matrix.map(point));

	contextMnu->addSeparator();
	contextMnu->addAction(ui->actionClearChatHistory);

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
		setColorAndFont();
	}

	inChatCharFormatChanged = false;
}

void ChatWidget::resetStatusBar()
{
	ui->typingLabel->clear();
	ui->typingpixmapLabel->clear();

	typing = false;

	emit infoChanged(this);
}

void ChatWidget::updateStatusTyping()
{
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
	QTextEdit *chatWidget = ui->chatTextEdit;
	QString text;
	RsHtml::optimizeHtml(chatWidget, text);
	std::wstring msg = text.toStdWString();

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

	bool msgToLarge = false;
	if (maxMessageSize > 0) {
		msgToLarge = (msg.length() >= maxMessageSize);
	}

	ui->sendButton->setEnabled(!msgToLarge);
	text = tr("%1This message consists of %2 characters.").arg(msgToLarge ? tr("Warning: ") : "").arg(msg.length());
	ui->sendButton->setToolTip(text);
	ui->chatTextEdit->setToolTip(msgToLarge?text:"");
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
	RsHtml::optimizeHtml(chatWidget, text);
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
	ui->infoFrame->setVisible(false);
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
	bool bFirstFound = true;
	uint uiFoundCount = 0;

	removeFoundText();

	if (qsLastsearchText!=qsStringToFind)
		qtcCurrent=QTextCursor(qtdDocument);
	qsLastsearchText=qsStringToFind;

	if (!qsStringToFind.isEmpty())
	{
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

				}//if (bFirstFound && (qtcHighLight.position()>qtcCurrent.position()))


				if (uiFoundCount<UINT_MAX)
					uiFoundCount+=1;
			}//if(!qtcHighLight.isNull())
		}//while(!qtcHighLight.isNull() && !qtcHighLight.atEnd())

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
	QRgb color = QColorDialog::getRgba(ui->chatTextEdit->textColor().rgba(), &ok, window());
	if (ok) {
		currentColor = QColor(color);
        PeerSettings->setPrivateChatColor(chatId, currentColor.name());
		colorChanged();
		setColorAndFont();
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
	QFont font = QFontDialog::getFont(&ok, currentFont, this);
	if (ok) {
		currentFont = font;
		setFont();
	}
}

void ChatWidget::resetFont()
{
	currentFont.fromString(Settings->getChatScreenFont());
	setFont();
}

void ChatWidget::setColorAndFont()
{

	ui->chatTextEdit->setFont(currentFont);
	ui->chatTextEdit->setTextColor(currentColor);

	ui->chatTextEdit->setFocus();
}

void ChatWidget::setFont()
{
	setColorAndFont();
    PeerSettings->setPrivateChatFont(chatId, currentFont.toString());
}

void ChatWidget::smileyWidget()
{
	Emoticons::showSmileyWidget(this, ui->emoteiconButton, SLOT(addSmiley()), true);
}

void ChatWidget::addSmiley()
{
	ui->chatTextEdit->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void ChatWidget::clearChatHistory()
{
	ui->textBrowser->clear();
	on_searchButton_clicked(false);
	ui->markButton->setChecked(false);
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
    ImHistoryBrowser imBrowser(chatId, ui->chatTextEdit, window());
	imBrowser.exec();
}

void ChatWidget::searchHistory()
{
	if(ui->actionSearch_History->isChecked()){
      ui->searchframe->show();
  }else {
      ui->searchframe->hide();
  }
  
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
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg)", file)) {
		QString encodedImage;
		if (RsHtml::makeEmbeddedImage(file, encodedImage, 640*480)) {
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
		QString ext = QFileInfo(hashedFile.filename).suffix();

		RetroShareLink link;

		if(mDefaultExtraFileFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
            link.createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash.toStdString()));
		else
            link.createExtraFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash.toStdString()),QString::fromStdString(rsPeers->getOwnId().toStdString()));

		if (hashedFile.flag & HashedFile::Picture) {
			message += QString("<img src=\"file:///%1\" width=\"100\" height=\"100\">").arg(hashedFile.filepath);
			message+="<br>";
		} else {
			QString image = FilesDefs::getImageFromFilename(hashedFile.filename, false);
			if (!image.isEmpty()) {
				message += QString("<img src=\"%1\">").arg(image);
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
	if (chatType() == CHATTYPE_LOBBY) {
		// updateTitle is used
		return;
	}

    // make virtual peer id from gxs id in case of distant chat
    RsPeerId vpid;
    if(chatId.isGxsId())
        vpid = RsPeerId(chatId.toGxsId());
    else
        vpid = chatId.toPeerId();

	/* set font size for status  */
    if (peer_id.toStdString() == vpid.toStdString()) {
		// the peers status has changed

		QString peerName ;
        if(chatId.isGxsId())
		{
			RsIdentityDetails details  ;
            if(rsIdentity->getIdDetails(chatId.toGxsId(),details))
				peerName = QString::fromUtf8( details.mNickname.c_str() ) ;
			else
                peerName = QString::fromStdString(chatId.toGxsId().toStdString()) ;
		}
		else
            peerName = QString::fromUtf8(rsPeers->getPeerName(chatId.toPeerId()).c_str());

		// is scrollbar at the end?
		QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
		bool atEnd = (scrollbar->value() == scrollbar->maximum());

		switch (status) {
		case RS_STATUS_OFFLINE:
			ui->infoFrame->setVisible(true);
			ui->infoLabel->setText(peerName + " " + tr("appears to be Offline.") +"\n" + tr("Messages you send will be delivered after Friend is again Online"));
			break;

		case RS_STATUS_INACTIVE:
			ui->infoFrame->setVisible(true);
			ui->infoLabel->setText(peerName + " " + tr("is Idle and may not reply"));
			break;

		case RS_STATUS_ONLINE:
			ui->infoFrame->setVisible(false);
			break;

		case RS_STATUS_AWAY:
			ui->infoLabel->setText(peerName + " " + tr("is Away and may not reply"));
			ui->infoFrame->setVisible(true);
			break;

		case RS_STATUS_BUSY:
			ui->infoLabel->setText(peerName + " " + tr("is Busy and may not reply"));
			ui->infoFrame->setVisible(true);
			break;
		}

		ui->titleLabel->setText(peerName);
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
	QString status_text;

    // TODO: fix peer_id and types and eveyrhing
    /*
    if (RsPeerId(peer_id.toStdString()) == peerId) {
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
    */
}

void ChatWidget::updateStatusString(const QString &statusMask, const QString &statusString)
{
	ui->typingLabel->setText(QString(statusMask).arg(tr(statusString.toLatin1()))); // displays info for 5 secs.
	ui->typingpixmapLabel->setPixmap(QPixmap(":images/typing.png") );

	if (statusString == "is typing...") {
		typing = true;

		emit infoChanged(this);
	}

	QTimer::singleShot(5000, this, SLOT(resetStatusBar())) ;
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
