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


#ifndef _WIKI_GROUP_DIALOG_H
#define _WIKI_GROUP_DIALOG_H

#include "GxsGroupDialog.h"
#include "retroshare/rswiki.h"

class WikiGroupDialog : public GxsGroupDialog
{
	Q_OBJECT

public:
	WikiGroupDialog(TokenQueue *tokenQueue, QWidget *parent);
	WikiGroupDialog(const RsWikiCollection &collection, QWidget *parent);

protected:
        virtual bool service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta);
	virtual QPixmap service_getLogo();
	virtual QString service_getDescription();
	virtual RsGroupMetaData service_getMeta();

	//virtual void service_loadExistingGroup(const uint32_t &token);

private:

    RsWikiCollection mGrp;

};

#endif

