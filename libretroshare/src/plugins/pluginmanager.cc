#include <string.h>

#include "pluginmanager.h"
#include <dirent.h>
#include <serialiser/rsserial.h>
#include <serialiser/rstlvbase.h>
#include <serialiser/rstlvtypes.h>
#include <serialiser/rspluginitems.h>
#include <util/rsdir.h>
#include <util/folderiterator.h>
#include <ft/ftserver.h>
#include <dbase/cachestrapper.h>
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

std::string RsPluginManager::_plugin_entry_symbol ;
std::string RsPluginManager::_local_cache_dir ;
std::string RsPluginManager::_remote_cache_dir ;
std::vector<std::string> RsPluginManager::_plugin_directories ;

ftServer   		*RsPluginManager::_ftserver 					= NULL ;
p3LinkMgr   *RsPluginManager::_linkmgr							= NULL ;

typedef RsPlugin *(*RetroSharePluginEntry)(void) ;
RsPluginHandler *rsPlugins ;

RsPluginManager::RsPluginManager() : p3Config(CONFIG_TYPE_PLUGINS)
{
	_allow_all_plugins = false ;
}

void RsPluginManager::loadConfiguration()
{
	std::string dummyHash = "dummyHash";
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
#else
	return name.size() > 3 && !strcmp(name.c_str()+name.size()-3,".so") ;
#endif
}

void RsPluginManager::disablePlugin(const std::string& hash)
{
	std::set<std::string>::iterator it = _accepted_hashes.find(hash) ;
	
	if(it != _accepted_hashes.end())
	{
		std::cerr << "RsPluginManager::disablePlugin(): removing hash " << hash << " from white list" << std::endl;

		_accepted_hashes.erase(it) ;
		IndicateConfigChanged() ;
	}
}

void RsPluginManager::enablePlugin(const std::string& hash)
{
	if(_accepted_hashes.find(hash) == _accepted_hashes.end())
	{
		std::cerr << "RsPluginManager::enablePlugin(): inserting hash " << hash << " in white list" << std::endl;

		_accepted_hashes.insert(hash) ;
		IndicateConfigChanged() ;
	}
}

void RsPluginManager::loadPlugins(const std::vector<std::string>& plugin_directories)
{
	_plugin_directories = plugin_directories ;
	_plugin_entry_symbol = "RETROSHARE_PLUGIN_provide" ;

	// 0 - get the list of files to read

	for(uint32_t i=0;i<plugin_directories.size();++i)
	{
		librs::util::FolderIterator dirIt(plugin_directories[i]);
		if(!dirIt.isValid())
		{
			std::cerr << "Plugin directory : " << plugin_directories[i] << " does not exist." << std::endl ;
			continue ;
		}

		while(dirIt.readdir())
		{
			std::string fname;
			dirIt.d_name(fname);
			std::string fullname = plugin_directories[i] + "/" + fname;

			if(!acceptablePluginName(fullname))
				continue ;

			std::cerr << "Found plugin " << fullname << std::endl;
			std::cerr << "  Loading plugin..." << std::endl;

			loadPlugin(fullname) ;
		}
		dirIt.closedir();
	}

	std::cerr << "Loaded a total of " << _plugins.size() << " plugins." << std::endl;
}

void RsPluginManager::stopPlugins()
{
	std::cerr << "  Stopping plugins." << std::endl;

	for (uint32_t i = 0; i < _plugins.size(); ++i)
	{
		if (_plugins[i].plugin != NULL)
		{
			_plugins[i].plugin->stop();
//			delete _plugins[i].plugin;
//			_plugins[i].plugin = NULL;
		}
	}
}

void RsPluginManager::getPluginStatus(int i,uint32_t& status,std::string& file_name,std::string& hash,std::string& error_string) const
{
	if((uint32_t)i >= _plugins.size())
		return ;

	status = _plugins[i].status ;
	error_string = _plugins[i].info_string ;
	hash = _plugins[i].file_hash ;
	file_name = _plugins[i].file_name ;
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
	pinfo.file_hash = "No hash" ;
	pinfo.info_string = "" ;

	p->setPlugInHandler(this); // WIN fix, cannot share global space with shared libraries

	// The following choice is conservative by forcing RS to resolve all dependencies at
	// the time of loading the plugin. 

	pinfo.status = PLUGIN_STATUS_LOADED ;

	_plugins.push_back(pinfo) ;
	return true;
}

bool RsPluginManager::loadPlugin(const std::string& plugin_name)
{
	std::cerr << "  Loading plugin " << plugin_name << std::endl;

	PluginInfo pf ;
	pf.plugin = NULL ;
	pf.file_name = plugin_name ;
	pf.info_string = "" ;
	std::cerr << "    -> hashing." << std::endl;
	uint64_t size ;

	if(!RsDirUtil::getFileHash(plugin_name,pf.file_hash,size))
	{
		std::cerr << "    -> cannot hash file. Plugin read canceled." << std::endl;
		return false;
	}

	// This file can be loaded. Insert an entry into the list of detected plugins.
	//
	_plugins.push_back(pf) ;
	PluginInfo& pinfo(_plugins.back()) ;

	std::cerr << "    -> hash = " << pinfo.file_hash << std::endl;

	if((!_allow_all_plugins) && _accepted_hashes.find(pinfo.file_hash) == _accepted_hashes.end())
	{
		std::cerr  << "    -> hash is not in white list. Plugin is rejected. Go to config->plugins to authorise this plugin." << std::endl;
		pinfo.status = PLUGIN_STATUS_UNKNOWN_HASH ;
		pinfo.info_string = "" ;
		return false ;
	}
	else
	{
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

		void *pf = dlsym(handle,_plugin_entry_symbol.c_str()) ;

		if(pf == NULL) 
		{
			std::cerr << dlerror() << std::endl ;
			pinfo.status = PLUGIN_STATUS_MISSING_SYMBOL ;
			pinfo.info_string = "Symbol " + _plugin_entry_symbol + " is missing." ;
			return false ;
		}
		std::cerr << "  Added function entry for symbol " << _plugin_entry_symbol << std::endl ;

		RsPlugin *p = ( (*(RetroSharePluginEntry)pf)() ) ;

		if(p == NULL)
		{
			std::cerr << "  Plugin entry function " << _plugin_entry_symbol << " returns NULL ! It should return an object of type RsPlugin* " << std::endl;
			pinfo.status = PLUGIN_STATUS_NULL_PLUGIN ;
			pinfo.info_string = "Plugin entry " + _plugin_entry_symbol + "() return NULL" ;
			return false ;
		}

		pinfo.status = PLUGIN_STATUS_LOADED ;
		pinfo.plugin = p ;
		p->setPlugInHandler(this); // WIN fix, cannot share global space with shared libraries
		pinfo.info_string = "" ;

		return true;
	}
}

p3LinkMgr *RsPluginManager::getLinkMgr() const
{
	assert(_linkmgr != NULL) ;
	return _linkmgr ;
}


ftServer *RsPluginManager::getFileServer() const
{
	assert(_ftserver != NULL) ;
	return _ftserver ;
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
void RsPluginManager::slowTickPlugins(time_t seconds)
{
	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_cache_service() != NULL && (seconds % _plugins[i].plugin->rs_cache_service()->tickDelay() ))
		{
			std::cerr << "  ticking plugin " << _plugins[i].plugin->getPluginName() << std::endl;
			_plugins[i].plugin->rs_cache_service()->tick() ;
		}
}

void RsPluginManager::registerCacheServices()
{
	std::cerr << "  Registering cache services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_cache_service() != NULL)
		{
			rsFiles->getCacheStrapper()->addCachePair(CachePair(_plugins[i].plugin->rs_cache_service(),_plugins[i].plugin->rs_cache_service(),CacheId(_plugins[i].plugin->rs_service_id(), 0))) ;
			std::cerr << "     adding new cache pair for plugin " << _plugins[i].plugin->getPluginName() << ", with RS_ID " << _plugins[i].plugin->rs_service_id() << std::endl ;
		}
}

void RsPluginManager::registerClientServices(p3ServiceServer *pqih)
{
	std::cerr << "  Registering pqi services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i].plugin != NULL && _plugins[i].plugin->rs_pqi_service() != NULL)
		{
			pqih->addService(_plugins[i].plugin->rs_pqi_service()) ;
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
			else if(_plugins[i].plugin->rs_pqi_service() != NULL)
				ConfigMgr->addConfiguration(_plugins[i].plugin->configurationFileName(), _plugins[i].plugin->rs_pqi_service());
			else
				continue ;

			std::cerr << "    Added configuration for plugin " << _plugins[i].plugin->getPluginName() << ", with file " << _plugins[i].plugin->configurationFileName() << std::endl;
		}
}		

bool RsPluginManager::loadList(std::list<RsItem*>& list)
{
	_accepted_hashes.clear() ;

	std::cerr << "RsPluginManager::loadList(): " << std::endl;

	std::list<RsItem *>::iterator it;
	for(it = list.begin(); it != list.end(); it++) 
	{
		RsPluginHashSetItem *vitem = dynamic_cast<RsPluginHashSetItem*>(*it);

		if(vitem) 
			for(std::list<std::string>::const_iterator it(vitem->hashes.ids.begin());it!=vitem->hashes.ids.end();++it)
			{
				_accepted_hashes.insert(*it) ;
				std::cerr << "  loaded hash " << *it << std::endl;
			}

		RsConfigKeyValueSet *witem = dynamic_cast<RsConfigKeyValueSet *>(*it) ;

		if(witem)
		{
			for(std::list<RsTlvKeyValue>::const_iterator kit = witem->tlvkvs.pairs.begin(); kit != witem->tlvkvs.pairs.end(); ++kit) 
				if((*kit).key == "ALLOW_ALL_PLUGINS")
				{
					std::cerr << "WARNING: Allowing all plugins. No hash will be checked. Be careful! " << std::endl ;
					_allow_all_plugins = (kit->value == "YES");
				}
		}

		delete (*it);
	}
	return true;
}

bool RsPluginManager::saveList(bool& cleanup, std::list<RsItem*>& list)
{
	cleanup = true ;

	RsPluginHashSetItem *vitem = new RsPluginHashSetItem() ;

	for(std::set<std::string>::const_iterator it(_accepted_hashes.begin());it!=_accepted_hashes.end();++it)
		vitem->hashes.ids.push_back(*it) ;

	list.push_back(vitem) ;

	RsConfigKeyValueSet *witem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "ALLOW_ALL_PLUGINS" ;
	kv.value = _allow_all_plugins?"YES":"NO" ;
	witem->tlvkvs.pairs.push_back(kv) ;

	list.push_back(witem) ;

	return true;
}

RsCacheService::RsCacheService(uint16_t service_type,uint32_t config_type,uint32_t tick_delay, RsPluginHandler* pgHandler)
        : CacheSource(service_type, true, pgHandler->getFileServer()->getCacheStrapper(), pgHandler->getLocalCacheDir()),
          CacheStore (service_type, true, pgHandler->getFileServer()->getCacheStrapper(), pgHandler->getFileServer()->getCacheTransfer(), pgHandler->getRemoteCacheDir()),
	  p3Config(config_type), // CONFIG_TYPE_RANK_LINK
	  _tick_delay_in_seconds(tick_delay)
{
}

RsPQIService::RsPQIService(uint16_t service_type,uint32_t config_type,uint32_t /*tick_delay_in_seconds*/, RsPluginHandler* /*pgHandler*/)
	: p3Service(service_type),p3Config(config_type)
{
}

