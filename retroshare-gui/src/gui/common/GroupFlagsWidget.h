/*******************************************************************************
 * gui/common/GroupFlagsWidget.h                                               *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <stdint.h>
#include <QPushButton>
#include <QFrame>
#include <retroshare/rsflags.h>

class GroupFlagsWidget: public QWidget
{
	Q_OBJECT

	public:
		GroupFlagsWidget(QWidget *parent,FileStorageFlags flags = FileStorageFlags(0u)) ;
		virtual ~GroupFlagsWidget() ;

		FileStorageFlags flags() const ;
		void setFlags(FileStorageFlags flags) ;

		static QString groupInfoString(FileStorageFlags flags, const QList<QString> &groupNames) ;

	public slots:
		void updated() ;

	protected slots:
        void update_DL_button(bool) ;
        void update_SR_button(bool) ;
        void update_BR_button(bool) ;

	signals:
		void flagsChanged(FileStorageFlags) const ;

	private:
		void update_button_state(bool b,int id) ;

		QPushButton *_buttons[4] ;

		QLayout *_layout ;
        QIcon _icons[6] ;
		FileStorageFlags _flags[4] ;

		static QString _tooltips_on[4] ;
		static QString _tooltips_off[4] ;
};
