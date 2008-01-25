/*
 * libretroshare/src/pqi: p3cfgmgr.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "pqi/p3cfgmgr.h"
#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"




p3ConfigMgr::p3ConfigMgr(std::string dir, std::string fname, std::string signame)
	:basedir(dir), metafname(fname), metasigfname(signame)
{


}

void	p3ConfigMgr::tick()
{
	bool toSave = false;

	/* iterate through and check if any have changed */
	std::map<uint32_t, pqiConfig *>::iterator it;
	for(it = configs.begin(); it != configs.end(); it++)
	{
		if (it->second->ConfInd.Changed(0))
		{
			toSave = true;
		}
	}

	if (toSave)
	{
		saveConfiguration();
	}
}


void	p3ConfigMgr::saveConfiguration()
{
	/* setup metaconfig */

	std::map<uint32_t, pqiConfig *>::iterator it;
	for(it = configs.begin(); it != configs.end(); it++)
	{
		if (it->second->ConfInd.Changed(1))
		{
			it->second->saveConfiguration(basedir);
		}
		/* save metaconfig */
	}

	/* sign the hash of the data */


	/* write signature to configuration */
}

void	p3ConfigMgr::loadConfiguration()
{



}

void	p3ConfigMgr::addConfiguration(uint32_t type, pqiConfig *conf)
{
	configs[type] = conf;
}


p3Config::p3Config(uint32_t t, std::string name)
:pqiConfig(t, name)
{
	return;
}

bool	p3Config::loadConfiguration(std::string basedir, std::string &loadHash)
{
	std::list<RsItem *> load;
	std::list<RsItem *>::iterator it;

	std::string fname = basedir;
	if (fname != "")
		fname += "/";
	fname += Filename();

	BinFileInterface *bio = new BinFileInterface(fname.c_str(), 
				BIN_FLAGS_READABLE | BIN_FLAGS_HASH_DATA);
	pqiarchive archive(setupSerialiser(), bio, BIN_FLAGS_READABLE);
	RsItem *item = NULL;

	while(NULL != (item = archive.GetItem()))
	{
		load.push_back(item);
	}
	/* check hash */
	std::string hashstr = archive.gethash();

	if (hashstr != loadHash)
	{
		/* bad load */
		for(it = load.begin(); it != load.end(); it++)
		{
			delete (*it);
		}
		return false;
	}

	setHash(hashstr);

	/* else okay */
	return loadList(load);
}


bool	p3Config::saveConfiguration(std::string basedir)
{
	bool toKeep;
	std::list<RsItem *> toSave = saveList(toKeep);

	std::string fname = basedir;
	if (fname != "")
		fname += "/";
	fname += Filename();

	BinFileInterface *bio = new BinFileInterface(fname.c_str(), 
				BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);

	uint32_t arch_flags = BIN_FLAGS_WRITEABLE;
	if (toKeep)
		arch_flags |= BIN_FLAGS_NO_DELETE;

	pqiarchive archive(setupSerialiser(), bio, arch_flags);
	std::list<RsItem *>::iterator it;
	for(it = toSave.begin(); it != toSave.end(); it++)
	{
		archive.SendItem(*it);
	}

	/* store the hash */
	setHash(archive.gethash());

	/* else okay */
	return true;
}


/**************************** CONFIGURATION CLASSES ********************/

p3GeneralConfig::p3GeneralConfig()
	:p3Config(CONFIG_TYPE_GENERAL, "gen.set")
{
	return;
}

		// General Configuration System
std::string     p3GeneralConfig::getSetting(std::string opt)
{
	/* extract from config */
	std::map<std::string, std::string>::iterator it;
	if (settings.end() == (it = settings.find(opt)))
	{
		std::string nullstring;
		return nullstring;
	}
	return it->second;
}

void            p3GeneralConfig::setSetting(std::string opt, std::string val)
{
	/* extract from config */
	std::map<std::string, std::string>::iterator it;
	if (settings.end() != (it = settings.find(opt)))
	{
		if (it->second == val)
		{
			/* no change */
			return;
		}
	}

	ConfInd.IndicateChanged();
	settings[opt] = val;

	return;
}


/* TODO ******/

RsSerialiser *p3GeneralConfig::setupSerialiser()
{
	RsSerialiser *rss = NULL;
	return rss;
}

std::list<RsItem *> p3GeneralConfig::saveList(bool &cleanup)
{
	cleanup = false;
	std::list<RsItem *> savelist;
	//RsGenConfItem *item = ...
	std::map<std::string, std::string>::iterator it;
	for(it = settings.begin(); it != settings.end(); it++)
	{


	}
	return savelist;
}

		
bool    p3GeneralConfig::loadList(std::list<RsItem *> load)
{
	/* add into settings */

	return false;
}

