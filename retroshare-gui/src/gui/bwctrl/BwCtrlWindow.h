#ifndef RSBWCTRL_WINDOW_H
#define RSBWCTRL_WINDOW_H

/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 Robert Fernie
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

#include <QMainWindow>

#include <QAbstractItemDelegate>

// Defines for download list list columns
#define COLUMN_RSNAME 0
#define COLUMN_PEERID 1
#define COLUMN_IN_RATE 2
#define COLUMN_IN_MAX 3
#define COLUMN_IN_QUEUE 4
#define COLUMN_IN_ALLOC 5
#define COLUMN_IN_ALLOC_SENT 6
#define COLUMN_OUT_RATE 7
#define COLUMN_OUT_MAX 8
#define COLUMN_OUT_QUEUE 9
#define COLUMN_OUT_ALLOC 10
#define COLUMN_OUT_ALLOC_SENT 11
#define COLUMN_ALLOWED RECVD 12
#define COLUMN_COUNT 13


class QModelIndex;
class QPainter;

class BWListDelegate: public QAbstractItemDelegate {

	Q_OBJECT

public:
	BWListDelegate(QObject *parent=0);
	~BWListDelegate();
	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
	QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;

private:

public slots:

signals:
};

namespace Ui {
    class BwCtrlWindow;
}

class BwCtrlWindow : public QMainWindow {
    Q_OBJECT
public:

    static void showYourself ();
    static BwCtrlWindow* getInstance();
    static void releaseInstance();


    BwCtrlWindow(QWidget *parent = 0);
    ~BwCtrlWindow();

	void updateBandwidth();

public slots:
	void update();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::BwCtrlWindow *ui;

    static BwCtrlWindow *mInstance;

	BWListDelegate *BWDelegate;

};

#endif // RSBWCTRL_WINDOW_H

