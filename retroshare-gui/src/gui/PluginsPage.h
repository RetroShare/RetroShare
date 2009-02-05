#ifndef _PLUGINS_PAGE_H_
#define _PLUGINS_PAGE_H_

//#include <QFileDialog>



#include "mainpage.h"

#include <QGroupBox>
#include <QString>
#include <QDir>

//class QGroupBox;
class QVBoxLayout;
class QTabWidget;
class QFrame;
class QLabel;
class QTextEdit;
class QSpacerItem;

class QScriptEngine;

class PluginManagerWidget;
class PluginManager;

class PluginsPage : public MainPage 
{
    Q_OBJECT

public:
  /** Default Constructor */
    PluginsPage(QWidget *parent = 0);
  /** Default Destructor */
    virtual ~PluginsPage() ;

public slots:
    void loadDone(QString pluginName, QWidget* pluginWidget);
    void loadFailed(QString pluginName, QString errorMessage);

    void installComplete(QString pluginName);
    void installFailed(QString pluginFileName, QString errorMessage);
    
    void unloadPlugin(QString pluginName);

protected:
    QVBoxLayout* pluginPageLayout;
    QGroupBox* pluginPanel;
    QVBoxLayout* pluginPanelLayout;
  
    QTextEdit* errlogConsole;

//    QPushButton* instPlgButton;
//    QHBoxLayout* insPlgLay;
//    QSpacerItem* instPlgSpacer;


    
       
    QTabWidget* pluginTabs ;
    QVBoxLayout* pmLay;
    QFrame* pmFrame;
    QSpacerItem* pmSpacer;
    QString errorStrLog;
    PluginManagerWidget* pluginManagerWidget;
//  QFrame* pluginControlsContainer;    
    PluginManager* pluginManager;
   //QScriptEngine* engine;

    int loadBinaryPlugins(const QDir directory);//tring& errorString);
    int loadScriptPlugins(const QString directory, QString& errorString);

    int loadBinaryPlugin(const QString fileName, QString& errorString);
    int loadScriptPlugin(const QString dirName, QString& errorString);

};

#endif

