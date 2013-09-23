#ifndef RSIMAGEBLOCKWIDGET_H
#define RSIMAGEBLOCKWIDGET_H

#include <QWidget>

namespace Ui {
class RSImageBlockWidget;
}

class RSImageBlockWidget : public QWidget
{
	Q_OBJECT

public:
	explicit RSImageBlockWidget(QWidget *parent = 0);
	~RSImageBlockWidget();

signals:
	void showImages();

private:
	Ui::RSImageBlockWidget *ui;
};

#endif // RSIMAGEBLOCKWIDGET_H
