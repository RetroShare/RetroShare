/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2.1
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

#include "GxsIdChooser.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

/** Constructor */
GxsIdChooser::GxsIdChooser(QWidget *parent)
: QComboBox(parent), mFlags(IDCHOOSER_ANON_DEFAULT), mDefaultId("")
{
	return;
}

void GxsIdChooser::loadIds(uint32_t chooserFlags, RsGxsId defId)
{
	mFlags = chooserFlags;
	mDefaultId = defId;
	loadPrivateIds();
}


bool MakeIdDesc(const RsGxsId &id, QString &desc)
{
	RsIdentityDetails details;
	
	if (!rsIdentity->getIdDetails(id, details))
	{
		return false;
	}

	desc = QString::fromUtf8(details.mNickname.c_str());
	if (details.mPgpLinked)
	{
		desc += " (PGP) [";
	}
	else
	{
		desc += " (Anon) [";
	}
	desc += QString::fromStdString(id.substr(0,5));
	desc += "...]";

	return true;
}


void GxsIdChooser::loadPrivateIds()
{
	std::list<RsGxsId> ids;
	rsIdentity->getOwnIds(ids);


	if (ids.empty())
	{
		std::cerr << "GxsIdChooser::loadPrivateIds() ERROR no ids";
		std::cerr << std::endl;
		return;
	}	

	//rsIdentity->getDefaultId(defId);
	// Prefer to use an application specific default???
	int def = -1;

	if (!(mFlags & IDCHOOSER_ID_REQUIRED))
	{
		/* add No Signature option */
		QString str = "No Signature";
		QString id = "";

		addItem(str, id);
		if (mFlags & IDCHOOSER_ANON_DEFAULT)
		{
			def = 0;
		}
	}
	
	int i = 1;	
	std::list<RsGxsId>::iterator it;
	for(it = ids.begin(); it != ids.end(); it++, i++)
	{
		/* add to Chooser */
		QString str;
		if (!MakeIdDesc(*it, str))
		{
			std::cerr << "GxsIdChooser::loadPrivateIds() ERROR Desc for Id: " << *it;
			std::cerr << std::endl;
			continue;
		}
		QString id = QString::fromStdString(*it);

		addItem(str, id);

		if (mDefaultId == *it)
		{
			def = i;
		}
	}

	if (def >= 0)
	{
		setCurrentIndex(def);
		//ui.comboBox->setCurrentIndex(def);
	}
}

bool GxsIdChooser::getChosenId(RsGxsId &id)
{
	if (count() < 1)
	{
		return false;
	}

	int idx = currentIndex();

	QVariant var = itemData(idx);
	id = var.toString().toStdString();

	return true;
}
		
