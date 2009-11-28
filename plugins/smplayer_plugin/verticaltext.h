/* Taken from KMix */
/* Copyright (C) 2003-2004 Christian Esken <esken@kde.org> */


#ifndef VerticalText_h
#define VerticalText_h

#include <QWidget>
#include <QPaintEvent>

class VerticalText : public QWidget
{
public:
    VerticalText(QWidget * parent, Qt::WindowFlags f = 0);
    ~VerticalText();

	void setText(QString s) { _label = s; };
	QString text() { return _label; };
    QSize sizeHint() const;
    QSizePolicy sizePolicy () const;
	
protected:
    void paintEvent ( QPaintEvent * event );
	QString _label;
};

#endif
