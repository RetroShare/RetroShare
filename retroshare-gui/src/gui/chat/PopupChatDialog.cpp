/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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

#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QFontDialog>
#include <QDir>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QBuffer>
#include <QTextCodec>
#include <QSound>
#include <sys/stat.h>

#include "PopupChatDialog.h"
#include <gui/RetroShareLink.h>
#include "rshare.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsiface.h>
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "../RsAutoUpdatePage.h"

#include "gui/feeds/AttachFileItem.h"
#include "gui/msgs/MessageComposer.h"
#include <time.h>

#define appDir QApplication::applicationDirPath()

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

#include <sstream>

/*****
 * #define CHAT_DEBUG 1
 *****/


static std::map<std::string, PopupChatDialog *> chatDialogs;

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

/** Default constructor */
PopupChatDialog::PopupChatDialog(std::string id, std::string name, 
				QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), dialogId(id), dialogName(name),
    lastChatTime(0), lastChatName("")
    
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  Settings->loadWidgetInformation(this);
  this->move(qrand()%100, qrand()%100); //avoid to stack multiple popup chat windows on the same position

  m_bInsertOnVisible = true;
  
  loadEmoticons();
  
  last_status_send_time = 0 ;
  styleHtm = ":/qss/chat/default.htm";
  
  /* Hide or show the frames */
  showAvatarFrame(true);  
  ui.infoframe->setVisible(false);
  ui.statusmessagelabel->hide();

  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

  connect(ui.actionAvatar, SIGNAL(triggered()),this, SLOT(getAvatar()));

  connect(ui.chattextEdit, SIGNAL(textChanged ( ) ), this, SLOT(checkChat( ) ));
  
  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));
  connect(ui.addFileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));

  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getFont())); 
  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  connect(ui.emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
  connect(ui.styleButton, SIGNAL(clicked()), SLOT(changeStyle()));
  connect(ui.actionSave_Chat_History, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

  connect(ui.textBrowser, SIGNAL(anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

  connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
  connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

  std::cerr << "Connecting custom context menu" << std::endl;
  ui.chattextEdit->setContextMenuPolicy(Qt::CustomContextMenu) ;
  connect(ui.chattextEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

  // Create the status bar
  resetStatusBar() ;

  //ui.textBrowser->setOpenExternalLinks ( false );
  //ui.textBrowser->setOpenLinks ( false );

  QString title = tr("RetroShare - ") + QString::fromStdString(name)  ;
  setWindowTitle(title);
    
  setWindowIcon(QIcon(QString(":/images/rstray3.png")));
  
  ui.textboldButton->setIcon(QIcon(QString(":/images/edit-bold.png")));
  ui.textunderlineButton->setIcon(QIcon(QString(":/images/edit-underline.png")));
  ui.textitalicButton->setIcon(QIcon(QString(":/images/edit-italic.png")));
  ui.fontButton->setIcon(QIcon(QString(":/images/fonts.png")));
  ui.emoteiconButton->setIcon(QIcon(QString(":/images/emoticons/kopete/kopete020.png")));
  ui.styleButton->setIcon(QIcon(QString(":/images/looknfeel.png")));
  
  ui.textboldButton->setCheckable(true);
  ui.textunderlineButton->setCheckable(true);
  ui.textitalicButton->setCheckable(true);

  setAcceptDrops(true);
  ui.chattextEdit->setAcceptDrops(false);
  
  /*Disabled style Button when will switch chat style RetroShare will crash need to be fix */
  //ui.styleButton->setEnabled(false);
  
  QMenu * toolmenu = new QMenu();
  toolmenu->addAction(ui.actionClear_Chat);
  toolmenu->addAction(ui.actionSave_Chat_History);
  //toolmenu->addAction(ui.action_Disable_Emoticons);
  ui.pushtoolsButton->setMenu(toolmenu);

  mCurrentColor = Qt::black;
  mCurrentFont = QFont("Comic Sans MS", 10);

  colorChanged(mCurrentColor);
  setFont();

  pasteLinkAct = new QAction(QIcon(":/images/pasterslink.png"), tr( "Paste retroshare Link" ), this );
  connect( pasteLinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );

  updateAvatar() ;
  updatePeerAvatar(id) ;

  // load settings
  processSettings(true);

  // initialize first status
  StatusInfo peerStatusInfo;
  // No check of return value. Non existing status info is handled as offline.
  rsStatus->getStatus(dialogId, peerStatusInfo);
  updateStatus(QString::fromStdString(dialogId), peerStatusInfo.status);

  StatusInfo ownStatusInfo;
  if (rsStatus->getOwnStatus(ownStatusInfo)) {
    updateStatus(QString::fromStdString(ownStatusInfo.id), ownStatusInfo.status);
  }

  // initialize first custom state string
  QString customStateString = QString::fromStdString(rsMsgs->getCustomStateString(dialogId));
  updatePeersCustomStateString(QString::fromStdString(dialogId), customStateString);
}

/** Destructor. */
PopupChatDialog::~PopupChatDialog()
{
    // save settings
    processSettings(false);
}

void PopupChatDialog::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("ChatDialog"));

    if (bLoad) {
        // load settings

        // state of splitter
        ui.chatsplitter->restoreState(Settings->value("ChatSplitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        Settings->setValue("ChatSplitter", ui.chatsplitter->saveState());
    }

    Settings->endGroup();
}

/*static*/ PopupChatDialog *PopupChatDialog::getPrivateChat(std::string id, uint chatflags)
{
    /* see if it exists already */
    PopupChatDialog *popupchatdialog = NULL;
    bool show = false;

    if (chatflags & RS_CHAT_REOPEN)
    {
        show = true;
#ifdef PEERS_DEBUG
        std::cerr << "reopen flag so: enable SHOW popupchatdialog()" << std::endl;
#endif
    }

    std::map<std::string, PopupChatDialog *>::iterator it;
    if (chatDialogs.end() != (it = chatDialogs.find(id)))
    {
        /* exists already */
        popupchatdialog = it->second;
    }
    else
    {

        if (chatflags & RS_CHAT_OPEN_NEW)
        {
            RsPeerDetails sslDetails;
            if (rsPeers->getPeerDetails(id, sslDetails)) {
                popupchatdialog = new PopupChatDialog(id, sslDetails.name + " - " + sslDetails.location);
                chatDialogs[id] = popupchatdialog;
#ifdef PEERS_DEBUG
                std::cerr << "new chat so: enable SHOW popupchatdialog()" << std::endl;
#endif

                show = true;
            }
        }
    }

    if (show && popupchatdialog)
    {
#ifdef PEERS_DEBUG
        std::cerr << "SHOWING popupchatdialog()" << std::endl;
#endif

        if (popupchatdialog->isVisible() == false) {
            if (chatflags & RS_CHAT_FOCUS) {
                popupchatdialog->show();
            } else {
                popupchatdialog->showMinimized();
            }
        }
    }

    /* now only do these if the window is visible */
    if (popupchatdialog && popupchatdialog->isVisible())
    {
        if (chatflags & RS_CHAT_FOCUS)
        {
#ifdef PEERS_DEBUG
            std::cerr << "focus chat flag so: GETFOCUS popupchatdialog()" << std::endl;
#endif

            popupchatdialog->getfocus();
        }
        else
        {
#ifdef PEERS_DEBUG
            std::cerr << "no focus chat flag so: FLASH popupchatdialog()" << std::endl;
#endif

            popupchatdialog->flash();
        }
    }
    else
    {
#ifdef PEERS_DEBUG
        std::cerr << "not visible ... so leave popupchatdialog()" << std::endl;
#endif
    }

    return popupchatdialog;
}

/*static*/ void PopupChatDialog::cleanupChat()
{
    std::map<std::string, PopupChatDialog *>::iterator it;
    for (it = chatDialogs.begin(); it != chatDialogs.end(); it++) {
        if (it->second) {
            delete (it->second);
        }
    }

    chatDialogs.clear();
}

/*static*/ void PopupChatDialog::privateChatChanged()
{
    std::list<std::string> ids;
    if (!rsMsgs->getPrivateChatQueueIds(ids)) {
#ifdef PEERS_DEBUG
        std::cerr << "no chat available." << std::endl ;
#endif
        return;
    }

    uint chatflags = Settings->getChatFlags();

    std::list<std::string>::iterator id;
    for (id = ids.begin(); id != ids.end(); id++) {
        PopupChatDialog *pcd = getPrivateChat(*id, chatflags);

        if (pcd) {
            pcd->insertChatMsgs();
        }
    }
}

void PopupChatDialog::chatFriend(std::string id)
{
    if (id.empty()){
        return;
    }
    std::cerr<<" popup dialog chat friend 1"<<std::endl;
    bool oneLocationConnected = false;

    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(id, detail)) {
        return;
    }
    
    if (detail.isOnlyGPGdetail) {
        //let's get the ssl child details, and open all the chat boxes
        std::list<std::string> sslIds;
        rsPeers->getSSLChildListOfGPGId(detail.gpg_id, sslIds);
        for (std::list<std::string>::iterator it = sslIds.begin(); it != sslIds.end(); it++) {
            RsPeerDetails sslDetails;
            if (rsPeers->getPeerDetails(*it, sslDetails)) {
                if (sslDetails.state & RS_PEER_STATE_CONNECTED) {
                    oneLocationConnected = true;
                    getPrivateChat(*it, RS_CHAT_OPEN_NEW | RS_CHAT_REOPEN | RS_CHAT_FOCUS);
                }
            }
        }
    } else {
        if (detail.state & RS_PEER_STATE_CONNECTED) {
            oneLocationConnected = true;
            getPrivateChat(id, RS_CHAT_OPEN_NEW | RS_CHAT_REOPEN | RS_CHAT_FOCUS);
        }
    }

    if (!oneLocationConnected) {
        /* info dialog */
        if ((QMessageBox::question(NULL, tr("Friend Not Online"),tr("Your Friend is offline \nDo you want to send them a Message instead"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes) {
            MessageComposer::msgFriend(id);
        }
    }
}

/*static*/ void PopupChatDialog::updateAllAvatars()
{
    for(std::map<std::string, PopupChatDialog *>::const_iterator it(chatDialogs.begin());it!=chatDialogs.end();++it)
        it->second->updateAvatar() ;
}

void PopupChatDialog::pasteLink()
{
	std::cerr << "In paste link" << std::endl ;
	ui.chattextEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void PopupChatDialog::contextMenu( QPoint point )
{
	std::cerr << "In context menu" << std::endl ;
	if(RSLinkClipboard::empty())
		return ;

	QMenu contextMnu(this);
	contextMnu.addAction( pasteLinkAct);

        contextMnu.exec(QCursor::pos());
}

void PopupChatDialog::resetStatusBar() 
{
        ui.statusLabel->setText(QString("")) ;
}

void PopupChatDialog::updateStatusTyping()
{
    if (time(NULL) - last_status_send_time > 5)	// limit 'peer is typing' packets to at most every 10 sec
    {
#ifdef ONLY_FOR_LINGUIST
        tr("is typing...");
#endif

        rsMsgs->sendStatusString(dialogId, "is typing...");
        last_status_send_time = time(NULL) ;
    }
}

// Called by libretroshare through notifyQt to display the peer's status
//
void PopupChatDialog::updateStatusString(const QString& peer_id, const QString& status_string)
{
    QString status = QString::fromStdString(rsPeers->getPeerName(peer_id.toStdString())) + " " + tr(status_string.toAscii());
    ui.statusLabel->setText(status) ; // displays info for 5 secs.

    QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

/** 
 Overloads the default show() slot so we can set opacity*/

void PopupChatDialog::show()
{

  if(!this->isVisible()) {
    QMainWindow::show();
  } else {
    //QMainWindow::activateWindow();
    //setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    //QMainWindow::raise();
  }
  
}

void PopupChatDialog::getfocus()
{

  QMainWindow::activateWindow();
  setWindowState((windowState() & (~Qt::WindowMinimized)) | Qt::WindowActive);
  QMainWindow::raise();
}

void PopupChatDialog::flash()
{

  if(!this->isVisible()) {
    //QMainWindow::show();
  } else {
    // Want to reduce the interference on other applications.
    //QMainWindow::activateWindow();
    //setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    //QMainWindow::raise();
  }
  
}

void PopupChatDialog::showEvent(QShowEvent *event)
{
    if (m_bInsertOnVisible) {
        insertChatMsgs();
    }
}

void PopupChatDialog::closeEvent (QCloseEvent * event)
{
    Settings->saveWidgetInformation(this);

    hide();
    event->ignore();
}

void PopupChatDialog::updateChat()
{
	/* get chat off queue */

	/* write it out */

}

void PopupChatDialog::insertChatMsgs()
{
    if (isVisible() == false) {
        m_bInsertOnVisible = true;
        return;
    }

    m_bInsertOnVisible = false;

    std::list<ChatInfo> newchat;
    if (!rsMsgs->getPrivateChatQueue(dialogId, newchat))
    {
#ifdef PEERS_DEBUG
        std::cerr << "no chat for " << dialogId << " available." << std::endl ;
#endif
        return;
    }

    std::list<ChatInfo>::iterator it;
    for(it = newchat.begin(); it != newchat.end(); it++) {
        /* are they public? */
        if ((it->chatflags & RS_CHAT_PRIVATE) == 0) {
            /* this should not happen */
            continue;
        }

        addChatMsg(it->rsid, it->msg);
    }

    playsound();
    QApplication::alert(this);
}

void PopupChatDialog::addChatMsg(std::string &id, std::wstring &msg)
{
    QString timestamp = "[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "]";
    QString name = QString::fromStdString(rsPeers->getPeerName(id));
    QString message = QString::fromStdWString(msg);

    //replace http://, https:// and www. with <a href> links
    QRegExp rx("(retroshare://[^ <>]*)|(https?://[^ <>]*)|(www\\.[^ <>]*)");
    int count = 0;
    int pos = 100; //ignore the first 100 char because of the standard DTD ref
    while ( (pos = rx.indexIn(message, pos)) != -1 ) {
        //we need to look ahead to see if it's already a well formed link
        if (message.mid(pos - 6, 6) != "href=\"" && message.mid(pos - 6, 6) != "href='" && message.mid(pos - 6, 6) != "ttp://" ) {
            QString tempMessg = message.left(pos) + "<a href=\"" + rx.cap(count) + "\">" + rx.cap(count) + "</a>" + message.mid(pos + rx.matchedLength(), -1);
            message = tempMessg;
        }
        pos += rx.matchedLength() + 15;
        count ++;
    }

#ifdef CHAT_DEBUG
    std::cout << "PopupChatDialog:addChatMsg message : " << message.toStdString() << std::endl;
#endif

    if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool())
    {
	QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
            i.next();
            foreach(QString code, i.key().split("|"))
                message.replace(code, "<img src=\"" + i.value() + "\" />");
	}
    }
    history /*<< nickColor << color << font << fontSize*/ << timestamp << name << message;


    QString formatMsg = loadEmptyStyle()/*.replace(nickColor)
				    .replace(color)
				    .replace(font)
				    .replace(fontSize)*/
                        .replace("%timestamp%", timestamp)
                        .replace("%name%", name)
                        .replace("%message%", message);


    if ((ui.textBrowser->verticalScrollBar()->maximum() - 30) < ui.textBrowser->verticalScrollBar()->value() ) {
        ui.textBrowser->append(formatMsg + "\n");
    } else {
        //the vertical scroll is not at the bottom, so just update the text, the scroll will stay at the current position
        int scroll = ui.textBrowser->verticalScrollBar()->value();
        ui.textBrowser->setHtml(ui.textBrowser->toHtml() + formatMsg + "\n");
        ui.textBrowser->verticalScrollBar()->setValue(scroll);
        ui.textBrowser->update();
    }
    resetStatusBar() ;
}

void PopupChatDialog::checkChat()
{
	/* if <return> at the end of the text -> we can send it! */
	QTextEdit *chatWidget = ui.chattextEdit;
	std::string txt = chatWidget->toPlainText().toStdString();
	if ('\n' == txt[txt.length()-1] && txt.length()-1 == txt.find('\n')) /* only if on first line! */
		sendChat();
	else
		updateStatusTyping() ;

}


void PopupChatDialog::sendChat()
{
    QTextEdit *chatWidget = ui.chattextEdit;

    std::string ownId;

    {
        rsiface->lockData(); /* Lock Interface */
        const RsConfig &conf = rsiface->getConfig();

        ownId = conf.ownId;

        rsiface->unlockData(); /* Unlock Interface */
    }

    std::wstring msg = chatWidget->toHtml().toStdWString();

#ifdef CHAT_DEBUG 
    std::cout << "PopupChatDialog:sendChat " << styleHtm.toStdString() << std::endl;
#endif

    addChatMsg(ownId, msg);

    rsMsgs->sendPrivateChat(dialogId, msg);
    chatWidget->clear();
    setFont();

    /* redraw send list */
}

/**
 Toggles the ToolBox on and off, changes toggle button text
 */
void PopupChatDialog::showAvatarFrame(bool show)
{
    if (show) {
        ui.avatarframe->setVisible(true);
        ui.avatarFrameButton->setChecked(true);
        ui.avatarFrameButton->setToolTip(tr("Hide Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(tr(":images/hide_toolbox_frame.png")));
    } else {
        ui.avatarframe->setVisible(false);
        ui.avatarFrameButton->setChecked(false);
        ui.avatarFrameButton->setToolTip(tr("Show Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(tr(":images/show_toolbox_frame.png")));
    }
}

void PopupChatDialog::on_closeInfoFrameButton_clicked()
{
    ui.infoframe->setVisible(false);
}

void PopupChatDialog::setColor()
{	    
	bool ok;    
    QRgb color = QColorDialog::getRgba(ui.chattextEdit->textColor().rgba(), &ok, this);
    if (ok) {
        mCurrentColor = QColor(color);
        colorChanged(mCurrentColor);
    }
    setFont();
}

void PopupChatDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
}

void PopupChatDialog::getFont()
{
    bool ok;
    mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    setFont();
}

void PopupChatDialog::setFont()
{

//  mCurrentFont.setBold(ui.textboldButton->isChecked());
  bool flag;
  flag=ui.textboldButton->isChecked();
  mCurrentFont.setBold(flag);
  mCurrentFont.setUnderline(ui.textunderlineButton->isChecked());
  mCurrentFont.setItalic(ui.textitalicButton->isChecked());

  ui.chattextEdit->setFont(mCurrentFont);
  ui.chattextEdit->setTextColor(mCurrentColor);

  ui.chattextEdit->setFocus();

}

void PopupChatDialog::loadEmoticons2()
{
	QDir smdir(QApplication::applicationDirPath() + "/emoticons/kopete");
	//QDir smdir(":/gui/images/emoticons/kopete");
	QFileInfoList sminfo = smdir.entryInfoList(QStringList() << "*.gif" << "*.png", QDir::Files, QDir::Name);
	foreach(QFileInfo info, sminfo)
	{
		QString smcode = info.fileName().replace(".gif", "");
		QString smstring;
		for(int i = 0; i < 9; i+=3)
		{
			smstring += QString((char)smcode.mid(i,3).toInt());
		}
		//qDebug(smstring.toAscii());
		smileys.insert(smstring, info.absoluteFilePath());
	}
}

void PopupChatDialog::loadEmoticons()
{
	QString sm_codes;
	#if defined(Q_OS_WIN32)
	QFile sm_file(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
	#else
	QFile sm_file(QString(":/smileys/emotes.acs"));
	#endif
	if(!sm_file.open(QIODevice::ReadOnly))
	{
		std::cout << "error opening ressource file" << std::endl ;
		return ;
	}
	sm_codes = sm_file.readAll();
	sm_file.close();
	sm_codes.remove("\n");
	sm_codes.remove("\r");
	int i = 0;
	QString smcode;
	QString smfile;
	while(sm_codes[i] != '{')
	{
		i++;
		
	}
	while (i < sm_codes.length()-2)
	{
		smcode = "";
		smfile = "";
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while (sm_codes[i] != '\"')
		{
			smcode += sm_codes[i];
			i++;
			
		}
		i++;
		
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
		{
			smfile += sm_codes[i];
			i++;
		}
		i++;
		if(!smcode.isEmpty() && !smfile.isEmpty())
			#if defined(Q_OS_WIN32)
		    smileys.insert(smcode, smfile);
	        #else
			smileys.insert(smcode, ":/"+smfile);
			#endif

	}
}

//============================================================================

void PopupChatDialog::smileyWidget()
{ 
	qDebug("MainWindow::smileyWidget()");
	QWidget *smWidget = new QWidget(this , Qt::Popup);
    smWidget->setAttribute( Qt::WA_DeleteOnClose);
	smWidget->setWindowTitle("Emoticons");
	smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
	smWidget->setBaseSize( 4*24, (smileys.size()/4)*24  );

    //Warning: this part of code was taken from kadu instant messenger;
    //         It was EmoticonSelector::alignTo(QWidget* w) function there
    //         comments are Polish, I dont' know how does it work...
    // oblicz pozycj� widgetu do kt�rego r�wnamy
    QWidget* w = ui.emoteiconButton;
    QPoint w_pos = w->mapToGlobal(QPoint(0,0));
    // oblicz rozmiar selektora
    QSize e_size = smWidget->sizeHint();
    // oblicz rozmiar pulpitu
    QSize s_size = QApplication::desktop()->size();
    // oblicz dystanse od widgetu do lewego brzegu i do prawego
    int l_dist = w_pos.x();
    int r_dist = s_size.width() - (w_pos.x() + w->width());
    // oblicz pozycj� w zale�no�ci od tego czy po lewej stronie
    // jest wi�cej miejsca czy po prawej
    int x;
    if (l_dist >= r_dist)
        x = w_pos.x() - e_size.width();
    else
        x = w_pos.x() + w->width();
    // oblicz pozycj� y - centrujemy w pionie
    int y = w_pos.y() + w->height()/2 - e_size.height()/2;
    // je�li wychodzi poza doln� kraw�d� to r�wnamy do niej
    if (y + e_size.height() > s_size.height())
        y = s_size.height() - e_size.height();
    // je�li wychodzi poza g�rn� kraw�d� to r�wnamy do niej
    if (y < 0)
         y = 0;
    // ustawiamy selektor na wyliczonej pozycji
    smWidget->move(x, y);
	
	
	x = 0;
    y = 0;
	
	QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		QPushButton *smButton = new QPushButton("", smWidget);
		smButton->setGeometry(x*24, y*24, 24,24);
		smButton->setIconSize(QSize(24,24));
		smButton->setIcon(QPixmap(i.value()));
		smButton->setToolTip(i.key());
		++x;
		if(x > 4)
		{
			x = 0;
			y++;
		}
		connect(smButton, SIGNAL(clicked()), this, SLOT(addSmiley()));
        connect(smButton, SIGNAL(clicked()), smWidget, SLOT(close()));
	}
	
	smWidget->show();
}

//============================================================================

void PopupChatDialog::addSmiley()
{
	ui.chattextEdit->setText(ui.chattextEdit->toHtml() + qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

//============================================================================

QString PopupChatDialog::loadEmptyStyle()
{
#ifdef CHAT_DEBUG 
        std::cout << "PopupChatDialog:loadEmptyStyle " << styleHtm.toStdString() << std::endl;
#endif
	QString ret;
	QFile file(styleHtm);
	//file.open(QIODevice::ReadOnly);
       	if (file.open(QIODevice::ReadOnly)) {
		ret = file.readAll();
		file.close();
		QString styleTmp = styleHtm;
		QString styleCss = styleTmp.remove(styleHtm.lastIndexOf("."), styleHtm.length()-styleHtm.lastIndexOf(".")) + ".css";
		qDebug() << styleCss.toAscii();
		QFile css(styleCss);
		QString tmp;
		if (css.open(QIODevice::ReadOnly)) {
			tmp = css.readAll();
			css.close();
		}
		else {
#ifdef CHAT_DEBUG 
			std::cerr << "PopupChatDialog:loadEmptyStyle " << "Missing file of default css " << std::endl;
#endif
			tmp = "";
		}
		ret.replace("%css-style%", tmp);
		return ret;
       	}
	else {
#ifdef CHAT_DEBUG 
                std::cerr << "PopupChatDialog:loadEmptyStyle " << "Missing file of default style " << std::endl;
#endif
		ret="%timestamp% %name% \n %message% ";
		return ret;
	}
}

void PopupChatDialog::on_actionClear_Chat_triggered()
{
    ui.textBrowser->clear();
}

void PopupChatDialog::changeStyle()
{
	QString newStyle = QFileDialog::getOpenFileName(this, tr("Open Style"),
                                                 appDir + "/style/chat/",
                                                 tr("Styles (*.htm)"));
	if(!newStyle.isEmpty())
	{
		QString wholeChat;
		styleHtm = newStyle;
		
		
		for(int i = 0; i < history.size(); i+=4)
		{
			QString formatMsg = loadEmptyStyle();
			wholeChat += formatMsg.replace("%timestamp%", history.at(i+1))
                                  .replace("%name%", history.at(i+2))
				                  .replace("%message%", history.at(i+3)) + "\n";
		}
		ui.textBrowser->setHtml(wholeChat);
	}
	QTextCursor cursor = ui.textBrowser->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.textBrowser->setTextCursor(cursor);
}

void PopupChatDialog::updatePeerAvatar(const std::string& peer_id)
{
        #ifdef CHAT_DEBUG
        std::cerr << "popupchatDialog::updatePeerAvatar() updating avatar for peer " << peer_id << std::endl ;
        std::cerr << "Requesting avatar image for peer " << peer_id << std::endl ;
        #endif

	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getAvatarData(peer_id,data,size); 

        #ifdef CHAT_DEBUG
        std::cerr << "Image size = " << size << std::endl;
        #endif

        if(size == 0) {
           #ifdef CHAT_DEBUG
	   std::cerr << "Got no image" << std::endl ;
           #endif
	   ui.avatarlabel->setPixmap(QPixmap(":/images/no_avatar_background.png"));
		return ;
	}

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.avatarlabel->setPixmap(pix); // writes image into ba in JPG format

	delete[] data ;
}

void PopupChatDialog::updateAvatar()
{
	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getOwnAvatarData(data,size); 

        #ifdef CHAT_DEBUG
	std::cerr << "Image size = " << size << std::endl ;
        #endif

        if(size == 0) {
            #ifdef CHAT_DEBUG
            std::cerr << "Got no image" << std::endl ;
            #endif
        }

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.myavatarlabel->setPixmap(pix); // writes image into ba in PNGformat

	delete[] data ;
}

void PopupChatDialog::getAvatar()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg)");

	if(!fileName.isEmpty())
	{
		picture = QPixmap(fileName).scaled(96,96, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

		std::cerr << "Sending avatar image down the pipe" << std::endl ;

		// send avatar down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

		std::cerr << "Image size = " << ba.size() << std::endl ;

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()),ba.size()) ;	// last char 0 included.

		updateAvatar() ;
	}
}

void PopupChatDialog::addExtraFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
	    PopupChatDialog::addAttachment(filePath,0);
	}
}

void PopupChatDialog::addExtraPicture()
{
	// select a picture file
    QString qfile = QFileDialog::getOpenFileName(this, "Load Picture File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg)",0,
			        QFileDialog::DontResolveSymlinks);
	std::string filePath=qfile.toStdString();
	if(filePath!="")
	{
		PopupChatDialog::addAttachment(filePath,1); //picture
	}
}

void PopupChatDialog::addAttachment(std::string filePath,int flag) 
{
	    /* add a AttachFileItem to the attachment section */
	    std::cerr << "PopupChatDialog::addExtraFile() hashing file.";
	    std::cerr << std::endl;

	    /* add widget in for new destination */
        AttachFileItem *file = new AttachFileItem(filePath);
	    //file->
	    
	    if(flag==1)
	    	file->setPicFlag(1);

	    ui.vboxLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)), SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
}



void PopupChatDialog::fileHashingFinished(AttachFileItem* file) 
{
    std::cerr << "PopupChatDialog::fileHashingFinished() started.";
    std::cerr << std::endl;

    //check that the file is ok tos end
    if (file->getState() == AFI_STATE_ERROR) {
#ifdef CHAT_DEBUG
        std::cerr << "PopupChatDialog::fileHashingFinished error file is not hashed.";
#endif
        return;
    }

    std::string ownId;

    {
        rsiface->lockData(); /* Lock Interface */
        const RsConfig &conf = rsiface->getConfig();

        ownId = conf.ownId;

        rsiface->unlockData(); /* Unlock Interface */
    }

    QString message;
    QString ext = QFileInfo(QString::fromStdString(file->FileName())).suffix();

    if(file->getPicFlag()==1){
        message+="<img src=\"file:///";
        message+=file->FilePath().c_str();
        message+="\" width=\"100\" height=\"100\">";
        message+="<br>";
    }    
	else if (ext == "ogg" || ext == "mp3" || ext == "MP3"  || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma") 
	{
        message+="<img src=\":/images/audio-x-monkey.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    else if (ext == "avi" || ext == "AVI" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "ogm"
    || ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov"
    || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp")
    {
        message+="<img src=\":/images/video-x-generic.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z"
    || ext == "rar" || ext == "rpm" || ext == "deb")
    {
        message+="<img src=\":/images/application-x-rar.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    


    message+= RetroShareLink(QString::fromStdString(file->FileName()),file->FileSize(),QString::fromStdString(file->FileHash())).toHtmlSize();

#ifdef CHAT_DEBUG
    std::cerr << "PopupChatDialog::anchorClicked message : " << message.toStdString() << std::endl;
#endif

    std::wstring msg = message.toStdWString();

    addChatMsg(ownId, msg);

    rsMsgs->sendPrivateChat(dialogId, msg);
}

void PopupChatDialog::anchorClicked (const QUrl& link ) 
{
#ifdef CHAT_DEBUG
	std::cerr << "PopupChatDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
#endif

	std::list<std::string> srcIds;
	srcIds.push_back(dialogId);
        RetroShareLink::processUrl(link, &srcIds, RSLINK_PROCESS_NOTIFY_ALL);
}

void PopupChatDialog::dropEvent(QDropEvent *event)
{
	if (!(Qt::CopyAction & event->possibleActions()))
	{
		std::cerr << "PopupChatDialog::dropEvent() Rejecting uncopyable DropAction";
		std::cerr << std::endl;

		/* can't do it */
		return;
	}

	std::cerr << "PopupChatDialog::dropEvent() Formats";
	std::cerr << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PopupChatDialog::dropEvent() Urls:";
		std::cerr << std::endl;

		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator uit;
		for(uit = urls.begin(); uit != urls.end(); uit++)
		{
			std::string localpath = uit->toLocalFile().toStdString();
			std::cerr << "Whole URL: " << uit->toString().toStdString();
			std::cerr << std::endl;
			std::cerr << "or As Local File: " << localpath;
			std::cerr << std::endl;

			if (localpath.size() > 0)
			{
				struct stat buf;
				//Check that the file does exist and is not a directory
				if ((-1 == stat(localpath.c_str(), &buf))) {
				    std::cerr << "PopupChatDialog::dropEvent() file does not exists."<< std::endl;
				    QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
				    mb.setButtonText( QMessageBox::Ok, "OK" );
				    mb.exec();
				} else if (S_ISDIR(buf.st_mode)) {
				    std::cerr << "PopupChatDialog::dropEvent() directory not accepted."<< std::endl;
				    QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
				    mb.setButtonText( QMessageBox::Ok, "OK" );
				    mb.exec();
				} else {
				    PopupChatDialog::addAttachment(localpath,false);
				}
			}
		}
	}

	event->setDropAction(Qt::CopyAction);
	event->accept();
}

void PopupChatDialog::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
	std::cerr << "PopupChatDialog::dragEnterEvent() Formats";
	std::cerr << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PopupChatDialog::dragEnterEvent() Accepting Urls";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else
	{
		std::cerr << "PopupChatDialog::dragEnterEvent() No Urls";
		std::cerr << std::endl;
	}
}

bool PopupChatDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.textBrowser->document()->toPlainText();
    ui.textBrowser->document()->setModified(false);
    return true;
}

bool PopupChatDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("Text File (*.txt );;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();    
}

void PopupChatDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.textBrowser->document()->setModified(false);

    setWindowModified(false);
}

void PopupChatDialog::updateStatus(const QString &peer_id, int status)
{
    std::string stdPeerId = peer_id.toStdString();
    
    /* set font size for status  */
    QString statusString("<span style=\"font-size:11pt; font-weight:500;""\">%1</span>");

    if (stdPeerId == dialogId) {
        // the peers status has changed
        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_offline.png); }");
            ui.avatarlabel->setEnabled(false);
            ui.infoframe->setVisible(true);
            ui.infolabel->setText( QString::fromStdString(dialogName) + " " + tr("apears to be Offline.") +"\n" + tr("Messages you send will be lost and not delivered, rs-Mail this contact instead."));
            ui.friendnamelabel->setText( QString::fromStdString(dialogName) + " " + statusString.arg("(Offline)")) ;
            break;

        case RS_STATUS_INACTIVE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_away.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infoframe->setVisible(true);
            ui.infolabel->setText( QString::fromStdString(dialogName) + " " + tr("is Idle and may not reply"));
            ui.friendnamelabel->setText( QString::fromStdString(dialogName) + " " + statusString.arg("(Idle)")) ;
            break;

        case RS_STATUS_ONLINE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_online.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infoframe->setVisible(false);
            ui.friendnamelabel->setText( QString::fromStdString(dialogName) + " " + statusString.arg("(Online)")) ;
            break;

        case RS_STATUS_AWAY:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_away.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infolabel->setText( QString::fromStdString(dialogName) + " " + tr("is Away and may not reply"));
            ui.infoframe->setVisible(true);
            ui.friendnamelabel->setText( QString::fromStdString(dialogName) + " " + statusString.arg("(Away)")) ;
            break;

        case RS_STATUS_BUSY:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_busy.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infolabel->setText( QString::fromStdString(dialogName) + " " + tr("is Busy and may not reply"));
            ui.infoframe->setVisible(true);
            ui.friendnamelabel->setText( QString::fromStdString(dialogName) + " " + statusString.arg("(Busy)")) ;
            break;
        }
        return;
    }

    if (stdPeerId == rsPeers->getOwnId()) {
        // my status has changed
        
        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_offline.png); }");
            break;

        case RS_STATUS_INACTIVE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_away.png); }");
            break;

        case RS_STATUS_ONLINE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_online.png); }");
            break;

        case RS_STATUS_AWAY:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_away.png); }");
            break;

        case RS_STATUS_BUSY:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_busy.png); }");
            break;
        }
        
        return;
    }

    // ignore status change
}

void PopupChatDialog::updatePeersCustomStateString(const QString& peer_id, const QString& status_string)
{
    std::string stdPeerId = peer_id.toStdString();

    if (stdPeerId == dialogId) {
        // the peers status string has changed
        if (status_string.isEmpty()) {
            ui.statusmessagelabel->hide();
        } else {
            ui.statusmessagelabel->show();
            ui.statusmessagelabel->setText(status_string);
        }
    }
}

