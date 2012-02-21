/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Cyril Soler
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

#pragma once

#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "retroshare/rspeers.h"
#include "retroshare/rsfiles.h"

class RsPluginHandler ;
extern RsPluginHandler *rsPlugins ;

class p3Service ;
class p3LinkMgr ;
class MainPage ;
class QIcon ;
class QString ;
class QWidget ;
class QTranslator;
class QApplication;
class RsCacheService ;
class ftServer ;
class ConfigPage ;
class RsPQIService ;
class RsAutoUpdatePage ;

// Used for the status of plugins.
//
#define PLUGIN_STATUS_NO_STATUS      0x0000
#define PLUGIN_STATUS_UNKNOWN_HASH   0x0001
#define PLUGIN_STATUS_DLOPEN_ERROR   0x0002
#define PLUGIN_STATUS_MISSING_SYMBOL 0x0003
#define PLUGIN_STATUS_NULL_PLUGIN    0x0004
#define PLUGIN_STATUS_LOADED         0x0005

class RsPluginHandler;

/*!
 *
 * convenience class to store gui interfaces
 *
 */
class RsPlugInInterfaces {

public:

    RsPlugInInterfaces() { mPeers = NULL; mFiles = NULL; }
    RsPeers* mPeers;
    RsFiles* mFiles;

};

class RsPlugin
{
	public:
		//
		//================================ Services ==================================//
		//
		// Cache service. Use this for providing cache-based services, such as channels, forums.
		// Example plugin: LinksCloud 
		//
		virtual RsCacheService *rs_cache_service() 	const	{ return NULL ; }	

		// Peer-to-Peer service. Use this for providing a service based to friend to friend
		// exchange of data, such as chat, messages, etc. 
		// Example plugin: VOIP
		//
		virtual RsPQIService   *rs_pqi_service() 		const	{ return NULL ; }
		virtual uint16_t        rs_service_id() 	   const	{ return 0    ; }

		// Filename used for saving the specific plugin configuration. Both RsCacheService and RsPQIService
		// derive from p3Config, which means that the service provided by the plugin can load/save its own
		// config by deriving loadList() and saveList() from p3Config.
		//
		virtual std::string configurationFileName() const { return std::string() ; }

		//
		//=================================== GUI ====================================//
		//
		// Derive the following methods to provide GUI additions to RetroShare's GUI.
		//
		// Main page: like Transfers, Channels, Forums, etc.
		//
		virtual MainPage       		*qt_page()       		const	{ return NULL ; }	// The page itself
		virtual QIcon          		*qt_icon()       		const	{ return NULL ; } // the page icon. Todo: put icon as virtual in MainPage

		virtual QWidget        		*qt_config_panel()	const	{ return NULL ; } // Config panel, to appear config->plugins->[]->
		virtual ConfigPage     		*qt_config_page()  	const	{ return NULL ; } // Config tab to add in config panel.
		virtual RsAutoUpdatePage 	*qt_transfers_tab()	const	{ return NULL ; } // Tab to add in transfers, after turtle statistics.
		virtual std::string   		 qt_transfers_tab_name()const	{ return "Tab" ; } // Tab name

		virtual QTranslator    *qt_translator(QApplication * /* app */, const QString& /* languageCode */ ) const	{ return NULL ; }

		// 
		//========================== Plugin Description ==============================//
		// 
		//  All these items appear in the config->plugins tab, as a description of the plugin.
		//
		virtual std::string getShortPluginDescription() const = 0 ;
		virtual std::string getPluginName() const = 0 ;
		virtual void getPluginVersion(int& major,int& minor,int& svn_rev) const = 0 ;

		// 
		//========================== Plugin Interface ================================//
		// 
		// Use these methods to access main objects from RetroShare.
		//
		virtual void setInterfaces(RsPlugInInterfaces& interfaces) = 0;
		virtual void setPlugInHandler(RsPluginHandler* pgHandler) = 0;
};

class RsPluginHandler
{
	public:
		// Returns the number of loaded plugins.
		//
		virtual int nbPlugins() const = 0 ;
		virtual RsPlugin *plugin(int i) = 0 ;
		virtual const std::vector<std::string>& getPluginDirectories() const = 0;
		virtual void getPluginStatus(int i,uint32_t& status,std::string& file_name,std::string& file_hash,std::string& error_string) const = 0 ;
		virtual void enablePlugin(const std::string& hash) = 0;
		virtual void disablePlugin(const std::string& hash) = 0;

		virtual void allowAllPlugins(bool b) = 0 ;
		virtual bool getAllowAllPlugins() const = 0 ;

		virtual void slowTickPlugins(time_t sec) = 0 ;

		virtual const std::string& getLocalCacheDir() const =0;
		virtual const std::string& getRemoteCacheDir() const =0;
		virtual ftServer *getFileServer() const = 0;
		virtual p3LinkMgr *getLinkMgr() const = 0;
};



