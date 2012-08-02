#pragma once

#include <string>
#include <set>
#include <vector>
#include <retroshare/rsplugin.h>
#include <pqi/p3cfgmgr.h>

class p3ConfigMgr ;
class p3ServiceServer ;
class p3LinkMgr ;

class PluginInfo
{
	public:
		RsPlugin *plugin ;
		std::string info_string ;
		std::string file_hash ;
		std::string file_name ;
		uint32_t status ;
};

class RsPluginManager: public RsPluginHandler, public p3Config
{
	public:
		RsPluginManager() ;
		virtual ~RsPluginManager() {}
		
		// ------------ Derived from RsPluginHandler ----------------//
		//
		virtual int nbPlugins() const { return _plugins.size() ; }
		virtual RsPlugin *plugin(int i) { return _plugins[i].plugin ; }
		virtual const std::vector<std::string>& getPluginDirectories() const { return _plugin_directories ; }
		virtual void getPluginStatus(int i, uint32_t& status,std::string& file_name, std::string& hash,std::string& error_string) const ;
		virtual void enablePlugin(const std::string& hash) ;
		virtual void disablePlugin(const std::string& hash) ;

		virtual void slowTickPlugins(time_t sec) ;
		virtual const std::string& getLocalCacheDir() const ;
		virtual const std::string& getRemoteCacheDir() const ;
		virtual ftServer *getFileServer() const ;
		virtual p3LinkMgr *getLinkMgr() const ;

		virtual void allowAllPlugins(bool b) ;
		virtual bool getAllowAllPlugins() const ;

		// ---------------- Derived from p3Config -------------------//
		//
		bool saveList(bool& cleanup, std::list<RsItem*>& list) ;
		bool loadList(std::list<RsItem*>& list) ;
		virtual RsSerialiser* setupSerialiser() ;

		// -------------------- Own members -------------------------//
		//
		virtual void addConfigurations(p3ConfigMgr *cfgMgr) ;
		virtual void loadConfiguration() ;

                /*!
                 * sets interfaces for all loaded plugins
                 * @param interfaces
                 */
                void setInterfaces(RsPlugInInterfaces& interfaces);
		static void setPluginEntrySymbol(const std::string& s) { _plugin_entry_symbol = s ; }
		static bool acceptablePluginName(const std::string& s) ;
		static void setCacheDirectories(const std::string& local,const std::string& remote) ;
		static void setFileServer(ftServer *ft) { _ftserver = ft ; }
		static void setLinkMgr(p3LinkMgr *cm) { _linkmgr = cm ; }

		// Normal plugin loading system. Parses through the plugin directories and loads
		// dso libraries, checks for the hash, and loads/rejects plugins according to
		// the user's choice.
		//
		void loadPlugins(const std::vector<std::string>& plugin_directories) ;

		// Explicit function to load a plugin programatically. 
		// No hash-checking is performed (there's no DSO file to hash!)
		// Mostly convenient for insering plugins in development.
		//
		void loadPlugins(const std::vector<RsPlugin*>& explicit_plugin_entries) ;

		void stopPlugins();

		void registerCacheServices() ;
		void registerClientServices(p3ServiceServer *pqih) ;

	private:
		bool loadPlugin(RsPlugin *) ;
		bool loadPlugin(const std::string& shared_library_name) ;
		std::string hashPlugin(const std::string& shared_library_name) ;

		std::vector<PluginInfo> _plugins ;
		std::set<std::string> _accepted_hashes ;
		bool _allow_all_plugins ;

		static std::string _plugin_entry_symbol ;
		static std::string _remote_cache_dir ;
		static std::string _local_cache_dir ;
		static ftServer *_ftserver ;
		static p3LinkMgr *_linkmgr ;

		static std::vector<std::string> _plugin_directories ;
};

