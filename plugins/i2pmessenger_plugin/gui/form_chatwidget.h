#ifndef FORM_CHATWIDGET_h
#define FORM_CHATWIDGET_h


#include "ui_form_chatwidget.h"
#include "gui_icons.h"

#include <QtGui>
#include <Qt>
#include <QClipboard>
#include <QKeyEvent>
class ChatEventEater : public QObject
{
	Q_OBJECT

public:
	ChatEventEater(QWidget *parent = 0) : QObject(parent){ }
	bool m_send_on_enter;
	
signals:
	void sendMessage();
protected:
	bool eventFilter(QObject *obj, QEvent *event);

};

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
	void WorkAround();
	void changeWindowsTitle();
	

signals:
	void sendChatMessage(QString chatMessage);

private:
	QColor textColor;
	QStringList history;
	cUser* user;
	QFont  mCurrentFont;
	ChatEventEater *m_event_eater;
};
#endif
