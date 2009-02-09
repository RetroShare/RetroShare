#ifndef _PLUGIN_MANAGER_WIDGET_
#define _PLUGIN_MANAGER_WIDGET_

#include <QFrame>

#include <QString>

class QWidget;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QSpacerItem;
class QTextEdit;

//=============================================================================
   
class PluginFrame : public QFrame
{
    Q_OBJECT

public:
    PluginFrame( QString pluginName, QWidget* parent =0 );
    virtual ~PluginFrame();

    QString getPluginName();

signals:
    void needToRemove(QString pluginName);
    
protected slots:
    void removeButtonClicked();

protected:
    QString plgName;

    QPushButton* removeBtn;
    QVBoxLayout* buttonsLay;

    QHBoxLayout* mainLay; // main layout for the frame

    QVBoxLayout* labelsLay;
    QLabel* nameLabel;
    QLabel* descrLabel;

};

//=============================================================================

//! GUI representation of the PluginManager class

//!     This is something like GUI for PluginManager class. Or you can think
//! about PluginManagerWidget as a view, and a PluginManager as a model. 
//! Instances should be created only by PluginManager class; maybe later i'll
//! hide constructor in some way. Parent (or somebody else) can delete it.
//! Widget itself can be used anywere, in some 'settings' dialogs. 
class PluginManagerWidget: public QFrame
{
    Q_OBJECT

public:
    PluginManagerWidget(QWidget* parent =0);
    virtual ~PluginManagerWidget();

    void registerNewPlugin(QString pluginName);
    void removePluginFrame(QString pluginName);

signals:
    void removeRequested(QString pluginName);
    void installPluginRequested(QString fileName) ;

public slots:
    void acceptErrorMessage(QString errorMessage);

protected slots:
    
protected:
    QVBoxLayout* mainLayout;
    QFrame*      pluginFramesContainer;
    QVBoxLayout* pluginFramesLayout;

    QPushButton* installPluginButton;
    QHBoxLayout* installPluginLayout;
    QSpacerItem* installPluginSpacer;

    QTextEdit* errorsConsole;

protected slots:
    void installPluginButtonClicked();
};
			   
//=============================================================================

#endif

