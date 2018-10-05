/*******************************************************************************
 * libretroshare/src/pqi: p3cfgmgr.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie, Retroshare Team.                      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "util/rsdir.h"
//#include "retroshare/rspeers.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/authssl.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include <errno.h>
#include <rsserver/p3face.h>
#include <util/rsdiscspace.h>
#include "util/rsstring.h"

#include "rsitems/rsconfigitems.h"

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
	std::list<pqiConfig *>::iterator it;
	for(it = mConfigs.begin(); it != mConfigs.end(); ++it)
	{
		if ((*it)->HasConfigChanged(0))
		{

#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::tick() Config Changed - Element: ";
			std::cerr << *it;
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

	std::list<pqiConfig *>::iterator it;
	for(it = mConfigs.begin(); it != mConfigs.end(); ++it)
	{
		if ((*it)->HasConfigChanged(1))
		{
#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::globalSaveConfig() Saving Element: ";
			std::cerr << *it;
			std::cerr << std::endl;
#endif
			ok &= (*it)->saveConfiguration();
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
	std::list<pqiConfig *>::iterator cit;
	RsFileHash dummyHash ;
	for (cit = mConfigs.begin(); cit != mConfigs.end(); ++cit)
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::loadConfig() Element: ";
		std::cerr << *cit <<" Dummy Hash: " << dummyHash;
		std::cerr << std::endl;
#endif

		(*cit)->loadConfiguration(dummyHash);

		/* force config to NOT CHANGED */
		(*cit)->HasConfigChanged(0);
		(*cit)->HasConfigChanged(1);
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
	filename += "config/";
	filename += file;

	std::list<pqiConfig *>::iterator cit = std::find(mConfigs.begin(),mConfigs.end(),conf);
	if (cit != mConfigs.end())
	{
		std::cerr << "p3Config::addConfiguration() Config already added";
		std::cerr << std::endl;
		std::cerr << "\tOriginal filename " << (*cit)->Filename();
		std::cerr << std::endl;
		std::cerr << "\tIgnoring new filename " << filename;
		std::cerr << std::endl;
		return;
	}
	// Also check that the filename is not already registered for another config

	for(std::list<pqiConfig*>::const_iterator it = mConfigs.begin(); it!= mConfigs.end(); ++it)
		if((*it)->filename == filename)
		{
			std::cerr << "!!!!!!!!!! Trying to register a config for file \"" << filename << "\" that is already registered" << std::endl;
			std::cerr << "!!!!!!!!!! Please correct the code !" << std::endl;
			return;
		}

	conf->setFilename(filename);
	mConfigs.push_back(conf);
}


void    p3ConfigMgr::completeConfiguration()
{
	saveConfiguration();

	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	mConfigSaveActive = false;
}



p3Config::p3Config()
	:pqiConfig()
{
	return;
}


bool p3Config::loadConfiguration(RsFileHash& /* loadHash */)
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
		for(it = load.begin(); it != load.end(); ++it)
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
			for(it = load.begin(); it != load.end(); ++it)
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
	pqiSSLstore stream(setupSerialiser(), RsPeerId(), bio, stream_flags);

	if(!stream.getEncryptedItems(load))
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::loadAttempt() Error occurred trying to load Item" << std::endl;
#endif
		return false;
	}

	/* set hash */
	setHash(bio->gethash());

    // In order to check the signature that is stored on disk, we compute the hash of the current data (which should match the hash of the data on disc because we just read it),
    // and validate the signature from the disk on this data. The config file data is therefore hashed twice. Not a security issue, but
    // this is a bit inelegant.

	std::string signatureRead;
	RsFileHash strHash(Hash());

	BinFileInterface bfi(signFname.c_str(), BIN_FLAGS_READABLE);

    if(bfi.getFileSize() == 0)
        return false ;

    RsTemporaryMemory mem(bfi.getFileSize()) ;

    if(!bfi.readdata(mem,mem.size()))
		return false;

    // signature is stored as ascii so we need to convert it back to binary

    RsTemporaryMemory mem2(bfi.getFileSize()/2) ;

    if(!RsUtil::HexToBin(std::string((char*)(unsigned char*)mem,mem.size()),mem2,mem2.size()))
    {
        std::cerr << "Input string is not a Hex string!!"<< std::endl;
        return false ;
	}

	bool signature_checks = AuthSSL::getAuthSSL()->VerifyOwnSignBin(strHash.toByteArray(), RsFileHash::SIZE_IN_BYTES,mem2,mem2.size());

    std::cerr << "(II) checked signature of config file " << cfgFname << ": " << (signature_checks?"OK":"Wrong!") << std::endl;

    return signature_checks;
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

    std::cerr << "(II) Saving configuration file " << cfgFname << std::endl;

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	uint32_t stream_flags = BIN_FLAGS_WRITEABLE;
	bool written = true;

	if (!cleanup)
		stream_flags |= BIN_FLAGS_NO_DELETE;

	BinEncryptedFileInterface *cfg_bio = new BinEncryptedFileInterface(newCfgFname.c_str(), bioflags);
	pqiSSLstore *stream = new pqiSSLstore(setupSerialiser(), RsPeerId(), cfg_bio, stream_flags);

	written = written && stream->encryptedSendItems(toSave);

	if(!written)
		std::cerr << "(EE) Error while writing config file " << Filename() << ": file dropped!!" << std::endl;

	/* store the hash */
	setHash(cfg_bio->gethash());

	// bio is taken care of in stream's destructor, also forces file to close
	delete stream;

	/* sign data */
	std::string signature;
	RsFileHash strHash(Hash());
	AuthSSL::getAuthSSL()->SignData(strHash.toByteArray(),strHash.SIZE_IN_BYTES, signature);

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
	:p3Config()
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
	for(it = settings.begin(); it != settings.end(); ++it)
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
	} else
		delete item;

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
				kit != item->tlvkvs.pairs.end(); ++kit)
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

pqiConfig::pqiConfig()
	: cfgMtx("pqiConfig"), ConfInd(2)
{
	return;
}

pqiConfig::~pqiConfig()
{
	return;
}

const std::string& pqiConfig::Filename()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return filename;
}

const RsFileHash& pqiConfig::Hash()
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

void	pqiConfig::setHash(const RsFileHash& h)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	hash = h;
}

