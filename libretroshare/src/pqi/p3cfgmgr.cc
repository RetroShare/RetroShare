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

#include "util/rsdir.h"
//#include "retroshare/rspeers.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/authssl.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/pqiarchive.h"
#include <errno.h>
#include <rsserver/p3face.h>
#include <util/rsdiscspace.h>
#include "util/rsstring.h"

#include "serialiser/rsconfigitems.h"

/*
#define CONFIG_DEBUG 1
*/
#define BACKEDUP_SAVE


p3ConfigMgr::p3ConfigMgr(std::string dir)
        :basedir(dir), cfgMtx("p3ConfigMgr"),
	mConfigSaveActive(true)
{
}

void	p3ConfigMgr::tick()
{
	bool toSave = false;

      {
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

	/* iterate through and check if any have changed */
	std::map<uint32_t, pqiConfig *>::iterator it;
	for(it = configs.begin(); it != configs.end(); it++)
	{
		if (it->second->HasConfigChanged(0))
		{

#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::tick() Config Changed - Element: ";
			std::cerr << it->first;
			std::cerr << std::endl;
#endif

			toSave = true;
		}
	}

	/* disable saving before exit */
	if (!mConfigSaveActive)
	{
		toSave = false;
	}
      }

	if (toSave)
	{
		saveConfiguration();
	}
}


void	p3ConfigMgr::saveConfiguration()
{
	if(!RsDiscSpace::checkForDiscSpace(RS_CONFIG_DIRECTORY))
		return ;

	saveConfig();


}

void p3ConfigMgr::saveConfig()
{

	bool ok= true;

	RsStackMutex stack(cfgMtx);  /***** LOCK STACK MUTEX ****/

	std::map<uint32_t, pqiConfig *>::iterator it;
	for(it = configs.begin(); it != configs.end(); it++)
	{
		if (it->second->HasConfigChanged(1))
		{
#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::globalSaveConfig() Saving Element: ";
			std::cerr << it->first;
			std::cerr << std::endl;
#endif
			ok &= it->second->saveConfiguration();
		}
		/* save metaconfig */
	}
	return;
}


void p3ConfigMgr::loadConfiguration()
{
	loadConfig();

	return;
}

void p3ConfigMgr::loadConfig()
{
	std::map<uint32_t, pqiConfig *>::iterator cit;
	std::string dummyHash = "dummyHash";
	for (cit = configs.begin(); cit != configs.end(); cit++)
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::loadConfig() Element: ";
		std::cerr << cit->first <<"Dummy Hash: " << dummyHash;
		std::cerr << std::endl;
#endif

		cit->second->loadConfiguration(dummyHash);

		/* force config to NOT CHANGED */
		cit->second->HasConfigChanged(0);
		cit->second->HasConfigChanged(1);
	}

	return;
}


void	p3ConfigMgr::addConfiguration(std::string file, pqiConfig *conf)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

	/* construct filename */
	std::string filename = basedir;
	if (basedir != "")
	{
		filename += "/";
	}
	filename += file;

	conf->setFilename(filename);

	std::map<uint32_t, pqiConfig *>::iterator cit = configs.find(conf->Type());
	if (cit != configs.end())
	{
		std::cerr << "p3Config::addConfiguration() WARNING: type " << conf->Type();
		std::cerr << " with filename " << filename;
		std::cerr << " already added with filename " << cit->second->Filename() << std::endl;
	}
	configs[conf->Type()] = conf;
}


void    p3ConfigMgr::completeConfiguration()
{
	saveConfiguration();

	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	mConfigSaveActive = false;
}



p3Config::p3Config(uint32_t t)
	:pqiConfig(t)
{
	return;
}


bool p3Config::loadConfiguration(std::string &loadHash)
{
	return loadConfig();
}

bool p3Config::loadConfig()
{

#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::loadConfig() loading Configuration\n File: " << Filename() << std::endl;
#endif

	bool pass = true;
	std::string cfgFname = Filename();
	std::string cfgFnameBackup = cfgFname + ".tmp";

	std::string signFname = Filename() +".sgn";
	std::string signFnameBackup = signFname + ".tmp";

	std::list<RsItem *> load;
	std::list<RsItem *>::iterator it;

	// try 1st attempt
	if(!loadAttempt(cfgFname, signFname, load))
	{

#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::loadConfig() Failed to Load" << std::endl;
#endif

		/* bad load */
		for(it = load.begin(); it != load.end(); it++)
		{
			delete (*it);
		}
		pass = false;

		load.clear();
	}

	// try 2nd attempt with backup files if first failed
	if(!pass)
	{
		if(!loadAttempt(cfgFnameBackup, signFnameBackup, load))
		{

#ifdef CONFIG_DEBUG
			std::cerr << "p3Config::loadConfig() Failed on 2nd Pass" << std::endl;
#endif

			/* bad load */
			for(it = load.begin(); it != load.end(); it++)
			{
				delete (*it);
			}
			pass = false;
		}
		else
			pass = true;
	}



	if(pass)
		loadList(load);
	else
		return false;

	return pass;
}

bool p3Config::loadAttempt(const std::string& cfgFname,const std::string& signFname, std::list<RsItem *>& load)
{

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::loadAttempt() \nFilename: " << cfgFname <<  std::endl;
#endif


	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	uint32_t stream_flags = BIN_FLAGS_READABLE;

	BinEncryptedFileInterface *bio = new BinEncryptedFileInterface(cfgFname.c_str(), bioflags);
	pqiSSLstore stream(setupSerialiser(), "CONFIG", bio, stream_flags);

	if(!stream.getEncryptedItems(load))
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::loadAttempt() Error occurred trying to load Item" << std::endl;
#endif
		return false;
	}

	/* set hash */
	setHash(bio->gethash());

	BinMemInterface *signbio = new BinMemInterface(1000, BIN_FLAGS_READABLE);

	if(!signbio->readfromfile(signFname.c_str()))
	{
		delete signbio;
		return false;
	}

	std::string signatureStored((char *) signbio->memptr(), signbio->memsize());

	std::string signatureRead;
	std::string strHash(Hash());
	AuthSSL::getAuthSSL()->SignData(strHash.c_str(), strHash.length(), signatureRead);

	delete signbio;

	if(signatureRead != signatureStored)
		return false;

	return true;
}

bool p3Config::saveConfiguration()
{
		return saveConfig();
}

bool p3Config::saveConfig()
{

	bool cleanup = true;
	std::list<RsItem *> toSave;
	saveList(cleanup, toSave);

	// temporarily append new to files as these will replace current configuration
	std::string newCfgFname = Filename() + "_new";
	std::string newSignFname = Filename() + ".sgn" + "_new";

	std::string tmpCfgFname = Filename() + ".tmp";
	std::string tmpSignFname = Filename() + ".sgn" + ".tmp";

	std::string cfgFname = Filename();
	std::string signFname = Filename() + ".sgn";


	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	uint32_t stream_flags = BIN_FLAGS_WRITEABLE;
	bool written = true;

	if (!cleanup)
		stream_flags |= BIN_FLAGS_NO_DELETE;

	BinEncryptedFileInterface *cfg_bio = new BinEncryptedFileInterface(newCfgFname.c_str(), bioflags);
	pqiSSLstore *stream = new pqiSSLstore(setupSerialiser(), "CONFIG", cfg_bio, stream_flags);

	written = written && stream->encryptedSendItems(toSave);

	if(!written)
		std::cerr << "(EE) Error while writing config file " << Filename() << ": file dropped!!" << std::endl;

	/* store the hash */
	setHash(cfg_bio->gethash());

	// bio is taken care of in stream's destructor, also forces file to close
	delete stream;

	/* sign data */
	std::string signature;
	std::string strHash(Hash());
	AuthSSL::getAuthSSL()->SignData(strHash.c_str(),strHash.length(), signature);

    /* write signature to configuration */
    BinMemInterface *signbio = new BinMemInterface(signature.c_str(),
    		signature.length(), BIN_FLAGS_READABLE);

    signbio->writetofile(newSignFname.c_str());

    delete signbio;

    // now rewrite current files to temp files
	// rename back-up to current file
	if(!RsDirUtil::renameFile(cfgFname, tmpCfgFname)  || !RsDirUtil::renameFile(signFname, tmpSignFname)){
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() Failed to rename backup meta files: " << std::endl
				<< cfgFname << " to " << tmpCfgFname << std::endl
				<< signFname << " to " << tmpSignFname << std::endl;
#endif
			written = false;
		}



	// now rewrite current files to temp files
	// rename back-up to current file
	if(!RsDirUtil::renameFile(newCfgFname, cfgFname)  || !RsDirUtil::renameFile(newSignFname, signFname)){
	#ifdef CONFIG_DEBUG
				std::cerr << "p3Config::() Failed to rename meta files: " << std::endl
						<< newCfgFname << " to " << cfgFname << std::endl
						<< newSignFname << " to " << signFname << std::endl;
	#endif

				written = false;
			}



	saveDone(); // callback to inherited class to unlock any Mutexes protecting saveList() data

	return written;

}


/**************************** CONFIGURATION CLASSES ********************/

p3GeneralConfig::p3GeneralConfig()
	:p3Config(CONFIG_TYPE_GENERAL)
{
	return;
}

		// General Configuration System
std::string     p3GeneralConfig::getSetting(const std::string &opt)
{
#ifdef CONFIG_DEBUG
	std::cerr << "p3GeneralConfig::getSetting(" << opt << ")";
	std::cerr << std::endl;
#endif
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

	/* extract from config */
	std::map<std::string, std::string>::iterator it;
	if (settings.end() == (it = settings.find(opt)))
	{
		std::string nullstring;
		return nullstring;
	}
	return it->second;
}

void            p3GeneralConfig::setSetting(const std::string &opt, const std::string &val)
{
#ifdef CONFIG_DEBUG
	std::cerr << "p3GeneralConfig::setSetting(" << opt << " = " << val << ")";
	std::cerr << std::endl;
#endif
	{
		RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

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

		settings[opt] = val;
	}
	/* outside mutex */
	IndicateConfigChanged();

	return;
}

RsSerialiser *p3GeneralConfig::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsGeneralConfigSerialiser());
	return rss;
}

bool p3GeneralConfig::saveList(bool &cleanup, std::list<RsItem *>& savelist)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG
	std::cerr << "p3GeneralConfig::saveList() KV sets: " << settings.size();
	std::cerr << std::endl;
#endif

	cleanup = true;


	RsConfigKeyValueSet *item = new RsConfigKeyValueSet();
	std::map<std::string, std::string>::iterator it;
	for(it = settings.begin(); it != settings.end(); it++)
	{
		RsTlvKeyValue kv;
		kv.key = it->first;
		kv.value = it->second;
		item->tlvkvs.pairs.push_back(kv);

		/* make sure we don't overload it */
		if (item->tlvkvs.TlvSize() > 4000)
		{
			savelist.push_back(item);
			item = new RsConfigKeyValueSet();
		}
	}

	if (item->tlvkvs.pairs.size() > 0)
	{
		savelist.push_back(item);
	}

	return true;
}


bool    p3GeneralConfig::loadList(std::list<RsItem *>& load)
{
#ifdef CONFIG_DEBUG
	std::cerr << "p3GeneralConfig::loadList() count: " << load.size();
	std::cerr << std::endl;
#endif

	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

	/* add into settings */
	RsConfigKeyValueSet *item = NULL;
	std::list<RsItem *>::iterator it;
	std::list<RsTlvKeyValue>::iterator kit;

	for(it = load.begin(); it != load.end();)
	{
		item = dynamic_cast<RsConfigKeyValueSet *>(*it);
		if (item)
		{
			for(kit = item->tlvkvs.pairs.begin();
				kit != item->tlvkvs.pairs.end(); kit++)
			{
				settings[kit->key] = kit->value;
			}
		}

		/* cleanup */
		delete (*it);
		it = load.erase(it);
	}

	return true;
}


/**** MUTEX NOTE:
 * have protected all, but think that
 * only the Indication and hash really need it
 */

pqiConfig::pqiConfig(uint32_t t)
	: cfgMtx("pqiConfig"), ConfInd(2), type(t)
{
	return;
}

pqiConfig::~pqiConfig()
{
	return;
}

uint32_t   pqiConfig::Type()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return type;
}

const std::string& pqiConfig::Filename()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return filename;
}

const std::string& pqiConfig::Hash()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return hash;
}

void	pqiConfig::IndicateConfigChanged()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	ConfInd.IndicateChanged();
}

bool	pqiConfig::HasConfigChanged(uint16_t idx)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return  ConfInd.Changed(idx);
}

void    pqiConfig::setFilename(const std::string& name)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	filename = name;
}

void	pqiConfig::setHash(const std::string& h)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	hash = h;
}

