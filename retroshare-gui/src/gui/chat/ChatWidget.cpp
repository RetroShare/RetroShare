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

#include "ChatWidget.h"
#include "ui_ChatWidget.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/im_history/ImHistoryBrowser.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/Emoticons.h"
#include "util/misc.h"
#include "util/HandleRichText.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>

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
	isChatLobby = false;
	firstShow = true;
	inChatCharFormatChanged = false;

	lastStatusSendTime = 0 ;

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendChat()));
	connect(ui->addFileButton, SIGNAL(clicked()), this , SLOT(addExtraFile()));

	connect(ui->textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
	connect(ui->colorButton, SIGNAL(clicked()), this, SLOT(chooseColor()));
	connect(ui->emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
	connect(ui->actionSaveChatHistory, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(ui->actionClearChatHistory, SIGNAL(triggered()), this, SLOT(clearChatHistory()));
	connect(ui->actionDeleteChatHistory, SIGNAL(triggered()), this, SLOT(deleteChatHistory()));
	connect(ui->actionMessageHistory, SIGNAL(triggered()), this, SLOT(messageHistory()));
	connect(ui->actionChooseFont, SIGNAL(triggered()), this, SLOT(chooseFont()));
	connect(ui->actionResetFont, SIGNAL(triggered()), this, SLOT(resetFont()));

	connect(ui->hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

	connect(ui->textBrowser, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTextBrowser(QPoint)));

	connect(ui->chatTextEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
	// reset text and color after removing all characters from the QTextEdit and after calling QTextEdit::clear
	connect(ui->chatTextEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(chatCharFormatChanged()));

	ui->infoFrame->setVisible(false);
	ui->statusMessageLabel->hide();

	setAcceptDrops(true);
	ui->chatTextEdit->setAcceptDrops(false);
	ui->hashBox->setDropWidget(this);
	ui->hashBox->setAutoHide(true);

	QMenu *menu = new QMenu();
	menu->addAction(ui->actionChooseFont);
	menu->addAction(ui->actionResetFont);
	ui->fontButton->setMenu(menu);

	menu = new QMenu();
	menu->addAction(ui->actionClearChatHistory);
	menu->addAction(ui->actionDeleteChatHistory);
	menu->addAction(ui->actionSaveChatHistory);
	menu->addAction(ui->actionMessageHistory);
	ui->pushtoolsButton->setMenu(menu);

	ui->chatTextEdit->installEventFilter(this);

#if QT_VERSION < 0x040700
	// embedded images are not supported before QT 4.7.0
	ui->attachPictureButton->setVisible(false);
#endif

	resetStatusBar();
}

ChatWidget::~ChatWidget()
{
	processSettings(false);

	delete ui;
}

void ChatWidget::setDefaultExtraFileFlags(TransferRequestFlags fl)
{
	mDefaultExtraFileFlags = fl ;
	ui->hashBox->setDefaultTransferRequestFlags(fl) ;
}

void ChatWidget::addChatButton(QPushButton *button)
{
	ui->toolBarFrame->layout()->addWidget(button) ;
}

void ChatWidget::init(const std::string &peerId, const QString &title)
{
	this->peerId = peerId;
	this->title = title;

	ui->titleLabel->setText(title);

	std::string ownId = rsPeers->getOwnId();
	setName(QString::fromUtf8(rsPeers->getPeerName(ownId).c_str()));

	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(peerId, lid)) {
		isChatLobby = true;
		chatStyle.setStyleFromSettings(ChatStyle::TYPE_PUBLIC);
	} else {
		chatStyle.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);
	}

	currentColor.setNamedColor(PeerSettings->getPrivateChatColor(peerId));
	currentFont.fromString(PeerSettings->getPrivateChatFont(peerId));

	colorChanged();
	fontChanged();
	setColorAndFont();

	// load style
	PeerSettings->getStyle(peerId, "ChatWidget", style);

	if (!isChatLobby) {
		// initialize first status
		StatusInfo peerStatusInfo;
		// No check of return value. Non existing status info is handled as offline.
		rsStatus->getStatus(peerId, peerStatusInfo);
		updateStatus(QString::fromStdString(peerId), peerStatusInfo.status);

		// initialize first custom state string
		QString customStateString = QString::fromUtf8(rsMsgs->getCustomStateString(peerId).c_str());
		updatePeersCustomStateString(QString::fromStdString(peerId), customStateString);
	} else {
		updateTitle();
	}

	if (rsHistory->getEnable(false)) {
		// get chat messages from history
		std::list<HistoryMsg> historyMsgs;
		int messageCount = Settings->getPrivateChatHistoryCount();
		if (messageCount > 0) {
			rsHistory->getMessages(peerId, historyMsgs, messageCount);

			std::list<HistoryMsg>::iterator historyIt;
			for (historyIt = historyMsgs.begin(); historyIt != historyMsgs.end(); historyIt++) {
				addChatMsg(historyIt->incoming, QString::fromUtf8(historyIt->peerName.c_str()), QDateTime::fromTime_t(historyIt->sendTime), QDateTime::fromTime_t(historyIt->recvTime), QString::fromUtf8(historyIt->message.c_str()), TYPE_HISTORY);
			}
		}
	}

	processSettings(true);
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
	if (obj == ui->chatTextEdit) {
		if (event->type() == QEvent::KeyPress) {
			updateStatusTyping();

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (isChatLobby) {
					if (keyEvent->key() == Qt::Key_Tab) {
						completeNickname((bool)(keyEvent->modifiers() & Qt::ShiftModifier));
						return true; // eat event
					}
					else {
						completionWord.clear();
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
		std::string vpid;
		if (rsMsgs->getVirtualPeerId(lobbyIt->lobby_id, vpid)) {
			if (vpid == peerId) {
				lobby = &*lobbyIt;
				break;
			}
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
				it++) {
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

void ChatWidget::addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, enumChatType chatType)
{
#ifdef CHAT_DEBUG
	std::cout << "ChatWidget::addChatMsg message : " << message.toStdString() << std::endl;
#endif

	unsigned int formatTextFlag = RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_OPTIMIZE;
	unsigned int formatFlag = 0;

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
	if (chatType == TYPE_OFFLINE) {
		type = ChatStyle::FORMATMSG_OOUTGOING;
	} else if (chatType == TYPE_SYSTEM) {
		type = ChatStyle::FORMATMSG_SYSTEM;
	} else if (chatType == TYPE_HISTORY) {
		type = incoming ? ChatStyle::FORMATMSG_HINCOMING : ChatStyle::FORMATMSG_HOUTGOING;
	} else {
		type = incoming ? ChatStyle::FORMATMSG_INCOMING : ChatStyle::FORMATMSG_OUTGOING;
	}

	if (chatType == TYPE_SYSTEM) {
		formatFlag |= CHAT_FORMATMSG_SYSTEM;
	}

	QString formattedMessage = RsHtml().formatText(ui->textBrowser->document(), message, formatTextFlag, backgroundColor, desiredContrast);
	QString formatMsg = chatStyle.formatMessage(type, name, incoming ? sendTime : recvTime, formattedMessage, formatFlag);

	ui->textBrowser->append(formatMsg);

	resetStatusBar();

	if (incoming && chatType == TYPE_NORMAL) {
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
}

void ChatWidget::pasteLink()
{
	//std::cerr << "In paste link" << std::endl;
	ui->chatTextEdit->insertHtml(RSLinkClipboard::toHtml());
}

void ChatWidget::pasteOwnCertificateLink()
{
	//std::cerr << "In paste own certificate link" << std::endl;
	RetroShareLink link ;
	std::string ownId = rsPeers->getOwnId() ;

	if( link.createCertificate(ownId) )	{
		ui->chatTextEdit->insertHtml(link.toHtml() + " ");
	}
}

void ChatWidget::contextMenu(QPoint point)
{
	std::cerr << "In context menu" << std::endl;

	QMenu *contextMnu = ui->chatTextEdit->createStandardContextMenu(point);

	contextMnu->addSeparator();
	QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
	action->setDisabled(RSLinkClipboard::empty());
	contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste my certificate link"), this, SLOT(pasteOwnCertificateLink()));

	contextMnu->exec(QCursor::pos());
	delete(contextMnu);
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
	ui->statusLabel->clear();
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

		rsMsgs->sendStatusString(peerId, "is typing...");
		lastStatusSendTime = time(NULL) ;
	}
}

void ChatWidget::sendChat()
{
	QTextEdit *chatWidget = ui->chatTextEdit;

	if (chatWidget->toPlainText().isEmpty()) {
		// nothing to send
		return;
	}

	QString text;
	RsHtml::optimizeHtml(chatWidget, text);
	std::wstring msg = text.toStdWString();

	if (msg.empty()) {
		// nothing to send
		return;
	}

#ifdef CHAT_DEBUG
	std::cout << "ChatWidget:sendChat " << std::endl;
#endif

	if (rsMsgs->sendPrivateChat(peerId, msg)) {
		QDateTime currentTime = QDateTime::currentDateTime();
		addChatMsg(false, name, currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL);
	}

	chatWidget->clear();
	// workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
	// QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
	chatWidget->setCurrentCharFormat(QTextCharFormat ());
}

void ChatWidget::on_closeInfoFrameButton_clicked()
{
	ui->infoFrame->setVisible(false);
}

void ChatWidget::chooseColor()
{
	bool ok;
	QRgb color = QColorDialog::getRgba(ui->chatTextEdit->textColor().rgba(), &ok, window());
	if (ok) {
		currentColor = QColor(color);
		PeerSettings->setPrivateChatColor(peerId, currentColor.name());
		colorChanged();
		setColorAndFont();
	}
}

void ChatWidget::colorChanged()
{
	QPixmap pix(16, 16);
	pix.fill(currentColor);
	ui->colorButton->setIcon(pix);
}

void ChatWidget::chooseFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, currentFont, this);
	if (ok) {
		currentFont = font;
		fontChanged();
		setFont();
	}
}

void ChatWidget::resetFont()
{
	currentFont.fromString(Settings->getChatScreenFont());
	fontChanged();
	setFont();
}

void ChatWidget::fontChanged()
{
	ui->textboldButton->setChecked(currentFont.bold());
	ui->textunderlineButton->setChecked(currentFont.underline());
	ui->textitalicButton->setChecked(currentFont.italic());
}

void ChatWidget::setColorAndFont()
{
	currentFont.setBold(ui->textboldButton->isChecked());
	currentFont.setUnderline(ui->textunderlineButton->isChecked());
	currentFont.setItalic(ui->textitalicButton->isChecked());

	ui->chatTextEdit->setFont(currentFont);
	ui->chatTextEdit->setTextColor(currentColor);

	ui->chatTextEdit->setFocus();
}

void ChatWidget::setFont()
{
	setColorAndFont();
	PeerSettings->setPrivateChatFont(peerId, currentFont.toString());
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
}

void ChatWidget::deleteChatHistory()
{
	if ((QMessageBox::question(this, "RetroShare", tr("Do you really want to physically delete the history?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes) {
		clearChatHistory();
		rsHistory->clear(peerId);
	}
}

void ChatWidget::messageHistory()
{
	ImHistoryBrowser imBrowser(peerId, ui->chatTextEdit, window());
	imBrowser.exec();
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
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg)", file)) {
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
			link.createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash));
		else
			link.createExtraFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash),QString::fromStdString(rsPeers->getOwnId()));

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

	/* convert to real html document */
	QTextBrowser textBrowser;
	textBrowser.setHtml(message);
	std::wstring msg = textBrowser.toHtml().toStdWString();

	if (rsMsgs->sendPrivateChat(peerId, msg)) {
		QDateTime currentTime = QDateTime::currentDateTime();
		addChatMsg(false, name, currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL);
	}
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
	if (isChatLobby) {
		// updateTitle is used
		return;
	}

	/* set font size for status  */
	if (peer_id.toStdString() == peerId) {
		// the peers status has changed

		QString peerName = QString::fromUtf8(rsPeers->getPeerName(peerId).c_str());

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

		QString statusString("<span style=\"font-size:11pt; font-weight:500;""\">%1</span>");
		ui->titleLabel->setText(peerName + " (" + statusString.arg(StatusDefs::name(status)) + ")") ;

		peerStatus = status;

		if (atEnd) {
			// scroll to the end
			scrollbar->setValue(scrollbar->maximum());
		}

		emit infoChanged(this);
		emit statusChanged(status);

		return;
	}

	// ignore status change
}

void ChatWidget::updateTitle()
{
	if (!isChatLobby) {
		// updateStatus is used
		return;
	}

	ui->titleLabel->setText(name + "@" + title);
}

void ChatWidget::updatePeersCustomStateString(const QString& peer_id, const QString& status_string)
{
	std::string stdPeerId = peer_id.toStdString();
	QString status_text;

	if (stdPeerId == peerId) {
		// the peers status string has changed
		if (status_string.isEmpty()) {
			ui->statusMessageLabel->hide();
		} else {
			ui->statusMessageLabel->show();
			status_text = RsHtml().formatText(NULL, status_string, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);
			ui->statusMessageLabel->setText(status_text);
		}
	}
}

void ChatWidget::updateStatusString(const QString &statusMask, const QString &statusString)
{
	ui->statusLabel->setText(QString(statusMask).arg(tr(statusString.toAscii()))); // displays info for 5 secs.
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
		PeerSettings->setStyle(peerId, "PopupChatDialog", style);
		return true;
	}

	return false;
}
