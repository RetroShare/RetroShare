/*******************************************************************************
 * libretroshare/src/plugins: pluginmanager.h                                  *
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
	PluginInfo() : handle(NULL), plugin(NULL), API_version(0), svn_revision(0), status(0) {}

	public:
		// Handle of the loaded plugin.
		//
		void *handle;

		// Main object provided by the plugin. NULL is the plugin could not be loaded.
		//
		RsPlugin *plugin ;

		// Information related to the file. Do not require the plugin to be loaded nor the DSO to be openned.
		//
        RsFileHash file_hash ;
		std::string file_name ;

		// Information coming from directly loaded symbols. The plugin is responsible for providing them.
		//
		std::string creator ;		// creator of the plugin
		std::string name ;			// name of the plugin
		uint32_t API_version ;		// API version. 
		uint32_t svn_revision ;		// Coming from scripts. Same svn version but changing hash could be a security issue.

		// This info is filled when accessing the .so, and loading the plugin.
		//
		uint32_t status ;					// See the flags in retroshare/rsplugin.h
		std::string info_string ;
};

class RsPluginManager: public RsPluginHandler, public p3Config
{
	public:
		explicit RsPluginManager(const RsFileHash& current_executable_sha1_hash) ;
		virtual ~RsPluginManager() {}
		
		// ------------ Derived from RsPluginHandler ----------------//
		//
		virtual int nbPlugins() const { return _plugins.size() ; }
		virtual RsPlugin *plugin(int i) { return _plugins[i].plugin ; }
		virtual const std::vector<std::string>& getPluginDirectories() const { return _plugin_directories ; }
        virtual void getPluginStatus(int i, uint32_t& status,std::string& file_name, RsFileHash& hash,uint32_t& svn_revision,std::string& error_string) const ;
        virtual void enablePlugin(const RsFileHash& hash) ;
        virtual void disablePlugin(const RsFileHash &hash) ;

		virtual void slowTickPlugins(rstime_t sec) ;
		virtual const std::string& getLocalCacheDir() const ;
		virtual const std::string& getRemoteCacheDir() const ;
		virtual RsServiceControl *getServiceControl() const ;

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
		virtual bool loadConfiguration(RsFileHash &loadHash) ;
		virtual void loadConfiguration() ;

                /*!
                 * sets interfaces for all loaded plugins
                 * @param interfaces
                 */
		void setInterfaces(RsPlugInInterfaces& interfaces);

		static bool acceptablePluginName(const std::string& s) ;
		static void setCacheDirectories(const std::string& local,const std::string& remote) ;
		static void setServiceControl(RsServiceControl *cm) { _service_control = cm ; }

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

		void stopPlugins(p3ServiceServer *pqih);

		void registerCacheServices() ;
		void registerClientServices(p3ServiceServer *pqih) ;

	private:
		bool loadPlugin(RsPlugin *) ;
		bool loadPlugin(const std::string& shared_library_name, bool first_time) ;
        RsFileHash hashPlugin(const std::string& shared_library_name) ;

		std::vector<PluginInfo> _plugins ;

		// Should allow
		// 	- searching 
		// 	- saving all hash
		//
		// At start
		// 	* load reference executable hash. Compare with current executable. 
		// 		- if different => flush all plugin hashes from cache
		// 		- if equal, 
		//
        std::set<RsFileHash> _accepted_hashes ; // accepted hash values for reference executable hash.
        std::set<RsFileHash> _rejected_hashes ; // rejected hash values for reference executable hash.
        RsFileHash _current_executable_hash ;		// At all times, the list of accepted plugins should be related to the current hash of the executable.
		bool _allow_all_plugins ;

		static std::string _plugin_entry_symbol ;
		static std::string _plugin_revision_symbol ;
		static std::string _plugin_API_symbol ;
		static std::string _remote_cache_dir ;
		static std::string _local_cache_dir ;

		static RsServiceControl *_service_control ;

		static std::vector<std::string> _plugin_directories ;
};

