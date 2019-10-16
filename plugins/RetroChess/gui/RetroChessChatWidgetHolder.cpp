

#include <QToolButton>
#include <QPropertyAnimation>
#include <QIcon>
#include <QLayout>


#include "interface/rsRetroChess.h"

#include "gui/chat/ChatWidget.h"

#include "RetroChessChatWidgetHolder.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>

#define IMAGE_RetroChess ":/images/chess.png"

RetroChessChatWidgetHolder::RetroChessChatWidgetHolder(ChatWidget *chatWidget, RetroChessNotify *notify)
 : QObject(), ChatWidgetHolder(chatWidget), mRetroChessNotify(notify)
{
    QIcon icon ;
    icon.addPixmap(QPixmap(IMAGE_RetroChess)) ;


	playChessButton = new QToolButton ;
	playChessButton->setIcon(icon) ;
	playChessButton->setToolTip(tr("Invite Friend to Chess"));
	playChessButton->setIconSize(QSize(35,35)) ;
	playChessButton->setAutoRaise(true) ;

	mChatWidget->addChatBarWidget(playChessButton);
	connect(playChessButton, SIGNAL(clicked()), this , SLOT(chessPressed()));
	connect(notify, SIGNAL(chessInvited(RsPeerId)), this , SLOT(chessnotify(RsPeerId)));

}

RetroChessChatWidgetHolder::~RetroChessChatWidgetHolder()
{

	button_map::iterator it = buttonMapTakeChess.begin();
	while (it != buttonMapTakeChess.end()) {
		it = buttonMapTakeChess.erase(it);
  }
}

void RetroChessChatWidgetHolder::chessnotify(RsPeerId from_peer_id)
{
	RsPeerId peer_id =  mChatWidget->getChatId().toPeerId();//TODO support GXSID
	//if (peer_id!=from_peer_id)return;//invite from another chat
	if (rsRetroChess->hasInviteFrom(peer_id)){
    if (mChatWidget) {
        QString buttonName = QString::fromUtf8(rsPeers->getPeerName(peer_id).c_str());
        if (buttonName.isEmpty()) buttonName = "Chess";//TODO maybe change all with GxsId
        //disable old buttons
        button_map::iterator it = buttonMapTakeChess.begin();
        while (it != buttonMapTakeChess.end()) {
            it = buttonMapTakeChess.erase(it);
         }
        //button_map::iterator it = buttonMapTakeChess.find(buttonName);
        //if (it == buttonMapTakeChess.end()){
          mChatWidget->addChatMsg(true, tr("Chess Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
                                  , tr("%1 inviting you to start Chess. Do you want to accept or decline the invitation?").arg(buttonName), ChatWidget::MSGTYPE_SYSTEM);
          RSButtonOnText *button = mChatWidget->getNewButtonOnTextBrowser(tr("Accept"));
          button->setToolTip(tr("Accept"));
          button->setStyleSheet(QString("border: 1px solid #199909;")
                                .append("font-size: 12pt;  color: white;")
                                .append("min-width: 128px; min-height: 24px;")
                                .append("border-radius: 6px;")
                                .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                          "stop: 0 #22c70d, stop: 1 #116a06);")

                                );

          button->updateImage();

          connect(button,SIGNAL(clicked()),this,SLOT(chessStart()));
          connect(button,SIGNAL(mouseEnter()),this,SLOT(botMouseEnter()));
          connect(button,SIGNAL(mouseLeave()),this,SLOT(botMouseLeave()));

          buttonMapTakeChess.insert(buttonName, button);
        //}
      }
	

	}
}

void RetroChessChatWidgetHolder::chessPressed()
{
	RsPeerId peer_id =  mChatWidget->getChatId().toPeerId();//TODO support GXSID
	if (rsRetroChess->hasInviteFrom(peer_id)){
	
		rsRetroChess->acceptedInvite(peer_id);
		mRetroChessNotify->notifyChessStart(peer_id);
		return;
	
	}
    rsRetroChess->sendInvite(peer_id);
	
    QString peerName = QString::fromUtf8(rsPeers->getPeerName(peer_id).c_str());
    mChatWidget->addChatMsg(true, tr("Chess Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			                        , tr("You're now inviting %1 to play Chess").arg(peerName), ChatWidget::MSGTYPE_SYSTEM);
                                   
}

void RetroChessChatWidgetHolder::chessStart()
{
		RsPeerId peer_id =  mChatWidget->getChatId().toPeerId();//TODO support GXSID

		rsRetroChess->acceptedInvite(peer_id);
		mRetroChessNotify->notifyChessStart(peer_id);
		return;
}

void RetroChessChatWidgetHolder::botMouseEnter()
{
	RSButtonOnText *source = qobject_cast<RSButtonOnText *>(QObject::sender());
	if (source){
		source->setStyleSheet(QString("border: 1px solid #333333;")
                          .append("font-size: 12pt; color: white;")
                          .append("min-width: 128px; min-height: 24px;")
                          .append("border-radius: 6px;")
                          .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                  "stop: 0 #444444, stop: 1 #222222);")

                          );
		//source->setDown(true);
	}
}

void RetroChessChatWidgetHolder::botMouseLeave()
{
	RSButtonOnText *source = qobject_cast<RSButtonOnText *>(QObject::sender());
	if (source){
		source->setStyleSheet(QString("border: 1px solid #199909;")
                          .append("font-size: 12pt; color: white;")
                          .append("min-width: 128px; min-height: 24px;")
				                  .append("border-radius: 6px;")
				                  .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                   "stop: 0 #22c70d, stop: 1 #116a06);")

				                   );
		//source->setDown(false);
	}
}
