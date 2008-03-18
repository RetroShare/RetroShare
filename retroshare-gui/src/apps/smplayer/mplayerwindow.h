/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef MPLAYERWINDOW_H
#define MPLAYERWINDOW_H

#include <QWidget>
#include <QSize>
#include <QPoint>

#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>

#include "config.h"

class QWidget;
class QLabel;
class QKeyEvent;

#if USE_GL_WIDGET
#include <QGLWidget>
#define SC_WIDGET QGLWidget
#else
#define SC_WIDGET QWidget
#endif


#define ZOOM_STEP 0.05
#define ZOOM_MIN 0.5


//! Screen is a widget that hides the mouse cursor after some seconds if not moved.

class Screen : public SC_WIDGET
{
	Q_OBJECT

public:
	Screen(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~Screen();

signals:
	void mouseMoved(QPoint);

protected:
	virtual void mouseMoveEvent( QMouseEvent * e );
	virtual void paintEvent ( QPaintEvent * e );

protected slots:
	virtual void checkMousePos();

private:
	QPoint cursor_pos, last_cursor_pos;
};

//! MplayerLayer can be instructed to not delete the background.

class MplayerLayer : public Screen
{
	Q_OBJECT

public:
	MplayerLayer(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~MplayerLayer();

	//! If b is true, the background of the widget will be repainted as usual.
	/*! Otherwise the background will not repainted when a video is playing. */
	void allowClearingBackground(bool b);

	//! Return true if repainting the background is allowed.
	bool isClearingBackgroundAllowed() { return allow_clearing; };

public slots:
	//! Should be called when a file has started. 
    /*! It's needed to know if the background has to be cleared or not. */
	void playingStarted();
	//! Should be called when a file has stopped.
	void playingStopped();

protected:
	virtual void paintEvent ( QPaintEvent * e );

private:
	bool allow_clearing;
	bool playing;
};


class MplayerWindow : public Screen
{
    Q_OBJECT

public:
    MplayerWindow( QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~MplayerWindow();
    
	void showLogo( bool b);

	MplayerLayer * videoLayer() { return mplayerlayer; };

	void setResolution( int w, int h);
	void setAspect( double asp);
	void setMonitorAspect(double asp);
	void updateVideoWindow();

#if USE_COLORKEY
	void setColorKey(QColor c);
#endif

	void setOffsetX( int );
	int offsetX();

	void setOffsetY( int );
	int offsetY();

	void setZoom( double );
	double zoom();

	void allowVideoMovement(bool b) { allow_video_movement = b; };
	bool isVideoMovementAllowed() { return allow_video_movement; };

	virtual QSize sizeHint () const;
	virtual QSize minimumSizeHint() const;

public slots:
	void moveLeft();
	void moveRight();
	void moveUp();
	void moveDown();
	void incZoom();
	void decZoom();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

    virtual void resizeEvent( QResizeEvent * e);
    virtual void mouseReleaseEvent( QMouseEvent * e);
	virtual void mouseDoubleClickEvent( QMouseEvent * e );
	virtual void wheelEvent( QWheelEvent * e );
	void moveLayer( int offset_x, int offset_y );

protected slots:
	virtual void translatePos(QPoint p );

signals:
    void rightButtonReleased( QPoint p );
	void doubleClicked();
	void leftClicked();
	void middleClicked();
	void keyPressed(QKeyEvent * e);
	void wheelUp();
	void wheelDown();

protected:
    int video_width, video_height;
    double aspect;
	double monitoraspect;

	MplayerLayer * mplayerlayer;
	QLabel * logo;

	// Zoom and moving
	int offset_x, offset_y;
	double zoom_factor;

	// Original pos and dimensions of the mplayerlayer
	// before zooming or moving
	int orig_x, orig_y;
	int orig_width, orig_height;

	bool allow_video_movement;
};


#endif

