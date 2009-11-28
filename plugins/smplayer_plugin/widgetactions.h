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

#ifndef _WIDGETACTIONS_H_
#define _WIDGETACTIONS_H_

#include <QWidgetAction>
#include "timeslider.h"
#include "config.h"
#include "guiconfig.h"

class QStyle;

class MyWidgetAction : public QWidgetAction
{
	Q_OBJECT

public:
	MyWidgetAction( QWidget * parent );
	~MyWidgetAction();

	void setCustomStyle(QStyle * style) { custom_style = style; };
	QStyle * customStyle() { return custom_style; };

	void setStyleSheet(QString style) { custom_stylesheet = style; };
	QString styleSheet() { return custom_stylesheet; };

public slots:
	virtual void enable(); 	// setEnabled in QAction is not virtual :(
	virtual void disable();

protected:
	virtual void propagate_enabled(bool);

protected:
	QStyle * custom_style;
	QString custom_stylesheet;
};


class TimeSliderAction : public MyWidgetAction 
{
	Q_OBJECT

public:
	TimeSliderAction( QWidget * parent );
	~TimeSliderAction();

public slots:
	virtual void setPos(int);
	virtual int pos();
#if ENABLE_DELAYED_DRAGGING
	void setDragDelay(int);
	int dragDelay();

private:
	int drag_delay;
#endif

signals:
	void posChanged(int value);
	void draggingPos(int value);
#if ENABLE_DELAYED_DRAGGING
	void delayedDraggingPos(int);
#endif

protected:
	virtual QWidget * createWidget ( QWidget * parent );
};


class VolumeSliderAction : public MyWidgetAction 
{
	Q_OBJECT

public:
	VolumeSliderAction( QWidget * parent );
	~VolumeSliderAction();

	void setFixedSize(QSize size) { fixed_size = size; };
	QSize fixedSize() { return fixed_size; };

	void setTickPosition(QSlider::TickPosition position);
	QSlider::TickPosition tickPosition() { return tick_position; };

public slots:
	virtual void setValue(int);
	virtual int value();

signals:
	void valueChanged(int value);

protected:
	virtual QWidget * createWidget ( QWidget * parent );

private:
	QSize fixed_size;
	QSlider::TickPosition tick_position;
};


class TimeLabelAction : public MyWidgetAction 
{
	Q_OBJECT

public:
	TimeLabelAction( QWidget * parent );
	~TimeLabelAction();

	virtual QString text() { return _text; };

public slots:
	virtual void setText(QString s);

signals:
	void newText(QString s);

protected:
	virtual QWidget * createWidget ( QWidget * parent );

private:
	QString _text;
};


#if MINI_ARROW_BUTTONS
class SeekingButton : public QWidgetAction
{
	Q_OBJECT

public:
	SeekingButton( QList<QAction*> actions, QWidget * parent );
	~SeekingButton();

protected:
	virtual QWidget * createWidget ( QWidget * parent );

	QList<QAction*> _actions;
};
#endif

#endif

