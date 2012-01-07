#pragma once

#include "ui_ChatLobbyWidget.h"
#include "RsAutoUpdatePage.h"

class ChatLobbyWidget : public RsAutoUpdatePage, public Ui::ChatLobbyWidget 
{
	Q_OBJECT

	public:
		/** Default constructor */
		ChatLobbyWidget(QWidget *parent = 0, Qt::WFlags flags = 0);

		/** Default destructor */
		~ChatLobbyWidget();

		virtual void updateDisplay() ;
};

