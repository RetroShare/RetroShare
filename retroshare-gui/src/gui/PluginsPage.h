#ifndef _PLUGINS_PAGE_H_
#define _PLUGINS_PAGE_H_

//#include <QFileDialog>

//#include "chat/PopupChatDialog.h"

#include "mainpage.h"
//#include "ui_PeersDialog.h"

class QGroupBox;
class QVBoxLayout;
class QTabWidget;
class QFrame;
class QLabel;
class QScriptEngine;

class PluginsPage : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  PluginsPage(QWidget *parent = 0);
  /** Default Destructor */
  ~PluginsPage() ;

protected:
   QVBoxLayout* pluginPageLayout;
   QGroupBox* pluginPanel;
   QVBoxLayout* pluginPanelLayout;

   QTabWidget* pluginTabs ;

   QScriptEngine* engine;

};

#endif
