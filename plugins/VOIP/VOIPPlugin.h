/*******************************************************************************
 * plugins/VOIP/VOIPPlugin.h                                                   *
 *                                                                             *
 * Copyright 2015 by retroshare team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

/*VOIP*/
#include "gui/VOIPToasterNotify.h"
#include "services/p3VOIP.h"

/*libretroshare"*/
#include <retroshare/rsplugin.h>

class VOIPGUIHandler ;
class VOIPNotify ;

class VOIPPlugin: public RsPlugin
{
	public:
		VOIPPlugin() ;
		virtual ~VOIPPlugin() {}

		virtual p3Service      *p3_service() 			const	;
		virtual p3Config       *p3_config()             const { return mVOIP; }
		virtual uint16_t        rs_service_id()         const { return RS_SERVICE_TYPE_VOIP_PLUGIN ; }
		virtual ConfigPage     *qt_config_page()        const ;
		virtual QDialog        *qt_about_page()         const ;
		virtual ChatWidgetHolder *qt_get_chat_widget_holder(ChatWidget *chatWidget) const ;
		
		virtual QIcon *qt_icon() const;
		virtual std::string    qt_stylesheet() { return "VOIP"; }
		virtual QTranslator    *qt_translator(QApplication *app, const QString& languageCode, const QString& externalDir) const;
		virtual void           qt_sound_events(SoundEvents &events) const;

		virtual void getPluginVersion(int& major, int& minor, int &build, int& svn_rev) const ;
		virtual void setPlugInHandler(RsPluginHandler *pgHandler);

		virtual std::string configurationFileName() const { return "voip.cfg" ; }

		virtual std::string getShortPluginDescription() const ;
		virtual std::string getPluginName() const;
		virtual void getLibraries(std::list<RsLibraryInfo> &libraries);
		virtual void setInterfaces(RsPlugInInterfaces& interfaces);

		//================================== RsPlugin Notify ==================================//
		virtual ToasterNotify *qt_toasterNotify();

	private:
		mutable p3VOIP *mVOIP ;
		mutable RsPluginHandler *mPlugInHandler;
		mutable RsPeers* mPeers;
		mutable ConfigPage *config_page ;
		mutable QIcon *mIcon;

		VOIPNotify *mVOIPNotify ;
		VOIPGUIHandler *mVOIPGUIHandler ;
		VOIPToasterNotify *mVOIPToasterNotify ;
};

