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

#include "WikiGroupDialog.h"

#include <retroshare/rswiki.h>
#include <iostream>

WikiGroupDialog::WikiGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, parent, GXS_GROUP_DIALOG_CREATE_MODE)
{

	// To start with we only have open forums - with distribution controls.

        uint32_t enabledFlags = ( GXS_GROUP_FLAGS_ICON        |
                                GXS_GROUP_FLAGS_DESCRIPTION   |
                                GXS_GROUP_FLAGS_DISTRIBUTION  |
                                // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                                GXS_GROUP_FLAGS_SHAREKEYS     |
                                // GXS_GROUP_FLAGS_PERSONALSIGN  |
                                // GXS_GROUP_FLAGS_COMMENTS      |
                                0);

        uint32_t readonlyFlags = 0;

        uint32_t defaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
                                //GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
                                //GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

                                GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
                                //GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
                                //GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
                                //GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

                                //GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
                                GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
                                //GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |

                                //GXS_GROUP_DEFAULTS_COMMENTS_YES         |
                                GXS_GROUP_DEFAULTS_COMMENTS_NO          |
                                0);

	//setFlags(enabledFlags, readonlyFlags, defaultsFlags);
	setFlags(enabledFlags, defaultsFlags);

}

WikiGroupDialog::WikiGroupDialog(const RsWikiCollection &collection, QWidget *parent)
	:GxsGroupDialog(NULL, parent, GXS_GROUP_DIALOG_SHOW_MODE)
{

	// To start with we only have open forums - with distribution controls.

        uint32_t enabledFlags = ( GXS_GROUP_FLAGS_ICON        |
                                GXS_GROUP_FLAGS_DESCRIPTION   |
                                GXS_GROUP_FLAGS_DISTRIBUTION  |
                                GXS_GROUP_FLAGS_SHAREKEYS     |
                                0);

        uint32_t readonlyFlags = 0;

        uint32_t defaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
                                GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
                                GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
                                GXS_GROUP_DEFAULTS_COMMENTS_NO          |
                                0);

	setFlags(enabledFlags, defaultsFlags);

}


bool WikiGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsWikiCollection grp;
	grp.mMeta = meta;
	//grp.mDescription = std::string(desc.toUtf8());

	rsWiki->submitCollection(token, grp);
	return true;
}

QPixmap WikiGroupDialog::service_getLogo()
{
    return QPixmap(); // null pixmap
}

QString WikiGroupDialog::service_getDescription()
{
    return QString();
}

RsGroupMetaData WikiGroupDialog::service_getMeta()
{
    return mGrp.mMeta;
}


#if 0
void WikiGroupDialog::service_loadExistingGroup(const uint32_t &token)
{
        std::cerr << "WikiGroupDialog::service_loadExistingGroup()";
        std::cerr << std::endl;

	RsWikiCollection group;
	if (!rsWiki->getGroupData(token, group))
	{
        	std::cerr << "WikiGroupDialog::service_loadExistingGroup() ERROR Getting Group";
        	std::cerr << std::endl;

		return;
	}

	/* must call metadata loader */	
	loadExistingGroupMetaData(group.mMeta);

	/* now load any extra data we feel like */

}
#endif


