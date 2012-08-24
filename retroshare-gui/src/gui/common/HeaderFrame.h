#ifndef HEADERFRAME_H
#define HEADERFRAME_H

#include <QFrame>

namespace Ui {
class HeaderFrame;
}

class HeaderFrame : public QFrame
{
	Q_OBJECT
    
public:
	HeaderFrame(QWidget *parent = 0);
	~HeaderFrame();

	void setHeaderText(const QString &headerText);
	void setHeaderImage(const QPixmap &headerImage);

private:
	Ui::HeaderFrame *ui;
};

#endif // HEADERFRAME_H
