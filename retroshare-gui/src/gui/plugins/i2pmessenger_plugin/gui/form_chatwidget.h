#ifndef FORM_CHATWIDGET_h
#define FORM_CHATWIDGET_h


#include "ui_form_chatwidget.h"
#include <QtGui>
#include <Qt>

#include <QClipboard>

class cUser;
class form_ChatWidget : public QWidget, public Ui::form_chatwidget
{
Q_OBJECT
public:
	form_ChatWidget(cUser* user,QWidget* parent = 0);
	void closeEvent(QCloseEvent *e);
private slots:
	void sendMessageSignal();
	void addMessage(QString text);
	void setTextColor();
	void newMessageRecived();
	void setBold(bool t);
	void setFont();
	

signals:
	void sendChatMessage(QString chatMessage);

private:
	QColor textColor;
	QStringList history;
	cUser* user;
	QFont  mCurrentFont;
};
#endif