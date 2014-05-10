/*
 * Retroshare Identity.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#pragma once

#include <map>

#include <retroshare/rsidentity.h>


#include "ui_PeopleDialog.h"

#define IMAGE_IDENTITY          ":/images/identity/identities_32.png"

class UIStateHelper;

class PeopleDialog : public RsGxsUpdateBroadcastPage, public Ui::PeopleDialog
{
	Q_OBJECT

	public:
		PeopleDialog(QWidget *parent = 0);

		virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDENTITY) ; } //MainPage
		virtual QString pageName() const { return tr("People") ; } //MainPage
		virtual QString helpText() const { return ""; } //MainPage

	protected:
		virtual void updateDisplay(bool complete);

};

