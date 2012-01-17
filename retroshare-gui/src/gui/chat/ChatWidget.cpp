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
#include <QSound>

#include "ChatWidget.h"
#include "ui_ChatWidget.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/im_history/ImHistoryBrowser.h"
#include "HandleRichText.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/Emoticons.h"
#include "util/misc.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>

#include <time.h>

/*****
 * #define CHAT_DEBUG 1
 *****/

// play sound when recv a message
void playsound()
{
	Settings->beginGroup("Sound");
		Settings->beginGroup("SoundFilePath");
			QString OnlineSound = Settings->value("NewChatMessage","").toString();
		Settings->endGroup();
		Settings->beginGroup("Enable");
			bool flag = Settings->value("NewChatMessage",false).toBool();
		Settings->endGroup();
	Settings->endGroup();

	if (!OnlineSound.isEmpty() && flag) {
		if (QSound::isAvailable()) {
			QSound::play(OnlineSound);
		}
	}
}

ChatWidget::ChatWidget(QWidget *parent) :
	QWidget(parent), ui(new Ui::ChatWidget)
{
	ui->setupUi(this);

	newMessages = false;
	typing = false;
	peerStatus = 0;
	isChatLobby = false;

	lastStatusSendTime = 0 ;
	chatStyle.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendChat()));
	connect(ui->addFileButton, SIGNAL(clicked()), this , SLOT(addExtraFile()));

	connect(ui->textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
	connect(ui->fontButton, SIGNAL(clicked()), this, SLOT(getFont()));
	connect(ui->colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(ui->emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
	connect(ui->actionSaveChatHistory, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(ui->actionClearChatHistory, SIGNAL(triggered()), this, SLOT(clearChatHistory()));
	connect(ui->actionDeleteChatHistory, SIGNAL(triggered()), this, SLOT(deleteChatHistory()));
	connect(ui->actionMessageHistory, SIGNAL(triggered()), this, SLOT(messageHistory()));

	connect(ui->hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

	connect(ui->chattextEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

	ui->infoframe->setVisible(false);
	ui->statusmessagelabel->hide();

	ui->textboldButton->setIcon(QIcon(QString(":/images/edit-bold.png")));
	ui->textunderlineButton->setIcon(QIcon(QString(":/images/edit-underline.png")));
	ui->textitalicButton->setIcon(QIcon(QString(":/images/edit-italic.png")));
	ui->fontButton->setIcon(QIcon(QString(":/images/fonts.png")));
	ui->emoteiconButton->setIcon(QIcon(QString(":/images/emoticons/kopete/kopete020.png")));

	ui->textboldButton->setCheckable(true);
	ui->textunderlineButton->setCheckable(true);
	ui->textitalicButton->setCheckable(true);

	setAcceptDrops(true);
	ui->chattextEdit->setAcceptDrops(false);
	ui->hashBox->setDropWidget(this);
	ui->hashBox->setAutoHide(true);

	QMenu *toolmenu = new QMenu();
	toolmenu->addAction(ui->actionClearChatHistory);
	toolmenu->addAction(ui->actionDeleteChatHistory);
	toolmenu->addAction(ui->actionSaveChatHistory);
	toolmenu->addAction(ui->actionMessageHistory);
	ui->pushtoolsButton->setMenu(toolmenu);

	ui->chattextEdit->installEventFilter(this);

#ifdef RS_RELEASE_VERSION
	ui->attachPictureButton->setVisible(false);
#endif

	resetStatusBar();
}

ChatWidget::~ChatWidget()
{
	processSettings(false);

	delete ui;
}

void ChatWidget::init(const std::string &peerId, const QString &peerName)
{
	this->peerId = peerId;
	this->peerName = peerName;

	ui->friendnamelabel->setText(peerName);

	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(peerId, lid)) {
		isChatLobby = true;
	}

	currentColor.setNamedColor(PeerSettings->getPrivateChatColor(peerId));
	currentFont.fromString(PeerSettings->getPrivateChatFont(peerId));

	colorChanged(currentColor);
	fontChanged(currentFont);

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
		// currently not possible
		ui->actionDeleteChatHistory->setVisible(false);
		ui->actionMessageHistory->setVisible(false);
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
	if (obj == ui->chattextEdit) {
		if (event->type() == QEvent::KeyPress) {
			updateStatusTyping();

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
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
						ui->chattextEdit->textCursor().insertText("\n");
					} else {
						// send message with Enter
						sendChat();
					}
					return true; // eat event
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

void ChatWidget::addToolsAction(QAction *action)
{
	ui->pushtoolsButton->menu()->addAction(action);
}

void ChatWidget::showEvent(QShowEvent */*event*/)
{
	newMessages = false;
	emit infoChanged(this);
	focusDialog();
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
	ui->chattextEdit->setFocus();
}

void ChatWidget::addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, enumChatType chatType)
{
#ifdef CHAT_DEBUG
	std::cout << "ChatWidget::addChatMsg message : " << message.toStdString() << std::endl;
#endif

	unsigned int formatFlag = CHAT_FORMATMSG_EMBED_LINKS;

	// embed smileys ?
	if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool()) {
		formatFlag |= CHAT_FORMATMSG_EMBED_SMILEYS;
	}

	ChatStyle::enumFormatMessage type;
	if (chatType == TYPE_OFFLINE) {
		type = ChatStyle::FORMATMSG_OOUTGOING;
	} else if (chatType == TYPE_HISTORY) {
		type = incoming ? ChatStyle::FORMATMSG_HINCOMING : ChatStyle::FORMATMSG_HOUTGOING;
	} else {
		type = incoming ? ChatStyle::FORMATMSG_INCOMING : ChatStyle::FORMATMSG_OUTGOING;
	}

	QString formatMsg = chatStyle.formatMessage(type, name, incoming ? sendTime : recvTime, message, formatFlag);

	ui->textBrowser->append(formatMsg);

	/* Scroll to the end */
	QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
	scrollbar->setValue(scrollbar->maximum());

	resetStatusBar();

	if (incoming && chatType == TYPE_NORMAL) {
		playsound();

		emit newMessage(this);

		if (!isVisible() || (window() && (!window()->isActiveWindow() || window()->isMinimized()))) {
			newMessages = true;
		}

		emit infoChanged(this);
	}
}

void ChatWidget::pasteLink()
{
	std::cerr << "In paste link" << std::endl;
	ui->chattextEdit->insertHtml(RSLinkClipboard::toHtml());
}

void ChatWidget::contextMenu(QPoint /*point*/)
{
	std::cerr << "In context menu" << std::endl;

	QMenu *contextMnu = ui->chattextEdit->createStandardContextMenu();

	contextMnu->addSeparator();
	QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
	action->setDisabled(RSLinkClipboard::empty());

	contextMnu->exec(QCursor::pos());
	delete(contextMnu);
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
	QTextEdit *chatWidget = ui->chattextEdit;

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

	std::string ownId = rsPeers->getOwnId();

#ifdef CHAT_DEBUG
	std::cout << "ChatWidget:sendChat " << std::endl;
#endif

	if (rsMsgs->sendPrivateChat(peerId, msg)) {
		QDateTime currentTime = QDateTime::currentDateTime();
		addChatMsg(false, QString::fromUtf8(rsPeers->getPeerName(ownId).c_str()), currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL);
	}

	chatWidget->clear();
	// workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
	// QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
	chatWidget->setCurrentCharFormat(QTextCharFormat ());

	setFont();
}

void ChatWidget::on_closeInfoFrameButton_clicked()
{
	ui->infoframe->setVisible(false);
}

void ChatWidget::setColor()
{
	bool ok;
	QRgb color = QColorDialog::getRgba(ui->chattextEdit->textColor().rgba(), &ok, window());
	if (ok) {
		currentColor = QColor(color);
		PeerSettings->setPrivateChatColor(peerId, currentColor.name());
		colorChanged(currentColor);
	}
	setFont();
}

void ChatWidget::colorChanged(const QColor &c)
{
	QPixmap pix(16, 16);
	pix.fill(c);
	ui->colorButton->setIcon(pix);
}

void ChatWidget::getFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, currentFont, this);
	if (ok) {
		fontChanged(font);
	}
}

void ChatWidget::fontChanged(const QFont &font)
{
	currentFont = font;

	ui->textboldButton->setChecked(currentFont.bold());
	ui->textunderlineButton->setChecked(currentFont.underline());
	ui->textitalicButton->setChecked(currentFont.italic());

	setFont();
}

void ChatWidget::setFont()
{
	currentFont.setBold(ui->textboldButton->isChecked());
	currentFont.setUnderline(ui->textunderlineButton->isChecked());
	currentFont.setItalic(ui->textitalicButton->isChecked());

	ui->chattextEdit->setFont(currentFont);
	ui->chattextEdit->setTextColor(currentColor);

	ui->chattextEdit->setFocus();

	PeerSettings->setPrivateChatFont(peerId, currentFont.toString());
}

void ChatWidget::smileyWidget()
{
	Emoticons::showSmileyWidget(this, ui->emoteiconButton, SLOT(addSmiley()), true);
}

void ChatWidget::addSmiley()
{
	ui->chattextEdit->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
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
	ImHistoryBrowser imBrowser(peerId, ui->chattextEdit, window());
	imBrowser.exec();
}

void ChatWidget::addExtraFile()
{
	QStringList files;
	if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
		ui->hashBox->addAttachments(files/*, 0*/);
	}
}

void ChatWidget::addExtraPicture()
{
	// select a picture file
	QString file;
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg)", file)) {
		ui->hashBox->addAttachments(QStringList(file), HashedFile::Picture);
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
		link.createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash));

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
		std::string ownId = rsPeers->getOwnId();
		QDateTime currentTime = QDateTime::currentDateTime();
		addChatMsg(false, QString::fromUtf8(rsPeers->getPeerName(ownId).c_str()), currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL);
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
	/* set font size for status  */
	if (peer_id.toStdString() == peerId) {
		// the peers status has changed

		switch (status) {
		case RS_STATUS_OFFLINE:
			ui->infoframe->setVisible(true);
			ui->infolabel->setText(peerName + " " + tr("apears to be Offline.") +"\n" + tr("Messages you send will be delivered after Friend is again Online"));
			break;

		case RS_STATUS_INACTIVE:
			ui->infoframe->setVisible(true);
			ui->infolabel->setText(peerName + " " + tr("is Idle and may not reply"));
			break;

		case RS_STATUS_ONLINE:
			ui->infoframe->setVisible(false);
			break;

		case RS_STATUS_AWAY:
			ui->infolabel->setText(peerName + " " + tr("is Away and may not reply"));
			ui->infoframe->setVisible(true);
			break;

		case RS_STATUS_BUSY:
			ui->infolabel->setText(peerName + " " + tr("is Busy and may not reply"));
			ui->infoframe->setVisible(true);
			break;
		}

		QString statusString("<span style=\"font-size:11pt; font-weight:500;""\">%1</span>");
		ui->friendnamelabel->setText(peerName + " (" + statusString.arg(StatusDefs::name(status)) + ")") ;

		peerStatus = status;

		emit infoChanged(this);

		return;
	}

	// ignore status change
}

void ChatWidget::updatePeersCustomStateString(const QString& peer_id, const QString& status_string)
{
	std::string stdPeerId = peer_id.toStdString();
	QString status_text;

	if (stdPeerId == peerId) {
		// the peers status string has changed
		if (status_string.isEmpty()) {
			ui->statusmessagelabel->hide();
		} else {
			ui->statusmessagelabel->show();
			status_text = RsHtml::formatText(status_string, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);
			ui->statusmessagelabel->setText(status_text);
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

bool ChatWidget::setStyle()
{
	if (style.showDialog(window())) {
		PeerSettings->setStyle(peerId, "PopupChatDialog", style);
		return true;
	}

	return false;
}
