#pragma once

#include <retroshare/rsplugin.h>
#include "services/p3vors.h"

class PluginGUIHandler ;
class PluginNotifier ;

class VOIPPlugin: public RsPlugin
{
	public:
		VOIPPlugin() ;
		virtual ~VOIPPlugin() {}

		virtual RsPQIService   *rs_pqi_service() 			const	;
		virtual uint16_t        rs_service_id()         const { return RS_SERVICE_TYPE_VOIP_PLUGIN ; }
		virtual ConfigPage     *qt_config_page()        const ;
		virtual QDialog        *qt_about_page()         const ;
		virtual RsAutoUpdatePage *qt_transfers_tab()    const ;
		virtual std::string qt_transfers_tab_name()    const ;
        virtual PopupChatDialog_WidgetsHolder  *qt_allocate_new_popup_chat_dialog_widgets() const ;
		
		virtual QIcon *qt_icon() const;
		virtual QTranslator    *qt_translator(QApplication *app, const QString& languageCode, const QString& externalDir) const;
		virtual void           qt_sound_events(SoundEvents &events) const;

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
		mutable QIcon *mIcon;

		PluginNotifier *mPluginNotifier ;
		PluginGUIHandler *mPluginGUIHandler ;
};

