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

//=============================================================================
   
class PluginFrame : public QFrame
{
    Q_OBJECT

public:
    PluginFrame(QWidget* parent ,  QString pluginName );
    virtual ~PluginFrame();

signals:
    void needToLoad(QString pluginName);
    void needToUnload(QString pluginName);
    void needToRemove(QString pluginName);

public slots:
    void successfulLoad(QString pluginName, QWidget* wd=0);
    
protected slots:
    void loadButtonClicked() ;
    void removeButtonClicked();

protected:
    QString plgName;

    QPushButton* loadBtn;
    unsigned char loadBtnState;
    QPushButton* removeBtn;
    QVBoxLayout* buttonsLay;

    QHBoxLayout* mainLay; // main layout for the frame

    QVBoxLayout* labelsLay;
    QLabel* nameLabel;
    QLabel* descrLabel;

};

//=============================================================================

class PluginManagerWidget: public QFrame
{
    Q_OBJECT

public:
    PluginManagerWidget(QWidget* parent);
    virtual ~PluginManagerWidget();

    void addPluginWidget(PluginFrame* pf);

signals:
    void needToLoadFileWithPlugin(QString fileName) ;

protected:
    QVBoxLayout* vlay;

    QPushButton* instPlgButton;
    QHBoxLayout* instPlgLay;
    QSpacerItem* instPlgSpacer;

protected slots:
   void instPlgButtonClicked();
};
			   
//=============================================================================

#endif

