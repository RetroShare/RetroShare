#ifndef CHATTABWIDGET_H
#define CHATTABWIDGET_H

#include <QTabWidget>

namespace Ui {
class ChatTabWidget;
}

class PopupChatDialog;

class ChatTabWidget : public QTabWidget
{
	Q_OBJECT

public:
	explicit ChatTabWidget(QWidget *parent = 0);
	~ChatTabWidget();

	void addDialog(PopupChatDialog *dialog);
	void removeDialog(PopupChatDialog *dialog);

	void getInfo(bool &isTyping, bool &hasNewMessage, QIcon *icon);

signals:
	void tabChanged(PopupChatDialog *dialog);
	void infoChanged();

private slots:
    void tabClose(int tab);
    void tabChanged(int tab);
    void tabInfoChanged(PopupChatDialog *dialog);

private:
	Ui::ChatTabWidget *ui;
};

#endif // CHATTABWIDGET_H
