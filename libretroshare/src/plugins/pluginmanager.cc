#include <string.h>

#include "pluginmanager.h"
#include <dirent.h>
#include <util/folderiterator.h>
#include <ft/ftserver.h>
#include <dbase/cachestrapper.h>
#include <retroshare/rsplugin.h>
#include <retroshare/rsfiles.h>
#include <pqi/pqiservice.h>
#include <plugins/pluginclasses.h>

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
p3ConnectMgr   *RsPluginManager::_connectmgr 				= NULL ;

typedef RsPlugin *(*RetroSharePluginEntry)(void) ;
RsPluginHandler *rsPlugins ;

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
	return name.size() > 4 && !strcmp(name.c_str()+name.size()-3,".dll") ;
#else
	return name.size() > 3 && !strcmp(name.c_str()+name.size()-3,".so") ;
#endif
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

bool RsPluginManager::loadPlugin(const std::string& plugin_name)
{
	std::cerr << "  Loading plugin " << plugin_name << std::endl;

	// The following choice is somewhat dangerous since the program can stop when a symbol can
	// not be resolved. However, this is the only way to bind a single .so for both the
	// interface and command line executables.

	int link_mode = RTLD_NOW | RTLD_GLOBAL ; // RTLD_NOW
	//int link_mode = RTLD_GLOBAL ;

	// Warning: this temporary vector is necessary, because linking with a .so that would include BundleManager.h
	// is going to call the initialization of the static members of bundleManager a second time, and therefore
	// will erase whatever is already initialized. So I first open all libraries, then fill the vectors.
	//
	void *handle = dlopen(plugin_name.c_str(),link_mode) ;

	if(handle == NULL)
	{
		std::cerr << "  Cannot open plugin: " << dlerror() << std::endl ;
		return false ;
	}
	
	void *pf = dlsym(handle,_plugin_entry_symbol.c_str()) ;

	if(pf == NULL)
		std::cerr << dlerror() << std::endl ;
	else
		std::cerr << "  Added function entry for symbol " << _plugin_entry_symbol << std::endl ;

	RsPlugin *p = ( (*(RetroSharePluginEntry)pf)() ) ;

	if(p == NULL)
	{
		std::cerr << "  Plugin entry function " << _plugin_entry_symbol << " returns NULL ! It should return an object of type RsPlugin* " << std::endl;
		return false ;
	}
	_plugins.push_back(p) ;
	
	if(link_mode & RTLD_LAZY)
	{
		std::cerr << "  Symbols have been linked in LAZY mode. This means that undefined symbols may" << std::endl ;
		std::cerr << "  crash your program any time." << std::endl ;
	}

	return true ;
}

p3ConnectMgr *RsPluginManager::getConnectMgr() const
{
	assert(_connectmgr != NULL) ;
	return _connectmgr ;
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
		if(_plugins[i]->rs_cache_service() != NULL && (seconds % _plugins[i]->rs_cache_service()->tickDelay() ))
		{
			std::cerr << "  ticking plugin " << _plugins[i]->getPluginName() << std::endl;
			_plugins[i]->rs_cache_service()->tick() ;
		}
}

void RsPluginManager::registerCacheServices()
{
	std::cerr << "  Registering cache services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i]->rs_cache_service() != NULL)
		{
			rsFiles->getCacheStrapper()->addCachePair(CachePair(_plugins[i]->rs_cache_service(),_plugins[i]->rs_cache_service(),CacheId(_plugins[i]->rs_service_id(), 0))) ;
			std::cerr << "     adding new cache pair for plugin " << _plugins[i]->getPluginName() << ", with RS_ID " << _plugins[i]->rs_service_id() << std::endl ;
		}
}

void RsPluginManager::registerClientServices(p3ServiceServer *pqih)
{
	std::cerr << "  Registering pqi services." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i]->rs_pqi_service() != NULL)
		{
			pqih->addService(_plugins[i]->rs_pqi_service()) ;
			std::cerr << "    Added pqi service for plugin " << _plugins[i]->getPluginName() << std::endl;
		}
}

void RsPluginManager::addConfigurations(p3ConfigMgr *ConfigMgr)
{
	std::cerr << "  Registering configuration files." << std::endl;

	for(uint32_t i=0;i<_plugins.size();++i)
		if(_plugins[i]->configurationFileName().length() > 0)
		{
			ConfigMgr->addConfiguration(_plugins[i]->configurationFileName(), _plugins[i]->rs_cache_service());
			std::cerr << "    Added configuration for plugin " << _plugins[i]->getPluginName() << ", with file " << _plugins[i]->configurationFileName() << std::endl;
		}
}		

RsCacheService::RsCacheService(uint16_t service_type,uint32_t config_type,uint32_t tick_delay)
	: CacheSource(service_type, true, rsPlugins->getFileServer()->getCacheStrapper(), rsPlugins->getLocalCacheDir()), 
	  CacheStore (service_type, true, rsPlugins->getFileServer()->getCacheStrapper(), rsPlugins->getFileServer()->getCacheTransfer(), rsPlugins->getRemoteCacheDir()), 
	  p3Config(config_type), // CONFIG_TYPE_RANK_LINK
	  _tick_delay_in_seconds(tick_delay)
{
}

