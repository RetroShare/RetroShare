#pragma once

#include <retroshare/rsplugin.h>
#include "services/p3vors.h"

class VOIPPlugin: public RsPlugin
{
	public:
		VOIPPlugin() ;
		virtual ~VOIPPlugin() {}

		virtual RsPQIService   *rs_pqi_service() 			const	;
		virtual uint16_t        rs_service_id()         const { return RS_SERVICE_TYPE_VOIP_PLUGIN ; }
		virtual ConfigPage     *qt_config_page()        const ;

		virtual QTranslator    *qt_translator(QApplication *app, const QString& languageCode) const;

		virtual void getPluginVersion(int& major,int& minor,int& svn_rev) const ;
		virtual void setPlugInHandler(RsPluginHandler *pgHandler);

		virtual std::string configurationFileName() const { return "voip.cfg" ; }

		virtual std::string getShortPluginDescription() const ;
		virtual std::string getPluginName() const;
		virtual void setInterfaces(RsPlugInInterfaces& interfaces);

	private:
		mutable p3VoRS *mVoip ;
		mutable RsPluginHandler *mPlugInHandler;
		mutable RsPeers* mPeers;
		mutable ConfigPage *config_page ;
};

