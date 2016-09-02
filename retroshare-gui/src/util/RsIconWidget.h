#ifndef RSICONWIDGET_H
#define RSICONWIDGET_H

#include <QIcon>
#include <QLabel>

#include "util/RsIcon.h"

class RsIconWidget : public QLabel
{
Q_OBJECT
	Q_PROPERTY(QIcon icon READ icon WRITE setIcon)

public:
	RsIconWidget(QWidget *parent = 0);
	RsIconWidget(QIcon icon, QWidget *parent);
	~RsIconWidget() {}

	void setIcon(QIcon icon) { m_Icon = icon; }
	QIcon icon() { return m_Icon; }

private:
	void paintEvent(QPaintEvent *paintEvent);

	QIcon m_Icon;
};

#endif // RSICONWIDGET_H
