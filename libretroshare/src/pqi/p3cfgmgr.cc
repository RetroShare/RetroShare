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
#include "retroshare/rspeers.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/authssl.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/pqiarchive.h"
#include "pqi/pqinotify.h"
#include <errno.h>
#include <util/rsdiscspace.h>

#include "serialiser/rsconfigitems.h"

/*
#define CONFIG_DEBUG 1
*/
#define BACKEDUP_SAVE


p3ConfigMgr::p3ConfigMgr(std::string dir, std::string fname, std::string signame)
        :basedir(dir), metafname(fname), metasigfname(signame),
	mConfigSaveActive(true)
{
	oldConfigType = checkForGlobalSigConfig();

	// configuration to load correct global types
	pqiConfig::globalConfigType = oldConfigType;

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

	if(ok && oldConfigType)
		removeOldConfigType();
	return;
}


void p3ConfigMgr::removeOldConfigType()
{
	std::string fName = basedir + "/" + metafname;
	std::string sigfName = basedir + "/" + metasigfname;

	remove(fName.c_str());
	remove(sigfName.c_str());

	//now set globalconfig type to false so mgr saves
	oldConfigType = false;
	pqiConfig::globalConfigType = oldConfigType;

}
void p3ConfigMgr::globalSaveConfig()
{

	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::globalSaveConfig()";
	std::cerr << std::endl;
#endif

	RsConfigKeyValueSet *item = new RsConfigKeyValueSet();

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
			it->second->saveConfiguration();
		}
		/* save metaconfig */

#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::globalSaveConfig() Element: ";
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
	std::cerr << "p3ConfigMgr::globalSaveConfig() Complete MetaConfigItem: ";
	std::cerr << std::endl;
	item->print(std::cerr, 20);

#endif
	/* construct filename */
	std::string fname = basedir;
	std::string sign_fname = basedir;
	std::string fname_backup, sign_fname_backup; // back up files

	if (basedir != "")
	{
		fname += "/";
		sign_fname += "/";
	}

	fname += metafname;
	sign_fname += metasigfname;
	fname_backup = fname + ".tmp";
	sign_fname_backup = sign_fname + ".tmp";

	/* Write the data to a stream */
	uint32_t bioflags = BIN_FLAGS_WRITEABLE;
	BinMemInterface *configbio = new BinMemInterface(1000, bioflags);
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsGeneralConfigSerialiser());
	pqistore store(rss, "CONFIG", configbio, BIN_FLAGS_WRITEABLE);

	store.SendItem(item);

	/* sign data */
	std::string signature;
	AuthSSL::getAuthSSL()->SignData(configbio->memptr(), configbio->memsize(), signature);

    /* write signature to configuration */
    BinMemInterface *signbio = new BinMemInterface(signature.c_str(),
    		signature.length(), BIN_FLAGS_READABLE);

#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::globalSaveConfig() MetaFile Signature:";
	std::cerr << std::endl;
	std::cerr << signature;
	std::cerr << std::endl;
#endif

	// begin two pass save
	backedUpFileSave(fname, fname_backup, sign_fname, sign_fname_backup, configbio, signbio);

	delete signbio;

}

bool p3ConfigMgr::backedUpFileSave(const std::string& fname, const std::string& fname_backup, const std::string& sign_fname,
		const std::string& sign_fname_backup, BinMemInterface* configbio, BinMemInterface* signbio){

	FILE *file = NULL, *sign = NULL;
	char *config_buff=NULL, *sign_buff=NULL;
	int size_file=0, size_sign=0;

	// begin two pass saving by writing to back up file instead
	if (!configbio->writetofile(fname_backup.c_str()) || !signbio->writetofile(sign_fname_backup.c_str()))
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3ConfigMgr::backedupFileSave() Failed write to Backup MetaFiles " << fname_backup
				<< " and " << sign_fname_backup;
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::backedUpFileSave() Save file and keeps a back up " << std::endl;
#endif

	// open file from which to collect buffer
	file = fopen(fname.c_str(), "rb");
	sign = fopen(sign_fname.c_str(), "rb");

	// if failed then create files
	if((file == NULL) || (sign == NULL)){
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave()  failed to open meta files " << fname << std::endl;
#endif

		file = fopen(fname.c_str(), "wb");
		sign = fopen(sign_fname.c_str(), "wb");

		if((file == NULL) || (sign == NULL)){
			std::cerr << "p3Config::backedUpFileSave()  failed to open backup meta files" << fname_backup << std::endl;
			return false;
		}
	}


	//determine file size
	fseek(file, 0L, SEEK_END);
	size_file = ftell(file);
	fseek(file, 0L, SEEK_SET);

	fseek(sign, 0L, SEEK_END);
	size_sign = ftell(sign);
	fseek(sign, 0L, SEEK_SET);

	if((size_file) > 0 && (size_sign > 0)){
		//read this into a buffer
		config_buff = new char[size_file];
		fread(config_buff, 1, size_file, file);

		//read this into a buffer
		sign_buff = new char[size_sign];
		fread(sign_buff, 1, size_sign, sign);
	}

	fclose(file);
	fclose(sign);

	// rename back-up to current file
	if(!RsDirUtil::renameFile(fname_backup, fname)  || !RsDirUtil::renameFile(sign_fname_backup, sign_fname)){
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() Failed to rename backup meta files: " << std::endl
				<< fname_backup << " to " << fname << std::endl
				<< sign_fname_backup << " to " << sign_fname << std::endl;
#endif
		return true;
	}

	if((size_file) > 0 && (size_sign > 0)){

		// now write actual back-up file
		file = fopen(fname_backup.c_str(), "wb");
		sign = fopen(sign_fname_backup.c_str(), "wb");

		if((file == NULL) || (sign == NULL)){
#ifdef CONFIG_DEBUG
			std::cerr << "p3Config::backedUpFileSave()  fopen failed for file: " << fname_backup << std::endl;
#endif
			return true;
		}

		if(size_file != fwrite(config_buff,1,  size_file, file))
			getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "Write error", "Error while writing backup configuration file " + fname_backup + "\nIs your disc full or out of quota ?");

		if(size_sign != fwrite(sign_buff, 1, size_sign, sign))
			getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "Write error", "Error while writing main signature file " + sign_fname_backup + "\nIs your disc full or out of quota ?");
		
		fclose(file);
		fclose(sign);

#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() finished backed up save.  " << std::endl;
#endif

		delete[] config_buff;
		delete[] sign_buff;
	}
    return true;
}

void p3ConfigMgr::loadConfiguration()
{
	if(oldConfigType)
		globalLoadConfig();
	else
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

void	p3ConfigMgr::globalLoadConfig()
{
	RsStackMutex stack(cfgMtx); /***** LOCK STACK MUTEX ****/

#ifdef CONFIG_DEBUG
	std::cerr << "p3ConfigMgr::loadConfiguration()";
	std::cerr << std::endl;
#endif

	/* construct filename */
	std::string fname = basedir;
	std::string sign_fname = basedir;
	std::string fname_backup, sign_fname_backup;

	if (basedir != "")
	{
		fname += "/";
		sign_fname += "/";
	}

	fname += metafname;
	sign_fname += metasigfname;

	// temporary files
	fname_backup = fname + ".tmp";
	sign_fname_backup = sign_fname + ".tmp";

	BinMemInterface* membio = new BinMemInterface(1000, BIN_FLAGS_READABLE);

	// Will attempt to get signature first from meta file then if that fails try temporary meta files, these will correspond to temp configs
	bool pass = getSignAttempt(fname, sign_fname, membio);

	// if first attempt fails then try and temporary files
	if(!pass){

		#ifdef CONFIG_DEBUG
		std::cerr << "\np3ConfigMgr::loadConfiguration(): Trying to load METACONFIG item and METASIGN with temporary files";
		std::cerr << std::endl;
		#endif

		pass = getSignAttempt(fname_backup, sign_fname_backup , membio);

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

			/* force config to CHANGED to force saving into new non-global sig format */
			cit->second->IndicateConfigChanged();
	//		cit->second->HasConfigChanged(0);
	//		cit->second->HasConfigChanged(1);
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

bool p3ConfigMgr::checkForGlobalSigConfig()
{
	bool oldTypeExists;
	FILE *metaFile = NULL, *metaSig = NULL;
	std::string fName = basedir + "/" + metafname;
	std::string sigName = basedir + "/" + metasigfname;

	metaFile = fopen(fName.c_str(), "r");
	metaSig = fopen(sigName.c_str(), "r");

	// check if files exist
	if((metaFile != NULL) && (metaSig != NULL))
	{
		oldTypeExists = true;
		fclose(metaFile);
		fclose(metaSig);
	}
	else
		oldTypeExists =  false;



	return oldTypeExists;


}

p3Config::p3Config(uint32_t t)
	:pqiConfig(t)
{
	return;
}


bool p3Config::loadConfiguration(std::string &loadHash)
{
	if(globalConfigType)
		return loadGlobalConfig(loadHash);
	else
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
		return false;

	std::string signatureStored((char *) signbio->memptr(), signbio->memsize());

	std::string signatureRead;
	std::string strHash(Hash());
	AuthSSL::getAuthSSL()->SignData(strHash.c_str(), strHash.length(), signatureRead);

	if(signatureRead != signatureStored)
		return false;

	delete signbio;

	return true;
}

bool	p3Config::loadGlobalConfig(std::string &loadHash)
{
	bool pass = false;
	std::string cfg_fname = Filename();
	std::string cfg_fname_backup = cfg_fname + ".tmp";
	std::string hashstr;
	std::list<RsItem *> load;

#ifdef CONFIG_DEBUG
	std::string success_fname = cfg_fname;
	std::cerr << "p3Config::loadConfiguration(): Attempting to load configuration file" << cfg_fname << std::endl;
#endif

	pass = getHashAttempt(loadHash, hashstr, cfg_fname, load);

	if(!pass){
                load.clear();
		pass = getHashAttempt(loadHash, hashstr, cfg_fname_backup, load);
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::loadConfiguration() ERROR: Failed to get Hash from " << success_fname << std::endl;
		success_fname = cfg_fname_backup;
#endif

		if(!pass){
#ifdef CONFIG_DEBUG
			std::cerr << "p3Config::loadConfiguration() ERROR: Failed to get Hash from " << success_fname << std::endl;
#endif
			return false;
		}
	}

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::loadConfiguration(): SUCCESS: configuration file loaded" << success_fname << std::endl;
#endif

	setHash(hashstr);
	return loadList(load);
}




bool p3Config::getHashAttempt(const std::string& loadHash, std::string& hashstr,const std::string& cfg_fname,
		std::list<RsItem *>& load){

	std::list<RsItem *>::iterator it;

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	uint32_t stream_flags = BIN_FLAGS_READABLE;

	BinInterface *bio = new BinFileInterface(cfg_fname.c_str(), bioflags);
	PQInterface *stream = NULL;

	std::string tempString, msgConfigFileName;
	std::string filename = Filename() ;
	std::string::reverse_iterator rit = filename.rbegin();


	// get the msgconfig file name
	for(int i =0; (i <= 7) && (rit != filename.rend()); i++)
	{
		tempString.push_back(*rit);
		rit++;
	}

	rit = tempString.rbegin();

	for(; rit !=tempString.rend(); rit++)
		msgConfigFileName.push_back(*rit);

	if(msgConfigFileName == "msgs.cfg")
		stream = new pqiarchive(setupSerialiser(), bio, bioflags);
	else
		stream = new  pqistore(setupSerialiser(), "CONFIG", bio, bioflags);

	RsItem *item = NULL;

	while(NULL != (item = stream->GetItem()))
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
	std::cerr << " Elements from File: " << cfg_fname;
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

	delete stream;
	return true;
}


bool p3Config::saveConfiguration()
{
		return saveConfig();
}

bool p3Config::saveConfig()
{

	bool cleanup = true;
	std::list<RsItem *> toSave = saveList(cleanup);

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


bool	p3Config::saveGlobalConfig()
{

	bool cleanup = true;
	std::list<RsItem *> toSave = saveList(cleanup);


	std::string cfg_fname = Filename(); // get configuration file name
	std::string cfg_fname_backup = Filename()+".tmp"; // backup file for two pass save

#ifdef CONFIG_DEBUG
        std::cerr << "Writting p3config file " << cfg_fname << std::endl ;
        std::cerr << "p3Config::saveConfiguration() toSave " << toSave.size();
	std::cerr << " Elements to File: " << cfg_fname;
	std::cerr << std::endl;
#endif

	// saves current config and keeps back-up (old configuration)
	if(!backedUpFileSave(cfg_fname, cfg_fname_backup, toSave, cleanup)){
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::saveConfiguration(): Failed to save file" << std::endl;
#endif
		return false;
	}

	saveDone(); // callback to inherited class to unlock any Mutexes protecting saveList() data

	return true;
}


bool p3Config::backedUpFileSave(const std::string& cfg_fname, const std::string& cfg_fname_backup, std::list<RsItem* >& itemList,
		bool cleanup){

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	uint32_t stream_flags = BIN_FLAGS_WRITEABLE;
	bool written = true;
	FILE* cfg_file = NULL;
	char* buff=NULL;
	int size_file = 0;

	if (!cleanup)
		stream_flags |= BIN_FLAGS_NO_DELETE;

	BinInterface *bio = new BinFileInterface(cfg_fname_backup.c_str(), bioflags);
	pqistore *stream = new pqistore(setupSerialiser(), "CONFIG", bio, stream_flags);

	std::list<RsItem *>::iterator it;

	for(it = itemList.begin(); it != itemList.end(); it++)
	{
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() save item:";
		std::cerr << std::endl;
		(*it)->print(std::cerr, 0);
		std::cerr << std::endl;
#endif
		written = written && stream->SendItem(*it);

#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() saved ";
#endif

	}

	/* store the hash */
	setHash(bio->gethash());

	// bio is taken care of in stream's destructor
	delete stream;

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::backedUpFileSave() Save file and keeps a back up " << std::endl;
#endif

	// open file from which to collect buffer
	cfg_file = fopen(cfg_fname.c_str(), "rb");

	// if it fails to open, create file,but back-up file will now be empty
	if(cfg_file == NULL){
	#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave()  fopen failed for file: " << cfg_fname << std::endl;
	#endif

		cfg_file = fopen(cfg_fname.c_str(), "wb");

		if(cfg_file == NULL)
		{
			std::cerr << "p3Config::backedUpFileSave()  fopen failed for file:" << cfg_fname << std::endl;
			return false ;
		}
	}

	//determine file size
	fseek(cfg_file, 0L, SEEK_END);
	size_file = ftell(cfg_file);

	if(size_file < 0) // ftell returns -1 when fails
	{
		fclose(cfg_file);
		size_file = 0 ;
	}

	fseek(cfg_file, 0L, SEEK_SET);

#ifdef CONFIG_DEBUG
	std::cerr << "p3Config::backedUpFileSave(): Size of file: " << size_file << std::endl;
#endif

	// no point continuing, empty files all have same hash
	if(size_file > 0){

		//read this into a data buffer
		buff = new char[size_file];
		fread(buff, 1, size_file, cfg_file);
	}

	fclose(cfg_file);

	// rename back-up to current file, if this fails should return false hash's will not match at startup
	if(!RsDirUtil::renameFile(cfg_fname_backup, cfg_fname)){
#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() Failed to rename file" << cfg_fname_backup << " to "
			<< cfg_fname << std::endl;
#endif
		written &= false; // at least one file save should be successful
	}

	if(size_file > 0)
	{

		// now write actual back-up file
		cfg_file = fopen(cfg_fname_backup.c_str(), "wb");

		if(cfg_file == NULL){
#ifdef CONFIG_DEBUG
			std::cerr << "p3Config::backedUpFileSave()  fopen failed for file: " << cfg_fname_backup << std::endl;
#endif
		}

		if(size_file != fwrite(buff, 1, size_file, cfg_file))
		{
			getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "Write error", "Error while writing backup configuration file " + cfg_fname_backup + "\nIs your disc full or out of quota ?");
			fclose(cfg_file);
			return false ;
		}


		fclose(cfg_file);

#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() finished backed up save.  " << std::endl;
#endif


		delete[] buff;
		written |= true; // either backup or current file should have been saved
	}


#ifdef CONFIG_DEBUG
		std::cerr << "p3Config::backedUpFileSave() finished backed up save.  " << std::endl;
#endif

	return written;
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

bool pqiConfig::globalConfigType = false;

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

