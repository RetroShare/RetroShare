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

const uint32_t WikiCreateEnabledFlags = ( GXS_GROUP_FLAGS_ICON        |
                          GXS_GROUP_FLAGS_DESCRIPTION   |
                          GXS_GROUP_FLAGS_DISTRIBUTION  |
                          // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                          GXS_GROUP_FLAGS_SHAREKEYS     |
                          // GXS_GROUP_FLAGS_PERSONALSIGN  |
                          // GXS_GROUP_FLAGS_COMMENTS      |
                          0);

uint32_t WikiCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
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


WikiGroupDialog::WikiGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, WikiCreateEnabledFlags, WikiCreateDefaultsFlags, parent, "Create New Wiki Group")
{

	// To start with we only have open forums - with distribution controls.
#if 0
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
#endif

}

WikiGroupDialog::WikiGroupDialog(const RsWikiCollection &collection, QWidget *parent)
    	:GxsGroupDialog(collection.mMeta, GXS_GROUP_DIALOG_SHOW_MODE, parent)

{
#if 0

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
#endif

}


bool WikiGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsWikiCollection grp;
	grp.mMeta = meta;
	//grp.mDescription = std::string(desc.toUtf8());
	std::cerr << "WikiGroupDialog::service_CreateGroup() storing to Queue";
	std::cerr << std::endl;

	rsWiki->submitCollection(token, grp);

	return true;
}


