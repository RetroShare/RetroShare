#include <QMenu>

#include "SubscribeToolButton.h"

/* Use MenuButtonPopup, because the arrow of InstantPopup is too small */
#define USE_MENUBUTTONPOPUP

SubscribeToolButton::SubscribeToolButton(QWidget *parent) :
    QToolButton(parent)
{
	mSubscribed = false;

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
		setIcon(QIcon(":/images/accepted16.png"));
		setText(tr("Subscribed"));

		QMenu *menu = new QMenu;
		menu->addAction(QIcon(":/images/cancel.png"), tr("Unsubscribe"), this, SLOT(unsubscribePrivate()));

		if (!mSubscribedActions.empty()) {
			menu->addSeparator();
			menu->addActions(mSubscribedActions);
		}
		setMenu(menu);

#ifndef USE_MENUBUTTONPOPUP
		disconnect(this, SIGNAL(clicked()), this, SLOT(subscribePrivate()));
#endif
	} else {
		setPopupMode(QToolButton::DelayedPopup);
		setMenu(NULL);
		setIcon(QIcon(":/images/RSS_004_32.png"));
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
