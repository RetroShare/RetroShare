#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

#include <QObject>

#include <QString>
#include <QStringList>
#include <QList>

#include <QPluginLoader>

#include <QVector>

class PluginInterface;

class PluginManager: public QObject
{
    Q_OBJECT

public:
    PluginManager();
    ~PluginManager();

    void defaultLoad(QString baseDir = "");

    QStringList availablePlugins();
    QStringList loadedPlugins();

    bool isLoaded(QString pluginName);
    void unloadPlugin(QString pluginName);

public slots:
    void loadPlugin(QString pluginName);
    void readPluginFromFile(QString fullFileName); 
//    void loadPluginFromFile(QString fileName);


signals:
    void loadDone(QString pluginName, QWidget* pluginWidget);
    void loadFailed(QString pluginName, QString errorMessage);
    void installComplete(QString pluginName);
    void installFailed(QString pluginFileName, QString errorMessage);

protected:

    //QList<QPluginLoader*> loaders;
    QList<QWidget*> widgets;
//    QList<int> widgets;
    QStringList fileNames;
    QList<QString> names;
//    QList<int> states;

    PluginInterface* loadPluginInterface(QString fileName, 
	                                 QString& errorMessage) ;

}; 


#endif


