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

#include "mplayerwindow.h"
#include "global.h"
#include "desktopinfo.h"
#include "colorutils.h"

#ifndef MINILIB
#include "images.h"
#endif

#include <QLabel>
#include <QTimer>
#include <QCursor>
#include <QEvent>
#include <QLayout>
#include <QPixmap>
#include <QPainter>

#if DELAYED_RESIZE
#include <QTimer>
#endif

Screen::Screen(QWidget* parent, Qt::WindowFlags f) : QWidget(parent, f ) 
{
	setMouseTracking(TRUE);
	setFocusPolicy( Qt::NoFocus );
	setMinimumSize( QSize(0,0) );

#if NEW_MOUSE_CHECK_POS
	mouse_last_position = QPoint(0,0);
#else
	cursor_pos = QPoint(0,0);
	last_cursor_pos = QPoint(0,0);
#endif

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL(timeout()), this, SLOT(checkMousePos()) );
#if NEW_MOUSE_CHECK_POS
	timer->start(500);
#else
	timer->start(2000);
#endif

	// Change attributes
	setAttribute(Qt::WA_NoSystemBackground);
	//setAttribute(Qt::WA_StaticContents);
    //setAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_PaintUnclipped);
	//setAttribute(Qt::WA_PaintOutsidePaintEvent);
}

Screen::~Screen() {
}

void Screen::paintEvent( QPaintEvent * e ) {
	//qDebug("Screen::paintEvent");
	QPainter painter(this);
	painter.eraseRect( e->rect() );
	//painter.fillRect( e->rect(), QColor(255,0,0) );
}

#if NEW_MOUSE_CHECK_POS
void Screen::checkMousePos() {
	//qDebug("Screen::checkMousePos");
	QPoint pos = mapFromGlobal(QCursor::pos());

	if (mouse_last_position != pos) {
		setCursor(QCursor(Qt::ArrowCursor));
	} else {
		setCursor(QCursor(Qt::BlankCursor));
	}
	mouse_last_position = pos;
}
#else
void Screen::checkMousePos() {
	//qDebug("Screen::checkMousePos");
	
	if ( cursor_pos == last_cursor_pos ) {
		//qDebug(" same pos");
		if (cursor().shape() != Qt::BlankCursor) {
			//qDebug(" hiding mouse cursor");
			setCursor(QCursor(Qt::BlankCursor));
		}
	} else {
		last_cursor_pos = cursor_pos;
	}
}

void Screen::mouseMoveEvent( QMouseEvent * e ) {
	//qDebug("Screen::mouseMoveEvent");
	//qDebug(" pos: x: %d y: %d", e->pos().x(), e->pos().y() );
	cursor_pos = e->pos();

	if (cursor().shape() != Qt::ArrowCursor) {
		//qDebug(" showing mouse cursor" );
		setCursor(QCursor(Qt::ArrowCursor));
	}
}
#endif

/* ---------------------------------------------------------------------- */

MplayerLayer::MplayerLayer(QWidget* parent, Qt::WindowFlags f) 
	: Screen(parent, f) 
{
#if REPAINT_BACKGROUND_OPTION
	repaint_background = true;
#endif
	playing = false;
}

MplayerLayer::~MplayerLayer() {
}

#if REPAINT_BACKGROUND_OPTION
void MplayerLayer::setRepaintBackground(bool b) {
	qDebug("MplayerLayer::setRepaintBackground: %d", b);
	repaint_background = b;
}

void MplayerLayer::paintEvent( QPaintEvent * e ) {
	//qDebug("MplayerLayer::paintEvent: allow_clearing: %d", allow_clearing);
	if (repaint_background || !playing) {
		//qDebug("MplayerLayer::paintEvent: painting");
		Screen::paintEvent(e);
	}
}
#endif

void MplayerLayer::playingStarted() {
	qDebug("MplayerLayer::playingStarted");
	repaint();
	playing = true;
}

void MplayerLayer::playingStopped() {
	qDebug("MplayerLayer::playingStopped");
	playing = false;
	repaint();
}

/* ---------------------------------------------------------------------- */

MplayerWindow::MplayerWindow(QWidget* parent, Qt::WindowFlags f) 
	: Screen(parent, f) , allow_video_movement(false)
{
	offset_x = 0;
	offset_y = 0;
	zoom_factor = 1.0;

	setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( this, QColor(0,0,0) );

	mplayerlayer = new MplayerLayer( this );
	mplayerlayer->setAutoFillBackground(TRUE);

	logo = new QLabel( mplayerlayer );
	logo->setAutoFillBackground(TRUE);
#if QT_VERSION >= 0x040400
	logo->setAttribute(Qt::WA_NativeWindow); // Otherwise the logo is not visible in Qt 4.4
#else
	logo->setAttribute(Qt::WA_PaintOnScreen); // Fixes the problem if compiled with Qt < 4.4
#endif
	ColorUtils::setBackgroundColor( logo, QColor(0,0,0) );

	QVBoxLayout * mplayerlayerLayout = new QVBoxLayout( mplayerlayer );
	mplayerlayerLayout->addWidget( logo, 0, Qt::AlignHCenter | Qt::AlignVCenter );

    aspect = (double) 4 / 3;
	monitoraspect = 0;

	setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Expanding );
	setFocusPolicy( Qt::StrongFocus );

	installEventFilter(this);
	mplayerlayer->installEventFilter(this);
	//logo->installEventFilter(this);

#if DELAYED_RESIZE
	resize_timer = new QTimer(this);
	resize_timer->setSingleShot(true);
	resize_timer->setInterval(50);
	connect( resize_timer, SIGNAL(timeout()), this, SLOT(resizeLater()) );
#endif

	retranslateStrings();
}

MplayerWindow::~MplayerWindow() {
}

#if USE_COLORKEY
void MplayerWindow::setColorKey( QColor c ) {
	ColorUtils::setBackgroundColor( mplayerlayer, c );
}
#endif

void MplayerWindow::retranslateStrings() {
	//qDebug("MplayerWindow::retranslateStrings");
#ifndef MINILIB
	logo->setPixmap( Images::icon("background") );
#endif
}

void MplayerWindow::showLogo( bool b)
{
	if (b) logo->show(); else logo->hide();
}

/*
void MplayerWindow::changePolicy() {
	setSizePolicy( QSizePolicy::Preferred , QSizePolicy::Preferred  );
}
*/

void MplayerWindow::setResolution( int w, int h)
{
    video_width = w;
    video_height = h;
    
    //mplayerlayer->move(1,1);
    updateVideoWindow();
}


void MplayerWindow::resizeEvent( QResizeEvent * /* e */)
{
   /*qDebug("MplayerWindow::resizeEvent: %d, %d",
	   e->size().width(), e->size().height() );*/

#if !DELAYED_RESIZE
	offset_x = 0;
	offset_y = 0;

    updateVideoWindow();
	setZoom(zoom_factor);
#else
	resize_timer->start();
#endif
}

#if DELAYED_RESIZE
void MplayerWindow::resizeLater() {
	offset_x = 0;
	offset_y = 0;

    updateVideoWindow();
	setZoom(zoom_factor);
}
#endif

void MplayerWindow::setMonitorAspect(double asp) {
	monitoraspect = asp;
}

void MplayerWindow::setAspect( double asp) {
    aspect = asp;
	if (monitoraspect!=0) {
		aspect = aspect / monitoraspect * DesktopInfo::desktop_aspectRatio(this);
	}
	updateVideoWindow();
}


void MplayerWindow::updateVideoWindow()
{
	//qDebug("MplayerWindow::updateVideoWindow");

    //qDebug("aspect= %f", aspect);

    int w_width = size().width();
    int w_height = size().height();

	int w = w_width;
	int h = w_height;
	int x = 0;
	int y = 0;

	if (aspect != 0) {
	    int pos1_w = w_width;
	    int pos1_h = w_width / aspect + 0.5;
    
	    int pos2_h = w_height;
	    int pos2_w = w_height * aspect + 0.5;
    
	    //qDebug("pos1_w: %d, pos1_h: %d", pos1_w, pos1_h);
	    //qDebug("pos2_w: %d, pos2_h: %d", pos2_w, pos2_h);
    
	    if (pos1_h <= w_height) {
		//qDebug("Pos1!");
			w = pos1_w;
			h = pos1_h;
	
			y = (w_height - h) /2;
	    } else {
		//qDebug("Pos2!");
			w = pos2_w;
			h = pos2_h;
	
			x = (w_width - w) /2;
	    }
	}

    mplayerlayer->move(x,y);
    mplayerlayer->resize(w, h);

	orig_x = x;
	orig_y = y;
	orig_width = w;
	orig_height = h;
    
    //qDebug( "w_width: %d, w_height: %d", w_width, w_height);
    //qDebug("w: %d, h: %d", w,h);
}


void MplayerWindow::mouseReleaseEvent( QMouseEvent * e) {
    qDebug( "MplayerWindow::mouseReleaseEvent" );

	if (e->button() == Qt::LeftButton) {
		e->accept();
		emit leftClicked();
	}
	else
	if (e->button() == Qt::MidButton) {
		e->accept();
		emit middleClicked();
	}
	else
	if (e->button() == Qt::XButton1) {
		e->accept();
		emit xbutton1Clicked();
	}
	else
	if (e->button() == Qt::XButton2) {
		e->accept();
		emit xbutton2Clicked();
	}
	else
    if (e->button() == Qt::RightButton) {
		e->accept();
		//emit rightButtonReleased( e->globalPos() );
		emit rightClicked();
    } 
	else {
		e->ignore();
	}
}

void MplayerWindow::mouseDoubleClickEvent( QMouseEvent * e ) {
	if (e->button() == Qt::LeftButton) {
		e->accept();
		emit doubleClicked();
	} else {
		e->ignore();
	}
}

void MplayerWindow::wheelEvent( QWheelEvent * e ) {
    qDebug("MplayerWindow::wheelEvent: delta: %d", e->delta());
    e->accept();

	if (e->orientation() == Qt::Vertical) {
	    if (e->delta() >= 0)
	        emit wheelUp();
	    else
	        emit wheelDown();
	} else {
		qDebug("MplayerWindow::wheelEvent: horizontal event received, doing nothing");
	}
}

bool MplayerWindow::eventFilter( QObject * /*watched*/, QEvent * event ) {
	//qDebug("MplayerWindow::eventFilter");

	if ( (event->type() == QEvent::MouseMove) || 
         (event->type() == QEvent::MouseButtonRelease) ) 
	{
		QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event);

		if (event->type() == QEvent::MouseMove) {
			emit mouseMoved(mouse_event->pos());
		}
	}

	return false;
}

QSize MplayerWindow::sizeHint() const {
	//qDebug("MplayerWindow::sizeHint");
	return QSize( video_width, video_height );
}

QSize MplayerWindow::minimumSizeHint () const {
	return QSize(0,0);
}

void MplayerWindow::setOffsetX( int d) {
	offset_x = d;
	mplayerlayer->move( orig_x + offset_x, mplayerlayer->y() );
}

int MplayerWindow::offsetX() { return offset_x; }

void MplayerWindow::setOffsetY( int d) {
	offset_y = d;
	mplayerlayer->move( mplayerlayer->x(), orig_y + offset_y );
}

int MplayerWindow::offsetY() { return offset_y; }

void MplayerWindow::setZoom( double d) {
	zoom_factor = d;
	offset_x = 0;
	offset_y = 0;

	int x = orig_x;
	int y = orig_y;
	int w = orig_width;
	int h = orig_height;

	if (zoom_factor != 1.0) {
		w = w * zoom_factor;
		h = h * zoom_factor;

		// Center
		x = (width() - w) / 2;
		y = (height() -h) / 2;
	}

	mplayerlayer->move(x,y);
	mplayerlayer->resize(w,h);
}

double MplayerWindow::zoom() { return zoom_factor; }

void MplayerWindow::moveLayer( int offset_x, int offset_y ) {
	int x = mplayerlayer->x();
	int y = mplayerlayer->y();

	mplayerlayer->move( x + offset_x, y + offset_y );
}

void MplayerWindow::moveLeft() {
	if ((allow_video_movement) || (mplayerlayer->x()+mplayerlayer->width() > width() ))
		moveLayer( -16, 0 );
}

void MplayerWindow::moveRight() {
	if ((allow_video_movement) || ( mplayerlayer->x() < 0 ))
		moveLayer( +16, 0 );
}

void MplayerWindow::moveUp() {
	if ((allow_video_movement) || (mplayerlayer->y()+mplayerlayer->height() > height() ))
		moveLayer( 0, -16 );
}

void MplayerWindow::moveDown() {
	if ((allow_video_movement) || ( mplayerlayer->y() < 0 ))
		moveLayer( 0, +16 );
}

void MplayerWindow::incZoom() {
	setZoom( zoom_factor + ZOOM_STEP );
}

void MplayerWindow::decZoom() {
	double zoom = zoom_factor - ZOOM_STEP;
	if (zoom < ZOOM_MIN) zoom = ZOOM_MIN;
	setZoom( zoom );
}

// Language change stuff
void MplayerWindow::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_mplayerwindow.cpp"
