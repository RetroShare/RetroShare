/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef CALLTOASTER_H
#define CALLTOASTER_H

#include "IQtToaster.h"

#include <QtCore/QObject>

class QtToaster;

class QWidget;
class QString;
class QPixmap;
namespace Ui { class CallToaster; }

/**
 * Shows a toaster when a phone call is incoming.
 *
 * 
 */
class CallToaster : public QObject, public IQtToaster {
	Q_OBJECT
public:

	CallToaster();

	~CallToaster();

	void setMessage(const QString & message);

	void setPixmap(const QPixmap & pixmap);

	void show();

public Q_SLOTS:

	void close();

Q_SIGNALS:

	void hangUpButtonClicked();

	void pickUpButtonClicked();

private Q_SLOTS:

	void hangUpButtonSlot();

	void pickUpButtonSlot();

private:

	Ui::CallToaster * _ui;

	QWidget * _callToasterWidget;

	QtToaster * _toaster;
};

#endif	//CALLTOASTER_H
