#pragma once

#include <QDialog>
#include <gui/common/FriendSelectionWidget.h>

class FriendSelectionDialog : public QDialog
{
	public:
        static std::list<RsPgpId> selectFriends_PGP(QWidget *parent,const QString& caption,const QString& header_string,
                                FriendSelectionWidget::Modus  modus   = FriendSelectionWidget::MODUS_MULTI,
                                FriendSelectionWidget::ShowTypes = FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL,
                                const std::list<RsPgpId>& pre_selected_ids = std::list<RsPgpId>()) ;

        static std::list<RsPeerId> selectFriends_SSL(QWidget *parent,const QString& caption,const QString& header_string,
                                FriendSelectionWidget::Modus  modus   = FriendSelectionWidget::MODUS_MULTI,
                                FriendSelectionWidget::ShowTypes = FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL,
                                const std::list<RsPeerId>& pre_selected_ids = std::list<RsPeerId>()) ;
    private:
		virtual ~FriendSelectionDialog() ;
		FriendSelectionDialog(QWidget *parent,const QString& header_string,FriendSelectionWidget::Modus modus,FriendSelectionWidget::ShowTypes show_type,
																	FriendSelectionWidget::IdType pre_selected_id_type,
																	const std::list<std::string>& pre_selected_ids) ;

		FriendSelectionWidget *friends_widget ;
};

