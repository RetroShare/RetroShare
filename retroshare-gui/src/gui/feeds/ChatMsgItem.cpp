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

#include <QDateTime>
#include <QTimer>

#include "ChatMsgItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "util/HandleRichText.h"
#include "gui/common/AvatarDefs.h"
#include "gui/settings/rsharesettings.h"
#include "rshare.h"

#include "gui/notifyqt.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ChatMsgItem::ChatMsgItem(FeedHolder *parent, uint32_t feedId, const std::string &peerId, const std::string &message)
:QWidget(NULL), mParent(parent), mFeedId(feedId), mPeerId(peerId)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);
    
    messageFrame->setVisible(false);
    sendButton->hide();
    cancelButton->hide();
    sendButton->setEnabled(false);

    /* general ones */
    connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

    /* specific ones */
    connect( chatButton, SIGNAL( clicked( void ) ), this, SLOT( openChat ( void ) ) );
    connect( msgButton, SIGNAL( clicked( void ) ), this, SLOT( sendMsg ( void ) ) );
    connect( quickmsgButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );
    connect( cancelButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );
    connect( sendButton, SIGNAL( clicked( ) ), this, SLOT( sendMessage() ) );

    connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));

    avatar->setId(mPeerId, false);

    updateItemStatic();
    updateItem();
    insertChat(message);
}

void ChatMsgItem::updateItemStatic()
{
    if (!rsPeers)
        return;

    RsPeerDetails details;
    if (rsPeers->getPeerDetails(mPeerId, details))
    {
        /* set Peer name */
        peerNameLabel->setText(QString::fromUtf8(details.name.c_str()));
    }
    else
    {
        chatButton->setEnabled(false);
        msgButton->setEnabled(false);
    }

    /* fill in */
#ifdef DEBUG_ITEM
    std::cerr << "ChatMsgItem::updateItemStatic()";
    std::cerr << std::endl;
#endif
}

void ChatMsgItem::updateItem()
{
    if (!rsPeers)
        return;

    /* fill in */
#ifdef DEBUG_ITEM
    std::cerr << "ChatMsgItem::updateItem()";
    std::cerr << std::endl;
#endif

    if(!RsAutoUpdatePage::eventsLocked()) {
        RsPeerDetails details;
        if (!rsPeers->getPeerDetails(mPeerId, details))
        {
            return;
        }

        /* do buttons */
        chatButton->setEnabled(details.state & RS_PEER_STATE_CONNECTED);
        if (details.state & RS_PEER_STATE_FRIEND)
        {
            msgButton->setEnabled(true);
        }
        else
        {
            msgButton->setEnabled(false);
        }
    }
    
    /* slow Tick  */
    int msec_rate = 10129;

    QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
    return;
}

void ChatMsgItem::insertChat(const std::string &message)
{
#ifdef DEBUG_ITEM
    std::cerr << "ChatMsgItem::insertChat(): " << msg << std::endl;
#endif

    timestampLabel->setText(Rshare::customLongDate(QDateTime::currentDateTime()));

    QString formatMsg = QString::fromUtf8(message.c_str());

    unsigned int formatFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

    // embed smileys ?
    if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_GroupChat"), true).toBool()) {
        formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
     }

    formatMsg = RsHtml().formatText(NULL, formatMsg, formatFlag);

    chatTextlabel->setText(formatMsg);
}

void ChatMsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChatMsgItem::removeItem()";
	std::cerr << std::endl;
#endif

	mParent->lockLayout(this, true);
	hide();
	mParent->lockLayout(this, false);

	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}

void ChatMsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChatMsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIOSN ***********************/


void ChatMsgItem::sendMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChatMsgItem::sendMsg()";
	std::cerr << std::endl;
#endif

	if (mParent)
	{

    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->addRecipient(MessageComposer::TO, mPeerId, false);
    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
	}
}


void ChatMsgItem::openChat()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChatMsgItem::openChat()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		mParent->openChat(mPeerId);
	}
}

void ChatMsgItem::togglequickmessage()
{
	mParent->lockLayout(this, true);

	if (messageFrame->isHidden())
	{
		messageFrame->setVisible(true);
        sendButton->show();
        cancelButton->show();
    }
	else
	{
		messageFrame->setVisible(false);
        sendButton->hide();
        cancelButton->hide();
    }	

    mParent->lockLayout(this, false);
}

void ChatMsgItem::sendMessage()
{
    /* construct a message */
    MessageInfo mi;
    
    mi.title = tr("Quick Message").toStdWString();
    mi.msg =   quickmsgText->toHtml().toStdWString();
    mi.msgto.push_back(mPeerId);       
    
    rsMsgs->MessageSend(mi);

    quickmsgText->clear();
    messageFrame->setVisible(false);
    sendButton->hide();
    cancelButton->hide();
}

void ChatMsgItem::on_quickmsgText_textChanged()
{
    if (quickmsgText->toPlainText().isEmpty())
    {
        sendButton->setEnabled(false);
    }
    else
    {
        sendButton->setEnabled(true);
    }
}
