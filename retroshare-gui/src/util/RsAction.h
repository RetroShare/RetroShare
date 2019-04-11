/*******************************************************************************
 * util/RsAction.h                                                             *
 *                                                                             *
 * Copyright (c) 2007 DrBob          <retroshare.project@gmail.com>            *
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

#ifndef RETRO_ACTION_H
#define RETRO_ACTION_H


#include <QAction>

class  RsAction : public QAction {
	Q_OBJECT
public:

	RsAction(QWidget * parent, std::string rsid);
	RsAction(const QString & text, QObject * parent, std::string rsid);
	RsAction(const QIcon & icon, const QString & text, QObject * parent , std::string rsid);

public Q_SLOTS:

	/* attached to triggered() -> calls triggeredId() */
	void triggerEvent( bool checked );

Q_SIGNALS:

	void triggeredId( std::string rsid );

private:

	std::string RsId;
};

#endif	//RETRO_ACTION_H
