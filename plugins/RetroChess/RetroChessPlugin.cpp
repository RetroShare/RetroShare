/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <retroshare/rsplugin.h>
#include <retroshare/rsversion.h>
#include <retroshare-gui/RsAutoUpdatePage.h>
#include <QTranslator>
#include <QApplication>
#include <QString>
#include <QIcon>
#include <QMessageBox>
#include "gui/chat/ChatWidget.h"

#include "RetroChessPlugin.h"
#include "interface/rsRetroChess.h"
#include "gui/NEMainpage.h"
#include "gui/RetroChessNotify.h"
#include "gui/RetroChessChatWidgetHolder.h"


#define IMAGE_RetroChess ":/images/chess.png"

static void *inited = new RetroChessPlugin() ;

extern "C" {

	// This is *the* functions required by RS plugin system to give RS access to the plugin.
	// Be careful to:
	// - always respect the C linkage convention
	// - always return an object of type RsPlugin*
	//
	void *RETROSHARE_PLUGIN_provide()
	{
		static RetroChessPlugin *p = new RetroChessPlugin() ;

		return (void*)p ;
	}

	// This symbol contains the svn revision number grabbed from the executable. 
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
	uint32_t RETROSHARE_PLUGIN_revision = abs(atoi(RS_EXTRA_VERSION)) ;

	// This symbol contains the svn revision number grabbed from the executable. 
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
	uint32_t RETROSHARE_PLUGIN_api = RS_PLUGIN_API_VERSION ;
}

void RetroChessPlugin::getPluginVersion(int& major, int& minor, int& build, int& svn_rev) const
{
	major = RS_MAJOR_VERSION ;
	minor = RS_MINOR_VERSION ;
	build = RS_MINI_VERSION ;
	svn_rev = abs(atoi(RS_EXTRA_VERSION)) ;
}

RetroChessPlugin::RetroChessPlugin()
{
	qRegisterMetaType<RsPeerId>("RsPeerId");
	mainpage = NULL ;
	mRetroChess = NULL ;
	mPlugInHandler = NULL;
	mPeers = NULL;
	config_page = NULL ;
	mIcon = NULL ;

	mRetroChessNotify = new RetroChessNotify;
}

void RetroChessPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
    mPeers = interfaces.mPeers;
}

/*ConfigPage *RetroChessPlugin::qt_config_page() const
{
	// The config pages are deleted when config is closed, so it's important not to static the
	// created object.
	//
	return new AudioInputConfig() ;
}*/

QDialog *RetroChessPlugin::qt_about_page() const
{
	static QMessageBox *about_dialog = NULL ;
	
	if(about_dialog == NULL)
	{
		about_dialog = new QMessageBox() ;

		QString text ;
		text += QObject::tr("<h3>RetroShare RetroChess plugin</h3><br/>   * Contributors: Cyril Soler, Josselin Jacquard<br/>") ;
		text += QObject::tr("<br/>The RetroChess plugin adds RetroChess to the private chat window of RetroShare. to use it, proceed as follows:<UL>") ;
		text += QObject::tr("<li> setup microphone levels using the configuration panel</li>") ;
		text += QObject::tr("<li> check your microphone by looking at the VU-metters</li>") ;
		text += QObject::tr("<li> in the private chat, enable sound input/output by clicking on the two RetroChess icons</li></ul>") ;
		text += QObject::tr("Your friend needs to run the plugin to talk/listen to you, or course.") ;
		text += QObject::tr("<br/><br/>This is an experimental feature. Don't hesitate to send comments and suggestion to the RS dev team.") ;

		about_dialog->setText(text) ;
		about_dialog->setStandardButtons(QMessageBox::Ok) ;
	}

	return about_dialog ;
}

ChatWidgetHolder *RetroChessPlugin::qt_get_chat_widget_holder(ChatWidget *chatWidget) const
{
	switch (chatWidget->chatType()) {
	case ChatWidget::CHATTYPE_PRIVATE:
		return new RetroChessChatWidgetHolder(chatWidget, mRetroChessNotify);
	case ChatWidget::CHATTYPE_UNKNOWN:
	case ChatWidget::CHATTYPE_LOBBY:
	case ChatWidget::CHATTYPE_DISTANT:
		break;
	}

	return NULL;
}

p3Service *RetroChessPlugin::p3_service() const
{
	if(mRetroChess == NULL)
		rsRetroChess = mRetroChess = new p3RetroChess(mPlugInHandler,mRetroChessNotify) ; // , 3600 * 24 * 30 * 6); // 6 Months

	return mRetroChess ;
}

void RetroChessPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
    mPlugInHandler = pgHandler;
}

QIcon *RetroChessPlugin::qt_icon() const
{
	if (mIcon == NULL) {
		Q_INIT_RESOURCE(RetroChess_images);

		mIcon = new QIcon(IMAGE_RetroChess);
	}

	return mIcon;
}
MainPage *RetroChessPlugin::qt_page() const
{
	if(mainpage == NULL){
		mainpage = new NEMainpage(0, mRetroChessNotify);//mPeers, mFiles) ;
		//tpage = new NEMainpage( );
		//mainpage = tpage;
	}

	return mainpage ;
}

std::string RetroChessPlugin::getShortPluginDescription() const
{
	return "RetroChess";
}

std::string RetroChessPlugin::getPluginName() const
{
	return "RetroChess";
}

QTranslator* RetroChessPlugin::qt_translator(QApplication */*app*/, const QString& languageCode, const QString& externalDir) const
{
	return NULL;
}

void RetroChessPlugin::qt_sound_events(SoundEvents &/*events*/) const
{
//	events.addEvent(QApplication::translate("RetroChess", "RetroChess"), QApplication::translate("RetroChess", "Incoming call"), RetroChess_SOUND_INCOMING_CALL);
}

/*ToasterNotify *RetroChessPlugin::qt_toasterNotify(){
	if (!mRetroChessToasterNotify) {
		mRetroChessToasterNotify = new RetroChessToasterNotify(mRetroChess, mRetroChessNotify);
	}
	return mRetroChessToasterNotify;
}*/
