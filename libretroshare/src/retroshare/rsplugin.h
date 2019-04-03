/*******************************************************************************
 * libretroshare/src/retroshare: rsplugin.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Cyril Soler <csoler@users.sourceforge.net>           *
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

#include "util/rstime.h"
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "retroshare/rspeers.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rsversion.h"
#include "util/rsinitedptr.h"

class RsPluginHandler ;
extern RsPluginHandler *rsPlugins ;

class p3Service ;
class RsServiceControl ;
class RsReputations ;
class RsTurtle ;
class RsGxsTunnelService ;
class RsDht ;
class RsDisc ;
class RsMsgs ;
class RsGxsForums;
class RsGxsChannels;
class RsNotify;
class RsServiceControl;
class p3LinkMgr ;
class MainPage ;
class QIcon ;
class QString ;
class QDialog ;
class QWidget ;
class QTranslator;
class QApplication;
class RsCacheService ;
class ftServer ;
class ConfigPage ;
class RsPQIService ;
class RsAutoUpdatePage ;
class SoundEvents;
class FeedNotify;
class ToasterNotify;
class ChatWidget;
class ChatWidgetHolder;
// for gxs based plugins
struct RsIdentity;
class RsNxsNetMgr;
class RsGxsIdExchange;
class RsGcxs;
class PgpAuxUtils;
class p3Config;

namespace resource_api
{
    class ResourceRouter;
    class StateTokenServer;
}

// Plugin API version. Not used yet, but will be in the future the
// main value that decides for compatibility.
//
#define RS_PLUGIN_API_VERSION        0x000101

// Used for the status of plugins.
//
#define PLUGIN_STATUS_NO_STATUS      0x0000
#define PLUGIN_STATUS_REJECTED_HASH  0x0001
#define PLUGIN_STATUS_DLOPEN_ERROR   0x0002
#define PLUGIN_STATUS_MISSING_SYMBOL 0x0003
#define PLUGIN_STATUS_NULL_PLUGIN    0x0004
#define PLUGIN_STATUS_LOADED         0x0005
#define PLUGIN_STATUS_WRONG_API      0x0006
#define PLUGIN_STATUS_MISSING_API    0x0007
#define PLUGIN_STATUS_MISSING_SVN    0x0008

class RsPluginHandler;

/*!
 *
 * convenience class to store gui interfaces
 *
 */
class RsPlugInInterfaces {
public:
    RsUtil::inited_ptr<RsPeers>  mPeers;
    RsUtil::inited_ptr<RsFiles>  mFiles;
    RsUtil::inited_ptr<RsMsgs>   mMsgs;
    RsUtil::inited_ptr<RsTurtle> mTurtle;
    RsUtil::inited_ptr<RsDisc>   mDisc;
    RsUtil::inited_ptr<RsDht>    mDht;
    RsUtil::inited_ptr<RsNotify> mNotify;
    RsUtil::inited_ptr<RsServiceControl> mServiceControl;
    RsUtil::inited_ptr<RsPluginHandler> mPluginHandler;

    // gxs
    std::string     mGxsDir;
    RsUtil::inited_ptr<RsIdentity>      mIdentity;
    RsUtil::inited_ptr<RsNxsNetMgr>     mRsNxsNetMgr;
    RsUtil::inited_ptr<RsGxsIdExchange> mGxsIdService;
    RsUtil::inited_ptr<RsGcxs>          mGxsCirlces;
    RsUtil::inited_ptr<PgpAuxUtils>     mPgpAuxUtils;
    RsUtil::inited_ptr<RsGxsForums>     mGxsForums;
    RsUtil::inited_ptr<RsGxsChannels>   mGxsChannels;
    RsUtil::inited_ptr<RsGxsTunnelService>    mGxsTunnels;
    RsUtil::inited_ptr<RsReputations>   mReputations;
};

class RsPlugin
{
	public:
		RsPlugin() {}
		virtual ~RsPlugin() {}

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
        //virtual RsPQIService   *rs_pqi_service() 		const	{ return NULL ; }
        // gxs netservice is not a RsPQIService
        // so have two fns which result in the same gxs netservice to be returned
        virtual p3Service   *p3_service() 		const	{ return NULL ; }
        virtual p3Config   *p3_config() 		const	{ return NULL ; }
		virtual uint16_t        rs_service_id() 	   const	{ return 0    ; }


        // creates a new resource api handler object. ownership is transferred to the caller.
        // the caller should supply a statetokenserver, and keep it valid until destruction
        // the plugin should return a entry point name. this is to make the entry point name independent from file names
        virtual resource_api::ResourceRouter* new_resource_api_handler(const RsPlugInInterfaces& /* ifaces */, resource_api::StateTokenServer* /* sts */, std::string & /*entrypoint*/) const { return 0;}

		// Shutdown
		virtual void stop() {}

		// Filename used for saving the specific plugin configuration. Both RsCacheService and RsPQIService
		// derive from p3Config, which means that the service provided by the plugin can load/save its own
		// config by deriving loadList() and saveList() from p3Config.
		//
		virtual std::string configurationFileName() const 
		{ 
			std::cerr << "(EE) Plugin configuration file name requested in non overloaded method! Plugin code should derive configurationFileName() method!" << std::endl;
			return std::string() ;
		}

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
		virtual QDialog        		*qt_about_page()	   const	{ return NULL ; } // About/Help button in plugin entry will show this up
		virtual ConfigPage     		*qt_config_page()  	const	{ return NULL ; } // Config tab to add in config panel.
		virtual RsAutoUpdatePage 	*qt_transfers_tab()	const	{ return NULL ; } // Tab to add in transfers, after turtle statistics.
		virtual std::string   		 qt_transfers_tab_name()const	{ return "Tab" ; } // Tab name
		virtual void         		 qt_sound_events(SoundEvents &/*events*/) const	{ } // Sound events

		// Provide buttons for the ChatWidget
		virtual ChatWidgetHolder    *qt_get_chat_widget_holder(ChatWidget */*chatWidget*/) const { return NULL ; }

		virtual std::string    qt_stylesheet() { return ""; }
		virtual QTranslator    *qt_translator(QApplication * /* app */, const QString& /* languageCode */, const QString& /* externalDir */ ) const	{ return NULL ; }

		//
		//================================== Notify ==================================//
		//
		virtual FeedNotify *qt_feedNotify() { return NULL; }
		virtual ToasterNotify *qt_toasterNotify() { return NULL; }

		//
		//========================== Plugin Description ==============================//
		//
		//  All these items appear in the config->plugins tab, as a description of the plugin.
		//
		uint32_t getSvnRevision() const { return 0; } 	// This is read from libretroshare/retroshare/rsversion.h

		virtual std::string getShortPluginDescription() const = 0 ;
		virtual std::string getPluginName() const = 0 ;
		virtual void getPluginVersion(int& major,int& minor, int& build, int& svn_rev) const = 0 ;
		virtual void getLibraries(std::list<RsLibraryInfo> & /*libraries*/) {}

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
        virtual void getPluginStatus(int i,uint32_t& status,std::string& file_name,RsFileHash& file_hash,uint32_t& svn_revision,std::string& error_string) const = 0 ;
        virtual void enablePlugin(const RsFileHash& hash) = 0;
        virtual void disablePlugin(const RsFileHash& hash) = 0;

		virtual void allowAllPlugins(bool b) = 0 ;
		virtual bool getAllowAllPlugins() const = 0 ;

		virtual void slowTickPlugins(rstime_t sec) = 0 ;

		virtual const std::string& getLocalCacheDir() const =0;
        virtual const std::string& getRemoteCacheDir() const =0;

        virtual RsServiceControl *getServiceControl() const = 0;
};



