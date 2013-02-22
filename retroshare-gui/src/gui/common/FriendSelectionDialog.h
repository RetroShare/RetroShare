#pragma once

#include <QDialog>
#include <gui/common/FriendSelectionWidget.h>

class FriendSelectionDialog : public QDialog
{
	public:
		static std::list<std::string> selectFriends() ;

	private:
		virtual ~FriendSelectionDialog() ;
		FriendSelectionDialog(QWidget *parent = NULL) ;

		FriendSelectionWidget *friends_widget ;
};

