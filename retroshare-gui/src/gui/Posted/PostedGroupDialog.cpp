/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "PostedGroupDialog.h"
#include "util/TokenQueue.h"

#include <retroshare/rsposted.h>
#include <iostream>

#define POSTED_ENABLE_FLAG   ( GXS_GROUP_FLAGS_ICON          | \
                            GXS_GROUP_FLAGS_DESCRIPTION   | \
                            GXS_GROUP_FLAGS_DISTRIBUTION  | \
                            GXS_GROUP_FLAGS_PUBLISHSIGN   | \
                            GXS_GROUP_FLAGS_SHAREKEYS     | \
                            GXS_GROUP_FLAGS_PERSONALSIGN  | \
                            GXS_GROUP_FLAGS_COMMENTS      | \
                            0)

#define POSTED_CREATE_DEFAULT_FLAG ( GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        | \
                              GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     | \
                              GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     | \
                              GXS_GROUP_DEFAULTS_COMMENTS_NO          | \
                              0)

PostedGroupDialog::PostedGroupDialog(TokenQueue* tokenQueue,  RsPosted* posted, QWidget *parent)
        :GxsGroupDialog(tokenQueue, POSTED_ENABLE_FLAG, POSTED_CREATE_DEFAULT_FLAG, parent),
        mPosted(posted)
{
}

PostedGroupDialog::PostedGroupDialog(const RsPostedGroup& grp, uint32_t mode, QWidget *parent)
        :GxsGroupDialog(grp.mMeta, mode, parent), mGrp(grp)
{
}


bool PostedGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
        RsPostedGroup grp;
        grp.mDescription = getDescription().toStdString();
	grp.mMeta = meta;

        rsPosted->submitGroup(token, grp);
	return true;
}






