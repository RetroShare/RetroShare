/*******************************************************************************
 * libretroshare/src/plugins: pluginmanager.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Cyril Soler <csoler@users.sourceforge.net>                   *
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
#include <string.h>

#include "pluginmanager.h"
#include <dirent.h>

#if 0
#include <serialiser/rsserial.h>
#include <serialiser/rstlvbase.h>
#include <serialiser/rstlvtypes.h>
#endif

#include <rsitems/rspluginitems.h>

#include <rsserver/p3face.h>
#include <util/rsdir.h>
#include <retroshare/rsversion.h>
#include <util/folderiterator.h>
#include <ft/ftserver.h>
#include <retroshare/rsplugin.h>
#include <retroshare/rsfiles.h>
#include <pqi/pqiservice.h>
#include <plugins/rscacheservice.h>
#include <plugins/rspqiservice.h>

// lets disable the plugin system for now, as it's unfinished.
#ifdef WINDOWS_SYS
#include "dlfcn_win32.h"
#else
#include <dlfcn.h>
#endif

// #define DEBUG_PLUGIN_MANAGER 1

std::string RsPluginManager::_plugin_entry_symbol              = "RETROSHARE_PLUGIN_provide" ;
std::string RsPluginManager::_plugin_revision_symbol           = "RETROSHARE_PLUGIN_revision" ;
std::string RsPluginManager::_plugin_API_symbol         		= "RETROSHARE_PLUGIN_api" ;

std::string RsPluginManager::_local_cache_dir ;
std::string RsPluginManager::_remote_cache_dir ;
std::vector<std::string> RsPluginManager::_plugin_directories ;

RsServiceControl   *RsPluginManager::_service_control							= NULL ;

typedef RsPlugin *(*RetroSharePluginEntry)(void) ;
RsPluginHandler *rsPlugins ;

RsPluginManager::RsPluginManager(const RsFileHash &hash)
	: p3Config(),_current_executable_hash(hash)
{
	_allow_all_plugins = false ;
}

bool RsPluginManager::loadConfiguration(RsFileHash &loadHash)
{
	return p3Config::loadConfiguration(loadHash);
}

void RsPluginManager::loadConfiguration()
{
	RsFileHash dummyHash;
	p3Config::loadConfiguration(dummyHash);
}

void RsPluginManager::setInterfaces(RsPlugInInterfaces &interfaces)
{
    std::cerr << "RsPluginManager::setInterfaces()  " << std::endl;

    for(uint32_t i=0;i<_plugins.size();++i)
        if(_plugins[i].plugin != NULL)
		  {
			  std::cerr << " setting iterface for plugin " << _plugins[i].plugin->getPluginName() << ", with RS_ID " << _plugins[i].plugin->rs_service_id() << std::endl ;
			  _plugins[i].plugin->setInterfaces(interfaces);
		  }
}

void RsPluginManager::setCacheDirectories(const std::string& local_cache, const std::string& remote_cache)
{
	_local_cache_dir = local_cache ;
	_remote_cache_dir = remote_cache ;
}

bool RsPluginManager::acceptablePluginName(const std::string& name)
{
	// Needs some windows specific code here
	//
#ifdef WINDOWS_SYS
	return name.size() > 4 && name.substr(name.size() - 4) == ".dll";
#elif defined(__MACH__)
	return name.size() > 6 && !strcmp(name.c_str()+name.size()-6,".dylib") ;
#else
	return name.size() > 3 && !strcmp(name.c_str()+name.size()-3,".so") ;
#endif
}

void RsPluginManager::disablePlugin(const RsFileHash& hash)
{
    std::set<RsFileHash>::iterator it = _accepted_hashes.find(hash) ;
	
	if(it != _accepted_hashes.end())
	{
		std::cerr << "RsPluginManager::disablePlugin(): removing hash " << hash << " from white list" << std::endl;

		_accepted_hashes.erase(it) ;
		IndicateConfigChanged() ;
	}
	if(_rejected_hashes.find(hash) == _rejected_hashes.end())
	{
		std::cerr << "RsPluginManager::enablePlugin(): inserting hash " << hash << " in black list" << std::endl;

		_rejected_hashes.insert(hash) ;
		IndicateConfigChanged() ;
	}
}

void RsPluginManager::enablePlugin(const RsFileHash& hash)
{
	if(_accepted_hashes.find(hash) == _accepted_hashes.end())
	{
		std::cerr << "RsPluginManager::enablePlugin(): inserting hash " << hash << " in white list" << std::endl;

		_accepted_hashes.insert(hash) ;
		IndicateConfigChanged() ;
	}
    std::set<RsFileHash>::const_iterator it(_rejected_hashes.find(hash)) ;
	
	if(it != _rejected_hashes.end())
	{
		std::cerr << "RsPluginManager::enablePlugin(): removing hash " << hash << " from black list" << std::endl;

		_rejected_hashes.erase(it) ;
		IndicateConfigChanged() ;
	}
}

void RsPluginManager::loadPlugins(const std::vector<std::string>& plugin_directories)
{
	_plugin_directories = plugin_directories ;
	_plugin_entry_symbol = "RETROSHARE_PLUGIN_provide" ;
	_plugin_revision_symbol = "RETROSHARE_PLUGIN_revision" ;

	// 0 - get the list of files to read

	bool first_time = (_accepted_hashes.empty()) && _rejected_hashes.empty() ;

	for(uint32_t i=0;i<plugin_directories.size();++i)
	{
		librs::util::FolderIterator dirIt(plugin_directories[i],true);

		if(!dirIt.isValid())
		{
			std::cerr << "Plugin directory : " << plugin_directories[i] << " does not exist." << std::endl ;
			continue ;
		}

        for(;dirIt.isValid();dirIt.next())
		{
            std::string fname = dirIt.file_name();

			char lc = plugin_directories[i][plugin_directories[i].length()-1] ; // length cannot be 0 here.

			std::string fullname = (lc == '/' || lc == '\\')? (plugin_directories[i] + fname) : (plugin_directories[i] + "/" + fname) ;

			if(!acceptablePluginName(fullname))
				continue ;

			std::cerr << "Found plugin " << fullname << std::endl;
			std::cerr << "  Loading plugin..." << std::endl;

			loadPlugin(fullname, first_time) ;
		}
		dirIt.closedir();
	}

	std::cerr << "Examined a total of " << _plugins.size() << " plugins." << std::endl;

	// Save list of accepted hashes and reference value
	
	// Calling IndicateConfigChanged() at this point is not sufficient because the config flags are cleared
	// at start of the p3config thread, and this thread has not yet started.
	//
	saveConfiguration();
}

void RsPluginManager::stopPlugins(p3ServiceServer *pqih)
{
	std::cerr << "  Stopping plugins." << std::endl;

	for (uint32_t i = 0; i < _plugins.size(); ++i)
	{
		if (_plugins[i].plugin != NULL)
		{
			p3Service *service = _plugins[i].plugin->p3_service();
			if (service)
			{
				pqih->removeService(service);
			}

			_plugins[i].plugin->stop();
			delete _plugins[i].plugin;
			_plugins[i].plugin = NULL;
		}
		if (_plugins[i].handle)
		{
			dlclose(_plugins[i].handle);
			_plugins[i].handle = NULL;
		}
	}
}

void RsPluginManager::getPluginStatus(int i,uint32_t& status,std::string& file_name,RsFileHash &hash,uint32_t& svn_revision,std::string& error_string) const
{
	if((uint32_t)i >= _plugins.size())
		return ;

	status = _plugins[i].status ;
	error_string = _plugins[i].info_string ;
	hash = _plugins[i].file_hash ;
	file_name = _plugins[i].file_name ;
	svn_revision = _plugins[i].svn_revision ;
}

bool RsPluginManager::getAllowAllPlugins() const
{
	return _allow_all_plugins ;
}
void RsPluginManager::allowAllPlugins(bool b)
{
	_allow_all_plugins = b ;
	IndicateConfigChanged() ;
}
RsSerialiser *RsPluginManager::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsPluginSerialiser()) ;
	rss->addSerialType(new RsGeneralConfigSerialiser()) ;

	return rss ;
}

void RsPluginManager::loadPlugins(const std::vector<RsPlugin *>& plugins)
{
	for(uint32_t i=0;i<plugins.size();++i)
		loadPlugin(plugins[i]) ;
}

bool RsPluginManager::loadPlugin(RsPlugin *p)
{
	std::cerr << "  Loading programmatically inserted plugin " << std::endl;

	PluginInfo pinfo ;
	pinfo.plugin = p ;
	pinfo.file_name = "No file" ;
	pinfo.file_hash.clear() ;
	pinfo.info_string = "" ;

	p->setPlugInHandler(this); // WIN fix, cannot share global space with shared libraries

	// The following choice is conservative by forcing RS to resolve all dependencies at
	// the time of loading the plugin. 

	pinfo.status = PLUGIN_STATUS_LOADED ;

	_plugins.push_back(pinfo) ;
	return true;
}

bool RsPluginManager::loadPlugin(const std::string& plugin_name,bool first_time)
{
	std::cerr << "  Loading plugin " << plugin_name << std::endl;

	PluginInfo pf ;
	pf.plugin = NULL ;
	pf.file_name = plugin_name ;
	pf.info_string = "" ;
	std::cerr << "    -> hashing." << std::endl;
	uint64_t size ;

	// Stage 1 - get information related to file (hash, name, ...)
	//
	if(!RsDirUtil::getFileHash(plugin_name,pf.file_hash,size))
	{
		std::cerr << "    -> cannot hash file. Plugin read canceled." << std::endl;
		return false;
	}

	_plugins.push_back(pf) ;
	PluginInfo& pinfo(_plugins.back()) ;

	std::cerr << "    -> hash = " << pinfo.file_hash << std::endl;

	if(!_allow_all_plugins)
	{
		if(_accepted_hashes.find(pinfo.file_hash) == _accepted_hashes.end() && _rejected_hashes.find(pinfo.file_hash) == _rejected_hashes.end() )
			if(!RsServer::notify()->askForPluginConfirmation(pinfo.file_name,pinfo.file_hash.toStdString(),first_time))
				_rejected_hashes.insert(pinfo.file_hash) ;		// accepted hashes are treated at the end, for security.

		if(_rejected_hashes.find(pinfo.file_hash) != _rejected_hashes.end() )
		{
			pinfo.status = PLUGIN_STATUS_REJECTED_HASH ;
			std::cerr << "    -> hash rejected. Giving up plugin. " << std::endl;
			return false ;
		}
	}
	else
		std::cerr << "   -> ALLOW_ALL_PLUGINS Enabled => plugin loaded by default." << std::endl;

	std::cerr << "    -> hash authorized. Loading plugin. " << std::endl;

	// Stage 2 - open with dlopen, and get some basic info.
	//

	// The following choice is conservative by forcing RS to resolve all dependencies at
	// the time of loading the plugin. 

	int link_mode = RTLD_NOW | RTLD_GLOBAL ; 

	void *handle = dlopen(plugin_name.c_str(),link_mode) ;

	if(handle == NULL)
	{
		const char *val = dlerror() ;
		std::cerr << "  Cannot open plugin: " << val << std::endl ;
		pinfo.status = PLUGIN_STATUS_DLOPEN_ERROR ;
		pinfo.info_string = val ;
		return false ;
	}

	void *prev = dlsym(handle,_plugin_revision_symbol.c_str()) ; pinfo.svn_revision = (prev == NULL) ? 0 : (*(uint32_t *)prev) ;
	void *papi = dlsym(handle,_plugin_API_symbol.c_str()) ; pinfo.API_version = (papi == NULL) ? 0 : (*(uint32_t *)papi) ;

	std::cerr << "    -> plugin revision number: " << pinfo.svn_revision << std::endl;
	std::cerr << "       plugin API number     : " << std::hex << pinfo.API_version << std::dec << std::endl;

	// Check that the plugin provides a svn revision number and a API number
	//
	if(pinfo.API_version == 0) 
	{
		std::cerr  << "    -> No API version number." << std::endl;
		pinfo.status = PLUGIN_STATUS_MISSING_API ;
		pinfo.info_string = "" ;
		dlclose(handle);
		return false ;
	} 
#ifdef TO_REMOVE
	if(pinfo.svn_revision == 0)
	{
		std::cerr  << "    -> No svn revision number." << std::endl;
		pinfo.status = PLUGIN_STATUS_MISSING_SVN ;
		pinfo.info_string = "" ;
		dlclose(handle);
		return false ;
	} 
#endif

	// Now look for the plugin class symbol.
	//
	void *pfe = dlsym(handle,_plugin_entry_symbol.c_str()) ;

	if(pfe == NULL) 
	{
		std::cerr << dlerror() << std::endl ;
		pinfo.status = PLUGIN_STATUS_MISSING_SYMBOL ;
		pinfo.info_string = _plugin_entry_symbol ;
		dlclose(handle);
		return false ;
	}
	std::cerr << "   -> Added function entry for symbol " << _plugin_entry_symbol << std::endl ;

	RsPlugin *p = ( (*(RetroSharePluginEntry)pfe)() ) ;

	if(p == NULL)
	{
		std::cerr << "  Plugin entry function " << _plugin_entry_symbol << " returns NULL ! It should return an object of type RsPlugin* " << std::endl;
		pinfo.status = PLUGIN_STATUS_NULL_PLUGIN ;
		pinfo.info_string = "Plugin entry " + _plugin_entry_symbol + "() return NULL" ;
		dlclose(handle);
		return false ;
	}

	pinfo.status = PLUGIN_STATUS_LOADED ;
	pinfo.plugin = p ;
	pinfo.handle = handle;
	p->setPlugInHandler(this); // WIN fix, cannot share global space with shared libraries
	pinfo.info_string = "" ;

	_accepted_hashes.insert(pinfo.file_hash) ;	// do it now, to avoid putting in list a plugin that might have crashed during the load.
	return true;
}

const std::string& RsPluginManager::getLocalCacheDir() const
{
	assert(!_local_cache_dir.empty()) ;
	return _local_cache_dir ;
}
const std::string& RsPluginManager::getRemoteCacheDir() const
{
	assert(!_remote_cache_dir.empty()) ;
	return _remote_cache_dir ;
}
RsServiceControl *RsPluginManager::getServiceControl() const
{
	assert(_service_control);
	return _service_control ;
}
void RsPluginManager::slowTickPlugins(rstime_t seconds)
{
	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_cache_service() != NULL && (seconds % _plugins[i].plugin->rs_cache_service()->tickDelay() ))
		{
#ifdef DEBUG_PLUGIN_MANAGER
			std::cerr << "  ticking plugin " << _plugins[i].plugin->getPluginName() << std::endl;
#endif
			_plugins[i].plugin->rs_cache_service()->tick() ;
		}
}

void RsPluginManager::registerCacheServices()
{
    // this is removed since the old cache syste is gone, but we need to make it register new GXS group services instead.
#ifdef REMOVED
	std::cerr << "  Registering cache services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_cache_service() != NULL)
		{
            //rsFiles->getCacheStrapper()->addCachePair(CachePair(_plugins[i].plugin->rs_cache_service(),_plugins[i].plugin->rs_cache_service(),CacheId(_plugins[i].plugin->rs_service_id(), 0))) ;
			std::cerr << "     adding new cache pair for plugin " << _plugins[i].plugin->getPluginName() << ", with RS_ID " << _plugins[i].plugin->rs_service_id() << std::endl ;
		}
#endif
}

void RsPluginManager::registerClientServices(p3ServiceServer *pqih)
{
	std::cerr << "  Registering pqi services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
        //if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_pqi_service() != NULL)
        if(_plugins[i].plugin != NULL && _plugins[i].plugin->p3_service() != NULL)
		{
            //pqih->addService(_plugins[i].plugin->rs_pqi_service(), true) ;
            pqih->addService(_plugins[i].plugin->p3_service(), true) ;
			std::cerr << "    Added pqi service for plugin " << _plugins[i].plugin->getPluginName() << std::endl;
		}
}

void RsPluginManager::addConfigurations(p3ConfigMgr *ConfigMgr)
{
	std::cerr << "  Registering configuration files." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->configurationFileName().length() > 0)
		{
			if( _plugins[i].plugin->rs_cache_service() != NULL)
				ConfigMgr->addConfiguration(_plugins[i].plugin->configurationFileName(), _plugins[i].plugin->rs_cache_service());
            //else if(_plugins[i].plugin->rs_pqi_service() != NULL)
            //	ConfigMgr->addConfiguration(_plugins[i].plugin->configurationFileName(), _plugins[i].plugin->rs_pqi_service());
            else if(_plugins[i].plugin->p3_config() != NULL)
                ConfigMgr->addConfiguration(_plugins[i].plugin->configurationFileName(), _plugins[i].plugin->p3_config());
			else
				continue ;

			std::cerr << "    Added configuration for plugin " << _plugins[i].plugin->getPluginName() << ", with file " << _plugins[i].plugin->configurationFileName() << std::endl;
		}
}		

bool RsPluginManager::loadList(std::list<RsItem*>& list)
{
    std::set<RsFileHash> accepted_hash_candidates ;
    std::set<RsFileHash> rejected_hash_candidates ;

	std::cerr << "RsPluginManager::loadList(): " << std::endl;
    RsFileHash reference_executable_hash ;

	std::list<RsItem *>::iterator it;
	for(it = list.begin(); it != list.end(); ++it)
	{
		RsConfigKeyValueSet *witem = dynamic_cast<RsConfigKeyValueSet *>(*it) ;

		if(witem)
			for(std::list<RsTlvKeyValue>::const_iterator kit = witem->tlvkvs.pairs.begin(); kit != witem->tlvkvs.pairs.end(); ++kit) 
			{
				if((*kit).key == "ALLOW_ALL_PLUGINS")
				{
					_allow_all_plugins = (kit->value == "YES");

					if(_allow_all_plugins)
						std::cerr << "WARNING: Allowing all plugins. No hash will be checked. Be careful! " << std::endl ;
				}
				else if((*kit).key == "REFERENCE_EXECUTABLE_HASH")
				{
					reference_executable_hash = RsFileHash(kit->value) ;
					std::cerr << "   Reference executable hash: " << kit->value << std::endl;
				}
				else if((*kit).key == "ACCEPTED")
				{
                    accepted_hash_candidates.insert(RsFileHash((*kit).value)) ;
					std::cerr << "   Accepted hash: " << (*kit).value << std::endl;
				}
				else if((*kit).key == "REJECTED")
				{
                    rejected_hash_candidates.insert(RsFileHash((*kit).value)) ;
					std::cerr << "   Rejected hash: " << (*kit).value << std::endl;
				}
			}

		delete (*it);
	}

	// Rejected hashes are always kept, so that RS wont ask again if the executable hash has changed.
	//
	_rejected_hashes = rejected_hash_candidates ;

	if(reference_executable_hash == _current_executable_hash)
	{
		std::cerr << "(II) Executable hash matches. Updating the list of accepted/rejected plugins." << std::endl;

		_accepted_hashes = accepted_hash_candidates ;
	}
	else
		std::cerr << "(WW) Executable hashes do not match. Executable hash has changed. Discarding the list of accepted/rejected plugins." << std::endl;

	return true;
}

bool RsPluginManager::saveList(bool& cleanup, std::list<RsItem*>& list)
{
	std::cerr << "PluginManager: saving list." << std::endl;
	cleanup = true ;

	RsConfigKeyValueSet *witem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "ALLOW_ALL_PLUGINS" ;
	kv.value = _allow_all_plugins?"YES":"NO" ;
	witem->tlvkvs.pairs.push_back(kv) ;

	kv.key = "REFERENCE_EXECUTABLE_HASH" ;
    kv.value = _current_executable_hash.toStdString() ;
	witem->tlvkvs.pairs.push_back(kv) ;

	std::cerr << "  Saving current executable hash: " << kv.value << std::endl;

	// now push accepted and rejected hashes.
	
    for(std::set<RsFileHash>::const_iterator it(_accepted_hashes.begin());it!=_accepted_hashes.end();++it)
	{
        witem->tlvkvs.pairs.push_back( RsTlvKeyValue( "ACCEPTED", (*it).toStdString() ) ) ;
		std::cerr << "  " << *it << " : " << "ACCEPTED" << std::endl;
	}

    for(std::set<RsFileHash>::const_iterator it(_rejected_hashes.begin());it!=_rejected_hashes.end();++it)
	{
        witem->tlvkvs.pairs.push_back( RsTlvKeyValue( "REJECTED", (*it).toStdString() ) ) ;
		std::cerr << "  " << *it << " : " << "REJECTED" << std::endl;
	}

	list.push_back(witem) ;

	return true;
}

RsPQIService::RsPQIService(uint16_t /*service_type*/, uint32_t /*tick_delay_in_seconds*/, RsPluginHandler* /*pgHandler*/)
	: p3Service(),p3Config()
{
}

