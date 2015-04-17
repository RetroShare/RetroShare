#pragma once

#include <QDialog>
#include <retroshare/rstypes.h>
#include <retroshare/rsidentity.h>
#include <gui/common/FriendSelectionWidget.h>

class FriendSelectionDialog : public QDialog
{
	public:
        static std::set<RsPgpId> selectFriends_PGP(QWidget *parent,const QString& caption,const QString& header_string,
                                FriendSelectionWidget::Modus  modus   = FriendSelectionWidget::MODUS_MULTI,
                                FriendSelectionWidget::ShowTypes = FriendSelectionWidget::SHOW_GROUP,
                                const std::set<RsPgpId>& pre_selected_ids = std::set<RsPgpId>()) ;

        static std::set<RsPeerId> selectFriends_SSL(QWidget *parent,const QString& caption,const QString& header_string,
                                FriendSelectionWidget::Modus  modus   = FriendSelectionWidget::MODUS_MULTI,
                                FriendSelectionWidget::ShowTypes = FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL,
                                const std::set<RsPeerId>& pre_selected_ids = std::set<RsPeerId>()) ;

        static std::set<RsGxsId> selectFriends_GXS(QWidget *parent,const QString& caption,const QString& header_string,
                                FriendSelectionWidget::Modus  modus   = FriendSelectionWidget::MODUS_MULTI,
                                FriendSelectionWidget::ShowTypes = FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_GXS,
                                const std::set<RsGxsId>& pre_selected_ids = std::set<RsGxsId>()) ;
 
    private:
		virtual ~FriendSelectionDialog() ;
		FriendSelectionDialog(QWidget *parent,const QString& header_string,FriendSelectionWidget::Modus modus,FriendSelectionWidget::ShowTypes show_type,
																	FriendSelectionWidget::IdType pre_selected_id_type,
                                                                    const std::set<std::string>& pre_selected_ids) ;

		FriendSelectionWidget *friends_widget ;
};

