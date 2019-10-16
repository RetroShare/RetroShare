#ifndef RETROCHESSCHATWIDGETHOLDER_H
#define RETROCHESSCHATWIDGETHOLDER_H

#include "RetroChessChatWidgetHolder.h"
#include <gui/chat/ChatWidget.h>
#include <gui/common/RsButtonOnText.h>
#include <gui/RetroChessNotify.h>

class RetroChessChatWidgetHolder : public QObject, public ChatWidgetHolder
{
	Q_OBJECT

public:
	RetroChessChatWidgetHolder(ChatWidget *chatWidget, RetroChessNotify *notify);
	virtual ~RetroChessChatWidgetHolder();

public slots:
	void chessPressed();
  void chessStart();
  void chessnotify(RsPeerId from_peer_id);


private slots:
	void botMouseEnter();
	void botMouseLeave();  
	
protected:
	QToolButton *playChessButton ;
	RetroChessNotify *mRetroChessNotify;
	
	typedef QMap<QString, RSButtonOnText*> button_map;
	button_map buttonMapTakeChess;
};

#endif // RETROCHESSCHATWIDGETHOLDER_H
