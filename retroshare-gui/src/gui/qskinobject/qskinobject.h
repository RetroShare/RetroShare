/********************************************************************************************************
 * PROGRAM      : childform
 * DATE - TIME  : Samstag 30 Dezember 2006 - 12h04
 * AUTHOR       :  (  )
 * FILENAME     : QSkinMainWindow.h
 * LICENSE      : 
 * COMMENTARY   : 
 ********************************************************************************************************/
#ifndef QSkinObject_H
#define QSkinObject_H
#include "qskinwidgetresizehandler.h"

#include <QtCore>
#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QBitmap>
#include <QEvent>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QBasicTimer>
#ifdef WIN32
#define _WIN32_WINNT 0x0500
#define WINVER 0x0500
#include <windows.h>
#endif
class QSkinWidgetResizeHandler;
class  QSkinObject : public QObject
{
    Q_OBJECT
    friend class QSkinWidgetResizeHandler;
public:
    	QSkinObject(QWidget* wgtParent);
	~QSkinObject(){}
	void setSkinPath(const QString & skinpath);
	QString getSkinPath();
	int customFrameWidth();
public slots:
	void updateStyle();
	void updateButtons();
	void startSkinning();
	void stopSkinning();
protected:
	bool eventFilter(QObject *o, QEvent *e);
	//Events to filter
	//void mouseMoveEvent(QMouseEvent *event);
    	void mousePressEvent(QMouseEvent *event);
    	//void mouseReleaseEvent(QMouseEvent *mouseEvent);
    	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *e);
	//void closeEvent(QCloseEvent *e);

	void loadSkinIni();
	void manageRegions();
	QPixmap drawCtrl(QWidget * widget);
	QRegion childRegion;
	void timerEvent ( QTimerEvent * event );
private:
	QRect saveRect;
	QRect skinGeometry;
	QPixmap widgetMask;//the pixmap, in which the ready frame is stored on pressed? 
	QString skinPath;
	QFont titleFont;
	QColor titleColor;
	bool milchglas;
    bool gotMousePress;	
	QRegion quitButton;
	QRegion maxButton;
	QRegion minButton;
	QRect contentsRect;
	QSkinWidgetResizeHandler * resizeHandler;
	bool mousePress;
	QBasicTimer *skinTimer;
	QWidget *skinWidget;
	void fastbluralpha(QImage &img, int radius);
	Qt::WindowFlags flags;
	int wlong;
#ifdef WIN32
public slots:
	void setLayered();
	void updateAlpha();
private:
	double alpha;
#endif
};
#endif

