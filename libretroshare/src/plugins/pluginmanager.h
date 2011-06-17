#pragma once

#include <string>
#include <vector>
#include <retroshare/rsplugin.h>

class p3ConfigMgr ;
class p3ServiceServer ;
class p3ConnectMgr ;

class RsPluginManager: public RsPluginHandler
{
	public:
		RsPluginManager() {}
		virtual ~RsPluginManager() {}
		
		virtual int nbPlugins() const { return _plugins.size() ; }
		virtual RsPlugin *plugin(int i) { return _plugins[i] ; }
		virtual const std::vector<std::string>& getPluginDirectories() const { return _plugin_directories ; }


		virtual void slowTickPlugins(time_t sec) ;
		virtual void addConfigurations(p3ConfigMgr *cfgMgr) ;
		virtual const std::string& getLocalCacheDir() const ;
		virtual const std::string& getRemoteCacheDir() const ;
		virtual ftServer *getFileServer() const ;
		virtual p3ConnectMgr *getConnectMgr() const ;

		static void setPluginEntrySymbol(const std::string& s) { _plugin_entry_symbol = s ; }
		static bool acceptablePluginName(const std::string& s) ;
		static void setCacheDirectories(const std::string& local,const std::string& remote) ;
		static void setFileServer(ftServer *ft) { _ftserver = ft ; }
		static void setConnectMgr(p3ConnectMgr *cm) { _connectmgr = cm ; }

		void loadPlugins(const std::vector<std::string>& plugin_directories) ;

		void registerCacheServices() ;
		void registerClientServices(p3ServiceServer *pqih) ;
	private:
		bool loadPlugin(const std::string& shared_library_name) ;

		std::vector<RsPlugin *> _plugins ;

		static std::string _plugin_entry_symbol ;
		static std::string _remote_cache_dir ;
		static std::string _local_cache_dir ;
		static ftServer *_ftserver ;
		static p3ConnectMgr *_connectmgr ;

		static std::vector<std::string> _plugin_directories ;
};

