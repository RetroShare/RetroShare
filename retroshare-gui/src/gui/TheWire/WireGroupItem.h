/*******************************************************************************
 * gui/TheWire/WireGroupItem.h                                                 *
 *                                                                             *
 * Copyright (c) 2020 Robert Fernie   <retroshare.project@gmail.com>           *
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

#ifndef MRK_WIRE_GROUP_ITEM_H
#define MRK_WIRE_GROUP_ITEM_H

#include "ui_WireGroupItem.h"

#include <retroshare/rswire.h>

class WireGroupItem;

class WireGroupHolder
{
public:
	virtual ~WireGroupHolder() {}
	virtual void subscribe(RsGxsGroupId &groupId) = 0;
	virtual void unsubscribe(RsGxsGroupId &groupId) = 0;

	virtual void notifyGroupSelection(WireGroupItem *item) = 0;
};

class WireGroupItem : public QWidget, private Ui::WireGroupItem
{
  Q_OBJECT

public:
	WireGroupItem(WireGroupHolder *holder, const RsWireGroup &grp);

	void removeItem();

	void setSelected(bool on);
	bool isSelected();

	const QPixmap *getPixmap();
	RsGxsGroupId &groupId();

private slots:
	void show();
	void subscribe();

protected:
	void mousePressEvent(QMouseEvent *event);

private:
	void setup();
	void setGroupSet();
	void setBackground(QColor color);

	WireGroupHolder *mHolder;
	RsWireGroup mGroup;
	bool mSelected;
};

#endif
