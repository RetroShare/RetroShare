/*******************************************************************************
 * gui/common/FriendSelectionDialog.h                                          *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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

