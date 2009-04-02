/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef DEBUGMESSAGEMANAGER_H
#define DEBUGMESSAGEMANAGER_H

#include <QtGui>
#include <QSettings>

#include "ConnectionI2P.h"

class cDebugMessageManager:public QObject
{
	    Q_OBJECT

	public:
	cDebugMessageManager(cConnectionI2P* I2P);
	~cDebugMessageManager();


	public slots:
	void clearAllMessages();
	const QStringList getAllMessages();

	private slots:
	void NewIncomingDebugMessage(const QString Message);

	signals:
	void newDebugMessage(QString Message);

	private:
	cConnectionI2P* I2P;
	quint32 MaxMessageCount;
	QSettings* settings;
	QStringList Messages;
};
#endif 
