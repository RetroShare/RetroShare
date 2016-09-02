#include "RsIconWidget.h"

RsIconWidget::RsIconWidget(QWidget *parent /*= 0*/)
  : QLabel(parent), m_Icon(RsIcon())
{
}

RsIconWidget::RsIconWidget(QIcon icon, QWidget *parent = 0)
  : QLabel(parent), m_Icon(icon)
{
}

void RsIconWidget::paintEvent(QPaintEvent *paintEvent)
{
	if (!m_Icon.isNull()) {
		int w = this->fontMetrics().height();
		this->setPixmap(m_Icon.pixmap(w*1.5, w*1.5));
	}
	QLabel::paintEvent(paintEvent);
}
