#pragma once

#include <QWidget>

class OutQueueStatistics ;

class OutQueueStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		OutQueueStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

        void updateStatistics(OutQueueStatistics&) ;

	private:
		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};
