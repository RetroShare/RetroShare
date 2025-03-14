/*******************************************************************************
 * gui/feeds/WireNotifyGroupItem.h                                                 *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef WIRENOTIFYGROUPITEM_H
#define WIRENOTIFYGROUPITEM_H

#include <retroshare/rswire.h>
#include "gui/gxs/GxsGroupFeedItem.h"

namespace Ui {
class WireNotifyGroupItem;
}

class FeedHolder;

class WireNotifyGroupItem : public GxsGroupFeedItem
{
    Q_OBJECT

public:
    /** Default Constructor */
    WireNotifyGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate);
    WireNotifyGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsWireGroup &group, bool isHome, bool autoUpdate);
    ~WireNotifyGroupItem();

    bool setGroup(const RsWireGroup &group);

    uint64_t uniqueIdentifier() const override { return hash_64bits("WireNotifyGroupItem " + groupId().toStdString()) ; }

protected:
    /* FeedItem */
    virtual void doExpand(bool open);

    /* GxsGroupFeedItem */
    virtual QString groupName();
    virtual void loadGroup() override;
    virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_WIRE; }

private slots:
    void toggle() override;
    void subscribeWire();

private:
    void fill();
    void setup();
    void addEventHandler();

private:
    RsWireGroup mGroup;

    /** Qt Designer generated object */
    Ui::WireNotifyGroupItem *ui;
    RsEventsHandlerId_t mEventHandlerId;
};

#endif // WIRENOTIFYGROUPITEM_H