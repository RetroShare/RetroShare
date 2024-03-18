/*******************************************************************************
 * gui/feeds/WireNotifyPostItem.h                                              *
 *                                                                             *
 * Copyright (c) 2012, Robert Fernie   <retroshare.project@gmail.com>          *
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
#ifndef WIRENOTIFYPOSTITEM_H
#define WIRENOTIFYPOSTITEM_H

#include <QMetaType>
#include <QWidget>
#include <retroshare/rswire.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class WireNotifyPostItem;
}

class FeedHolder;
class SubFileItem;

class WireNotifyPostItem : public GxsFeedItem
{
    Q_OBJECT

public:
    // This one is used in NewFeed for incoming Wire posts. Only the group and msg ids are known at this point.
    // It can be used for all apparences of wire posts. But in rder to merge comments from the previous versions of the post, the list of
    // previous posts should be supplied. It's optional. If not supplied only the comments of the new version will be displayed.
    WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());
    WireNotifyPostItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());

    ~WireNotifyPostItem();

    uint64_t uniqueIdentifier() const override { return hash_64bits("WireNotifyPostItem " + messageId().toStdString()); }
    bool setPost(const RsWirePulse& pulse, bool doFill = true);

protected:

    /* FeedItem */
    virtual void doExpand(bool open);
    virtual void expandFill(bool first);

    virtual void paintEvent(QPaintEvent *) override;

    /* GxsGroupFeedItem */
    virtual QString groupName();
    virtual void loadGroup() override;
    virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_WIRE; }

    /* GxsFeedItem */
//    virtual QString messageName();
    virtual void loadMessage();
    virtual void loadComment();
    virtual QString messageName() override;
//    virtual void paintEvent(QPaintEvent *) override;

private:
    void setup();
    void fill();

private:
    bool mInFill;
    bool mCloseOnRead;
    bool mLoaded;

    bool mLoadingMessage;
    bool mLoadingGroup;
    bool mLoadingComment;


    RsGroupMetaData mGroupMeta;
    RsWirePulse mPulse;

    /** Qt Designer generated object */
    Ui::WireNotifyPostItem *ui;
};

Q_DECLARE_METATYPE(RsWirePulse)

#endif // WIRENOTIFYPOSTITEM_H
