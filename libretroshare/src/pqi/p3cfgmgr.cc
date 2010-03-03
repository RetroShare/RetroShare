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

#include "fstream"

using std::ofstream;
using std::ifstream;

#include "util/rsdir.h"
#include "rsiface/rspeers.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/authssl.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/pqinotify.h"
#include <errno.h>

#include "serialiser/rsconfigitems.h"

#define CONFIG_DEBUG 1


p3ConfigMgr::p3ConfigMgr(std::string dir, std::string fname, std::string signame)
        :basedir(dir), metafname(fname), metasigfname(signame),
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
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::saveConfiguration()";
	std::cerr << std::endl;
#endif

	RsConfigKeyValueSet *item = new RsConfigKeyValueSet();

	std::map<uint32_t, pqiConfig *>::iterator it;
	for(it = configs.begin(); it != configs.end(); it++)
	{
		if (it->second->HasConfigChanged(1))
		{
#ifdef CONFIG_DEBUG 
			std::cerr << "p3ConfigMgr::saveConfiguration() Saving Element: ";
			std::cerr << it->first;
			std::cerr << std::endl;
#endif
			it->second->saveConfiguration();
		}
		/* save metaconfig */

#ifdef CONFIG_DEBUG 
		std::cerr << "p3ConfigMgr::saveConfiguration() Element: ";
		std::cerr << it->first << " Hash: " << it->second->Hash();
		std::cerr << std::endl;
#endif
		if (it->second->Hash() == "")
		{
			/* skip if no hash */
			continue;
		}

		RsTlvKeyValue kv;
		{
			std::ostringstream out;
			out << it->first;
			kv.key = out.str();
		}
		kv.value = it->second->Hash();
		item->tlvkvs.pairs.push_back(kv);
	}

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::saveConfiguration() Complete MetaConfigItem: ";
	std::cerr << std::endl;
	item->print(std::cerr, 20);

#endif
	/* construct filename */
	std::string filename1 = basedir;
	std::string filename2 = basedir;
	std::string filename1_tmp, filename2_tmp; // temporary file to do two pass save

	if (basedir != "")
	{
		filename1 += "/";
		filename2 += "/";
	}
	filename1 += metasigfname;
	filename2 += metafname;
	filename1_tmp = filename1 + ".tmp";
	filename2_tmp = filename2 + ".tmp";

	/* Write the data to a stream */
	uint32_t bioflags = BIN_FLAGS_WRITEABLE;
	BinMemInterface *membio = new BinMemInterface(1000, bioflags);
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsGeneralConfigSerialiser());
	pqistore store(rss, "CONFIG", membio, BIN_FLAGS_WRITEABLE);

	store.SendItem(item);

	/* sign data */
	std::string signature;
        AuthSSL::getAuthSSL()->SignData(membio->memptr(), membio->memsize(), signature);

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::saveConfiguration() MetaFile Signature:";
	std::cerr << std::endl;
	std::cerr << signature;
	std::cerr << std::endl;
#endif

	if (!membio->writetofile(filename2_tmp.c_str()))
	{
#ifdef CONFIG_DEBUG 
		std::cerr << "p3ConfigMgr::saveConfiguration() Failed to Write temp MetaFile " << filename2 ;
		std::cerr << std::endl;
#endif
	}


	/* write signature to configuration */
	BinMemInterface *signbio = new BinMemInterface(signature.c_str(), 
					signature.length(), BIN_FLAGS_READABLE);

	if (!signbio->writetofile(filename1_tmp.c_str()))
	{
#ifdef CONFIG_DEBUG 
		std::cerr << "p3ConfigMgr::saveConfiguration() Failed to Write temp MetaSignFile" << filename1 ;
		std::cerr << std::endl;
#endif
	}


	/*
	 * if above written successfully now write actual file  that will be read initially
	 * The aim is that if any of the file savings here fail at least one of these pairings of
	 * signature will be compatible to either config.tmp and config files
	 */

	if (!membio->writetofile(filename1.c_str()) || !signbio->writetofile(filename2.c_str()))
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::saveConfiguration() Failed to Write MetaFile and MetaSignFile " << filename2 ;
		std::cerr << std::endl;
#endif
	}


	delete signbio;


}

void	p3ConfigMgr::loadConfiguration()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::loadConfiguration()";
	std::cerr << std::endl;
#endif

	/* construct filename */
	std::string filename1 = basedir;
	std::string filename2 = basedir;
	std::string filename1_tmp, filename2_tmp;

	if (basedir != "")
	{
		filename1 += "/";
		filename2 += "/";
	}

	filename1 += metasigfname;
	filename2 += metafname;

	// temporary files
	filename1_tmp = filename1 + ".tmp";
	filename2_tmp = filename2 + ".tmp";

	BinMemInterface* membio = new BinMemInterface(1000, BIN_FLAGS_READABLE);

	/*
	 * Will attempt to get signature first from meta file then if that fails try temporary meta files, these will correspond to temp configs
	 */

	bool pass = getSignAttempt(filename2, filename1, membio);

	// if first attempt fails then try and temporary files
	if(!pass){

		#ifdef CONFIG_DEBUG
		std::cerr << "\np3ConfigMgr::loadConfiguration(): Trying to load METACONFIG item and METASIGN with temporary files";
		std::cerr << std::endl;
		#endif

		pass = getSignAttempt(filename2_tmp, filename1_tmp, membio);

		if(!pass){
			#ifdef CONFIG_DEBUG
			std::cerr << "\np3ConfigMgr::loadConfiguration(): failed to load METACONFIG item and METASIGN";
			std::cerr << std::endl;
			#endif

			return;
		}
	}

	membio->fseek(0); /* go back to start of file */

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsGeneralConfigSerialiser());
	pqistore stream(rss, "CONFIG", membio, BIN_FLAGS_READABLE);

	RsItem *rsitem = stream.GetItem();

	RsConfigKeyValueSet *item = dynamic_cast<RsConfigKeyValueSet *>(rsitem);
	if (!item)
	{
		delete rsitem;
		return;
	}

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::loadConfiguration() Loaded MetaConfigItem: ";
	std::cerr << std::endl;
	item->print(std::cerr, 20);

#endif

	/* extract info from KeyValueSet */
	std::list<RsTlvKeyValue>::iterator it;
	for(it = item->tlvkvs.pairs.begin(); it != item->tlvkvs.pairs.end(); it++)
	{
		/* find the configuration */
		uint32_t confId = atoi(it->key.c_str());
		std::string hashin = it->value;

		/*********************** HACK TO CHANGE CACHE CONFIG ID *********
		 * REMOVE IN A MONTH OR TWO
		 */

		if (confId == CONFIG_TYPE_CACHE_OLDID)
		{
			confId = CONFIG_TYPE_CACHE;
		}

		/*********************** HACK TO CHANGE CACHE CONFIG ID *********/

		std::map<uint32_t, pqiConfig *>::iterator cit;
		cit = configs.find(confId);
		if (cit != configs.end())
		{
#ifdef CONFIG_DEBUG 
			std::cerr << "p3ConfigMgr::loadConfiguration() Element: ";
			std::cerr << confId << " Hash: " << hashin;
			std::cerr << std::endl;
#endif
			(cit->second)->loadConfiguration(hashin);
			/* force config to NOT CHANGED */
			cit->second->HasConfigChanged(0);
			cit->second->HasConfigChanged(1);
		}
	}

	delete item;

#ifdef CONFIG_DEBUG 
	std::cerr << "p3ConfigMgr::loadConfiguration() Done!";
	std::cerr << std::endl;
#endif

}


bool p3ConfigMgr::getSignAttempt(std::string& metaConfigFname, std::string& metaSignFname, BinMemInterface* membio){


	#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::getSignAttempt() metaConfigFname : " << metaConfigFname;
	std::cerr << std::endl;
	std::cerr << "p3ConfigMgr::getSignAttempt() metaSignFname : " << metaSignFname;
	std::cerr << std::endl;
	 #endif

	/* read signature */
	BinMemInterface *signbio = new BinMemInterface(1000, BIN_FLAGS_READABLE);

	if (!signbio->readfromfile(metaSignFname.c_str()))
	{
		#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::getSignAttempt() Failed to Load MetaSignFile";
		std::cerr << std::endl;
		#endif

		/* HACK to load the old one (with the wrong directory)
		 * THIS SHOULD BE REMOVED IN A COUPLE OF VERSIONS....
		 * ONLY HERE TO CORRECT BAD MISTAKE IN EARLIER VERSIONS.
		 */

		metaSignFname = metasigfname;
		metaConfigFname = metafname;

		if (!signbio->readfromfile(metaSignFname.c_str()))
		{
		#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::getSignAttempt() HACK: Failed to Load ALT MetaSignFile";
			std::cerr << std::endl;
		#endif
		}
		else
		{
		#ifdef CONFIG_DEBUG
			std::cerr << "p3ConfigMgr::getSignAttempt() HACK: Loaded ALT MetaSignFile";
			std::cerr << std::endl;
		#endif
		}
	}

	std::string oldsignature((char *) signbio->memptr(), signbio->memsize());
	delete signbio;



	if (!membio->readfromfile(metaConfigFname.c_str()))
	{
	#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::getSignAttempt() Failed to Load MetaFile";
	std::cerr << std::endl;
	#endif
	//		delete membio;
	//		return ;
	}

	/* get signature */
	std::string signature;
	AuthSSL::getAuthSSL()->SignData(membio->memptr(), membio->memsize(), signature);

	#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::getSignAttempt() New MetaFile Signature:";
	std::cerr << std::endl;
	std::cerr << signature;
	std::cerr << std::endl;
	#endif

	#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::getSignAttempt() Orig MetaFile Signature:";
	std::cerr << std::endl;
	std::cerr << oldsignature;
	std::cerr << std::endl;
	#endif

	if (signature != oldsignature)
	{
	/* Failed */
	#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::getSignAttempt() Signature Check Failed";
	std::cerr << std::endl;
	#endif
	return false;
	}

#ifdef CONFIG_DEBUG
std::cerr << "p3ConfigMgr::getSignAttempt() Signature Check Passed!";
std::cerr << std::endl;
#endif

	return true;

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


bool	p3Config::loadConfiguration(std::string &loadHash)
{

	std::string fname = Filename();
	std::string fnametmp = fname + ".tmp";
	std::string hashstr;
	std::list<RsItem *> load;

#ifdef CONFIG_DEBUG
	std::string success_fname = fname;
	std::cerr << "p3Config::loadConfiguration(): Attempting to load configuration file" << fname << std::endl;
#endif

	bool pass = getHashAttempt(loadHash, hashstr, fname, load);

	if(!pass){
		pass = getHashAttempt(loadHash, hashstr, fnametmp, load);
#ifdef CONFIG_DEBUG
		success_fname = fnametmp;
#endif

		if(!pass){
#ifdef CONFIG_DEBUG
			std::cerr << "p3Config::loadConfiguratio() ERROR: Failed to get Hash" << std::endl;
			return false;
#endif
		}
	}

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::loadConfiguration(): SUCCESS: configuration file loaded" << success_fname << std::endl;
#endif

	setHash(hashstr);

	// else okay
	return loadList(load);
}




bool p3Config::getHashAttempt(const std::string& loadHash, std::string& hashstr,const std::string& fname,
		std::list<RsItem *>& load){

	std::list<RsItem *>::iterator it;

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	uint32_t stream_flags = BIN_FLAGS_READABLE;

	BinInterface *bio = new BinFileInterface(fname.c_str(), bioflags);
	pqistore stream(setupSerialiser(), "CONFIG", bio, stream_flags);
	RsItem *item = NULL;

	while(NULL != (item = stream.GetItem()))
	{
#ifdef CONFIG_DEBUG 
		std::cerr << "p3Config::loadConfiguration() loaded item:";
		std::cerr << std::endl;
		item->print(std::cerr, 0);
		std::cerr << std::endl;
#endif
		load.push_back(item);
	}

#ifdef CONFIG_DEBUG 
	std::cerr << "p3Config::loadConfiguration() loaded " << load.size();
	std::cerr << " Elements from File: " << fname;
	std::cerr << std::endl;
#endif

	/* check hash */
	hashstr = bio->gethash();

	// if hash then atmpt load with temporary file
	if (hashstr != loadHash)
	{

#ifdef CONFIG_DEBUG 
		std::cerr << "p3Config::loadConfiguration() ERROR: Hash != MATCHloaded";
		std::cerr << std::endl;
#endif

		/* bad load */
		for(it = load.begin(); it != load.end(); it++)
		{
			delete (*it);
		}

		setHash("");

		return false;
	}
	//delete bio;
	return true;
}


bool	p3Config::saveConfiguration()
{

	bool cleanup = true;
	std::list<RsItem *> toSave = saveList(cleanup);


	std::string fname = Filename(); // get configuration file name
	std::string fnametmp = Filename()+".tmp"; // temporary file for two pass save
	std::string fnametmpold =  fnametmp + "_old";

#ifdef CONFIG_DEBUG 
        std::cerr << "Writting p3config file " << fname.c_str() << std::endl ;
        std::cerr << "p3Config::saveConfiguration() toSave " << toSave.size();
	std::cerr << " Elements to File: " << fname;
	std::cerr << std::endl;
#endif

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	uint32_t stream_flags = BIN_FLAGS_WRITEABLE;

	if (!cleanup)
		stream_flags |= BIN_FLAGS_NO_DELETE;

	BinInterface *bio = new BinFileInterface(fnametmp.c_str(), bioflags);
	pqistore *stream = new pqistore(setupSerialiser(), "CONFIG", bio, stream_flags);

	std::list<RsItem *>::iterator it;

	bool written = true ;

	for(it = toSave.begin(); it != toSave.end(); it++)
	{
#ifdef CONFIG_DEBUG 
		std::cerr << "p3Config::saveConfiguration() save item:";
		std::cerr << std::endl;
		(*it)->print(std::cerr, 0);
		std::cerr << std::endl;
#endif
		written = written && stream->SendItem(*it);

//		std::cerr << "written = " << written << std::endl ;
	}

	/* store the hash */
	setHash(bio->gethash());
	saveDone(); /* callback to inherited class to unlock any Mutexes
		     * protecting saveList() data
		     */

	//  so early?
	delete stream ;

	/**
	 * ensures real old config is alway present in file system, leave of config as tmp file
	 */

	if(!written)
		return false ;

        #ifdef CONFIG_DEBUG
        std::cerr << "p3config::saveConfiguration() renaming " << fnametmp.c_str() << " to " << fname.c_str() << std::endl;
        #endif

        // first write old config to old-temp file

        ifstream ifstrm;
		ofstream ofstrm;
		ifstrm.open(fname.c_str(), std::ifstream::binary);

		std::cerr << "p3config::saveConfiguration() Is file open: " <<  ifstrm.is_open();

		// if file does not exist then open temporay file already created
		if(!ifstrm.is_open()){
			ifstrm.close();
			ifstrm.open(fnametmp.c_str(), std::ifstream::binary);
		}

		// determine size of old config file
		ifstrm.seekg(0, std::ios_base::end);
		std::streampos length = ifstrm.tellg();
		ifstrm.seekg(0);

		//read this into a buffer
		char* buff = new char[length];
		ifstrm.read(buff, length);
		ifstrm.close();

		// write to config_old file
		ofstrm.open(fnametmpold.c_str(), std::ofstream::binary);
		ofstrm.write(buff, length);
		ofstrm.close();

		// now rename new temp config file to current(fname) and old config file to temp (fnametmp)
	if(!RsDirUtil::renameFile(fnametmp, fname) || !RsDirUtil::renameFile(fnametmpold, fnametmp)){
		std::cerr << "ERROR!" << std::endl;
	#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::saveConfiguration():  Failed to Rename" << std::endl;
	#endif
	}

	delete buff;
//	delete bio;
	return true;
}


/**************************** CONFIGURATION CLASSES ********************/

p3GeneralConfig::p3GeneralConfig()
	:p3Config(CONFIG_TYPE_GENERAL)
{
	return;
}

		// General Configuration System
std::string     p3GeneralConfig::getSetting(std::string opt)
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

void            p3GeneralConfig::setSetting(std::string opt, std::string val)
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

std::list<RsItem *> p3GeneralConfig::saveList(bool &cleanup)
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG 
	std::cerr << "p3GeneralConfig::saveList() KV sets: " << settings.size();
	std::cerr << std::endl;
#endif

	cleanup = true;
	std::list<RsItem *> savelist;

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

	return savelist;
}

		
bool    p3GeneralConfig::loadList(std::list<RsItem *> load)
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
	:ConfInd(2), type(t)
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

std::string pqiConfig::Filename() 
{ 
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	return filename; 
}	

std::string pqiConfig::Hash()     
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

void    pqiConfig::setFilename(std::string name) 
{ 
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	filename = name; 
}

void	pqiConfig::setHash(std::string h) 
{ 
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/
	hash = h; 
}

