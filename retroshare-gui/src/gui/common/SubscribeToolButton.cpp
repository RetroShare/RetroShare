/*******************************************************************************
 * gui/common/SubscribeToolButton.cpp                                          *
 *                                                                             *
 * Copyright (c) 2018, RetroShare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/FilesDefs.h"
#include <QMenu>

#include "SubscribeToolButton.h"

/* Use MenuButtonPopup, because the arrow of InstantPopup is too small */
#define USE_MENUBUTTONPOPUP

SubscribeToolButton::SubscribeToolButton(QWidget *parent) :
    QToolButton(parent)
{
	mSubscribed = false;

    	mMenu = NULL ;
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

#ifdef USE_MENUBUTTONPOPUP
	connect(this, SIGNAL(clicked()), this, SLOT(subscribePrivate()));
#endif

	updateUi();
}

void SubscribeToolButton::setSubscribed(bool subscribed)
{
	if (mSubscribed == subscribed) {
		return;
	}

	mSubscribed = subscribed;

	updateUi();
}

void SubscribeToolButton::addSubscribedAction(QAction *action)
{
	mSubscribedActions.push_back(action);
}

void SubscribeToolButton::updateUi()
{
	if (mSubscribed) {
#ifdef USE_MENUBUTTONPOPUP
		setPopupMode(QToolButton::MenuButtonPopup);
#else
		setPopupMode(QToolButton::InstantPopup);
#endif
        //setIcon(FilesDefs::getIconFromQtResourcePath(":/images/accepted16.png"));
		setText(tr("Subscribed"));

        	if(mMenu != NULL)	// that's because setMenu does not give away memory ownership
		    delete mMenu ;
            
		mMenu = new QMenu;
        mMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/cancel.svg"), tr("Unsubscribe"), this, SLOT(unsubscribePrivate()));

		if (!mSubscribedActions.empty()) {
			mMenu->addSeparator();
			mMenu->addActions(mSubscribedActions);
		}
        
		setMenu(mMenu);

#ifndef USE_MENUBUTTONPOPUP
		disconnect(this, SIGNAL(clicked()), this, SLOT(subscribePrivate()));
#endif
	} else {
		setPopupMode(QToolButton::DelayedPopup);
		setMenu(NULL);
        //setIcon(FilesDefs::getIconFromQtResourcePath(":/images/RSS_004_32.png"));
		setText(tr("Subscribe"));

#ifndef USE_MENUBUTTONPOPUP
		connect(this, SIGNAL(clicked()), this, SLOT(subscribePrivate()));
#endif
	}
}

void SubscribeToolButton::subscribePrivate()
{
	if (menu()) {
#ifdef USE_MENUBUTTONPOPUP
		showMenu();
#endif
		return;
	}

	emit subscribe(true);

	setSubscribed(true);
}

void SubscribeToolButton::unsubscribePrivate()
{
	emit subscribe(false);

	setSubscribed(false);
}
