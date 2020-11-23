/*******************************************************************************
 * gui/common/RsBanListToolButton.cpp                                          *
 *                                                                             *
 * Copyright (C) 2015, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QMenu>

#include "RsBanListToolButton.h"
#include "util/RsNetUtil.h"

#include <retroshare/rsbanlist.h>

/* Use MenuButtonPopup, because the arrow of InstantPopup is too small */
#define USE_MENUBUTTONPOPUP

RsBanListToolButton::RsBanListToolButton(QWidget *parent) :
    QToolButton(parent)
{
	mList = LIST_WHITELIST;
	mMode = MODE_ADD;

	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

#ifdef USE_MENUBUTTONPOPUP
	connect(this, SIGNAL(clicked()), this, SLOT(showMenu()));
#endif

	updateUi();
}

void RsBanListToolButton::setMode(List list, Mode mode)
{
	mList = list;
	mMode = mode;
	updateUi();
}

bool RsBanListToolButton::setIpAddress(const QString &ipAddress)
{
	mIpAddress.clear();

	if (ipAddress.isEmpty()) {
		updateUi();
		return false;
	}

	struct sockaddr_storage addr;
	int bytes ;

	if (!RsNetUtil::parseAddrFromQString(ipAddress, addr, bytes) || bytes != 0) {
		updateUi();
		return false;
	}

	mIpAddress = ipAddress;
	updateUi();

	return true;
}

void RsBanListToolButton::updateUi()
{
#ifdef USE_MENUBUTTONPOPUP
	setPopupMode(QToolButton::MenuButtonPopup);
#else
	setPopupMode(QToolButton::InstantPopup);
#endif

	switch (mList) {
	case LIST_WHITELIST:
		switch (mMode) {
		case MODE_ADD:
			setText(tr("Add IP to whitelist"));
			break;
		case MODE_REMOVE:
			setText(tr("Remove IP from whitelist"));
			break;
		}
		break;
	case LIST_BLACKLIST:
		switch (mMode) {
		case MODE_ADD:
			setText(tr("Add IP to blacklist"));
			break;
		case MODE_REMOVE:
			setText(tr("Remove IP from blacklist"));
			break;
		}
		break;
	}

	if (!mIpAddress.isEmpty()) {
		sockaddr_storage addr ;
		int masked_bytes ;

		if (RsNetUtil::parseAddrFromQString(mIpAddress, addr, masked_bytes)) {
			QMenu *m = new QMenu;

			m->addAction(QString("%1 %2").arg(tr("Only IP"), RsNetUtil::printAddrRange(addr, 0)), this, SLOT(applyIp()))->setData(0);
			m->addAction(QString("%1 %2").arg(tr("Entire range"), RsNetUtil::printAddrRange(addr, 1)), this, SLOT(applyIp()))->setData(1);
			m->addAction(QString("%1 %2").arg(tr("Entire range"), RsNetUtil::printAddrRange(addr, 2)), this, SLOT(applyIp()))->setData(2);

			setMenu(m);
		} else {
			setMenu(NULL);
		}

		setToolTip(mIpAddress);
	} else {
		setMenu(NULL);
		setToolTip("");
	}
}

void RsBanListToolButton::applyIp()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	sockaddr_storage addr;
	int masked_bytes;

	if (!RsNetUtil::parseAddrFromQString(mIpAddress, addr, masked_bytes)) {
		return;
	}

	uint32_t list_type;
	switch (mList) {
		case LIST_BLACKLIST:
			list_type = RSBANLIST_TYPE_BLACKLIST;
		break;
		case LIST_WHITELIST:
		default:
			list_type = RSBANLIST_TYPE_WHITELIST;
		break;
	}

	masked_bytes = action->data().toUInt();
	bool changed = false;

	switch (mMode) {
		case MODE_REMOVE:
			changed = rsBanList->removeIpRange(addr, masked_bytes, list_type);
		break;
		case MODE_ADD:
		default:
			changed = rsBanList->addIpRange(addr, masked_bytes, list_type, "");
		break;
	}

	if (changed) {
		emit banListChanged(RsNetUtil::printAddrRange(addr, masked_bytes));
	}
}
